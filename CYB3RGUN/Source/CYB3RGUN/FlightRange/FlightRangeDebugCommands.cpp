// CYB3RGUN THEGAME. Console commands that drive the flight range for automated verification.
// Not compiled into shipping builds.

#include "FlightRangeGameMode.h"
#include "FlightRangeSettings.h"
#include "FlightTarget.h"
#include "ShooterCharacter.h"
#include "WeaponStatus.h"
#include "Containers/Ticker.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"

#if !UE_BUILD_SHIPPING

DEFINE_LOG_CATEGORY_STATIC(LogFlightRangeDebug, Log, All);

namespace FlightRangeDebug
{
	AFlightRangeGameMode* GetRange(UWorld* World)
	{
		return World ? Cast<AFlightRangeGameMode>(World->GetAuthGameMode()) : nullptr;
	}

	/**
	 *  A stand in shooter for unattended rounds: turns towards the lead point of the nearest target in front at a limited
	 *  turn rate, waits a reaction time on a new target, fires when the aim is close with a small random error, and
	 *  reloads an empty weapon. It is a test driver, not a measure of how a person plays.
	 */
	struct FAutoPlayer
	{
		TWeakObjectPtr<UWorld> World;
		TWeakObjectPtr<AFlightTarget> Current;
		FTSTicker::FDelegateHandle Handle;
		FRotator AimError = FRotator::ZeroRotator;
		double AcquiredAt = 0.0;
		bool bTriggerDown = false;

		static constexpr float TurnDegreesPerSecond = 220.0f;
		static constexpr float ReactionSeconds = 0.3f;
		static constexpr float FireToleranceDegrees = 1.2f;
		static constexpr float AimErrorDegrees = 0.9f;
		static constexpr float MaxRange = 2400.0f;

		bool Tick(float DeltaSeconds)
		{
			UWorld* W = World.Get();
			AFlightRangeGameMode* Range = GetRange(W);
			APlayerController* PC = W ? UGameplayStatics::GetPlayerController(W, 0) : nullptr;
			AShooterCharacter* Shooter = PC ? Cast<AShooterCharacter>(PC->GetPawn()) : nullptr;
			if (!Range || !Shooter || W->IsPaused())
			{
				return true;
			}
			if (bTriggerDown)
			{
				Shooter->DoStopFiring();
				bTriggerDown = false;
			}
			if (Range->GetRoundState() != EFlightRoundState::Running || !Range->GetDirector())
			{
				return true;
			}

			FWeaponStatus Status;
			if (Shooter->GetWeaponStatus(Status) && Status.bEmpty && !Status.bReloading)
			{
				Shooter->DoReload();
				return true;
			}

			FVector ViewLocation;
			FRotator ViewRotation;
			PC->GetPlayerViewPoint(ViewLocation, ViewRotation);

			// keep the target while it is up, otherwise the one needing the smallest turn
			AFlightTarget* Target = Current.Get();
			if (!Target || Target->IsDown())
			{
				Target = nullptr;
				float BestTurn = TNumericLimits<float>::Max();
				TArray<AFlightTarget*> Targets;
				Range->GetDirector()->GetTargets(Targets);
				for (AFlightTarget* Candidate : Targets)
				{
					const FVector ToTarget = Candidate->GetLeadPoint() - ViewLocation;
					if (Candidate->IsDown() || ToTarget.Size() > MaxRange)
					{
						continue;
					}
					const float Turn = FMath::Abs(FRotator::NormalizeAxis(ToTarget.Rotation().Yaw - ViewRotation.Yaw));
					if (Turn < BestTurn)
					{
						BestTurn = Turn;
						Target = Candidate;
					}
				}
				Current = Target;
				AcquiredAt = FPlatformTime::Seconds();
				AimError = FRotator(FMath::FRandRange(-AimErrorDegrees, AimErrorDegrees), FMath::FRandRange(-AimErrorDegrees, AimErrorDegrees), 0.0f);
			}
			if (!Target)
			{
				return true;
			}

			const FRotator Wanted = (Target->GetLeadPoint() - ViewLocation).Rotation() + AimError;
			const FRotator Delta = (Wanted - PC->GetControlRotation()).GetNormalized();
			const float MaxStep = TurnDegreesPerSecond * DeltaSeconds;
			FRotator Step(FMath::Clamp(Delta.Pitch, -MaxStep, MaxStep), FMath::Clamp(Delta.Yaw, -MaxStep, MaxStep), 0.0f);
			PC->SetControlRotation((PC->GetControlRotation() + Step).GetNormalized());

			const bool bAligned = FMath::Abs(Delta.Pitch) < FireToleranceDegrees && FMath::Abs(Delta.Yaw) < FireToleranceDegrees;
			if (bAligned && FPlatformTime::Seconds() - AcquiredAt >= ReactionSeconds)
			{
				Shooter->DoStartFiring();
				bTriggerDown = true;
				AimError = FRotator(FMath::FRandRange(-AimErrorDegrees, AimErrorDegrees), FMath::FRandRange(-AimErrorDegrees, AimErrorDegrees), 0.0f);
			}
			return true;
		}
	};

	FAutoPlayer& GetAutoPlayer()
	{
		static FAutoPlayer Player;
		return Player;
	}
}

static FAutoConsoleCommandWithWorldAndArgs GFlightRangeAutoPlayCommand(
	TEXT("FlightRange.AutoPlay"),
	TEXT("FlightRange.AutoPlay 1|0. A test driver aims at the lead point of the nearest target, fires and reloads."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		FlightRangeDebug::FAutoPlayer& Player = FlightRangeDebug::GetAutoPlayer();
		const bool bOn = Args.IsEmpty() || Args[0] != TEXT("0");
		if (Player.Handle.IsValid())
		{
			FTSTicker::GetCoreTicker().RemoveTicker(Player.Handle);
			Player.Handle.Reset();
		}
		Player.World = World;
		Player.Current.Reset();
		if (bOn)
		{
			Player.Handle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([](float DeltaSeconds)
			{
				return FlightRangeDebug::GetAutoPlayer().Tick(DeltaSeconds);
			}));
		}
		UE_LOG(LogFlightRangeDebug, Log, TEXT("FlightRange.AutoPlay %s"), bOn ? TEXT("on") : TEXT("off"));
	}));

static FAutoConsoleCommandWithWorld GFlightRangeEndCommand(
	TEXT("FlightRange.End"),
	TEXT("Ends the flight range round now, as if the time had run out."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		if (AFlightRangeGameMode* Range = FlightRangeDebug::GetRange(World))
		{
			Range->FinishRound();
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GFlightRangeTimeCommand(
	TEXT("FlightRange.Time"),
	TEXT("FlightRange.Time <seconds>. Sets the time left in the round."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (AFlightRangeGameMode* Range = FlightRangeDebug::GetRange(World); Range && !Args.IsEmpty())
		{
			Range->SetSecondsLeft(FCString::Atof(*Args[0]));
		}
	}));

static FAutoConsoleCommandWithWorld GFlightRangeStatusCommand(
	TEXT("FlightRange.Status"),
	TEXT("Logs the round state, time, score, hits, misses and the targets in flight."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		AFlightRangeGameMode* Range = FlightRangeDebug::GetRange(World);
		if (!Range)
		{
			UE_LOG(LogFlightRangeDebug, Warning, TEXT("FlightRange.Status: no flight range"));
			return;
		}
		const FFlightRangeStats Stats = Range->GetStats();
		UE_LOG(LogFlightRangeDebug, Log, TEXT("FlightRange.Status: state %d, %d s left, score %d, hits %d of %d, misses %d, shots %d, in flight %d"),
			static_cast<int32>(Range->GetRoundState()), Range->GetSecondsLeft(), Stats.FinalScore, Stats.TargetsHit, Stats.TargetsLaunched, Stats.Misses, Stats.ShotsFired,
			Range->GetDirector() ? Range->GetDirector()->CountInFlight() : 0);
	}));

#endif
