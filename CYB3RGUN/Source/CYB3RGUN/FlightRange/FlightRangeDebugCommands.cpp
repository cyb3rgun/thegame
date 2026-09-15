// CYB3RGUN THEGAME. Console commands that drive the flight range for automated verification.
// Not compiled into shipping builds.

#include "FlightGalleryControls.h"
#include "FlightRangeGameMode.h"
#include "RailAimComponent.h"
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
	 *  A stand in shooter for unattended rounds: moves the crosshair towards the lead point of the nearest target at a limited
	 *  speed and slides the gallery towards it when it is outside the view, waits a reaction time on a new target, fires when
	 *  the crosshair is close with a small random error, and reloads an empty weapon. It is a test driver, not a measure of
	 *  how a person plays.
	 */
	struct FAutoPlayer
	{
		TWeakObjectPtr<UWorld> World;
		TWeakObjectPtr<AFlightTarget> Current;
		FTSTicker::FDelegateHandle Handle;
		FVector2D ScreenError = FVector2D::ZeroVector;
		double AcquiredAt = 0.0;
		bool bTriggerDown = false;

		static constexpr float ScreensPerSecond = 1.2f;
		static constexpr float ReactionSeconds = 0.3f;
		static constexpr float FireTolerance = 0.012f;
		static constexpr float AimError = 0.008f;
		static constexpr float MaxRange = 2400.0f;

		bool Tick(float DeltaSeconds)
		{
			UWorld* W = World.Get();
			AFlightRangeGameMode* Range = GetRange(W);
			APlayerController* PC = W ? UGameplayStatics::GetPlayerController(W, 0) : nullptr;
			AShooterCharacter* Shooter = PC ? Cast<AShooterCharacter>(PC->GetPawn()) : nullptr;
			URailAimComponent* Aim = Shooter ? Shooter->FindComponentByClass<URailAimComponent>() : nullptr;
			UFlightGalleryControls* Gallery = Range ? Range->GetGallery() : nullptr;
			if (!Range || !Shooter || !Aim || !Gallery || W->IsPaused())
			{
				return true;
			}
			if (bTriggerDown)
			{
				Shooter->DoStopFiring();
				bTriggerDown = false;
			}
			Gallery->SetDriverScroll(0.0f);
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

			// keep the target while it is up, otherwise the one nearest the view sideways; the range counts depth and height, the gallery slides for the rest
			AFlightTarget* Target = Current.Get();
			const FVector Right = Gallery->GetRailRight();
			if (!Target || Target->IsDown())
			{
				Target = nullptr;
				float BestSide = TNumericLimits<float>::Max();
				TArray<AFlightTarget*> Targets;
				Range->GetDirector()->GetTargets(Targets);
				for (AFlightTarget* Candidate : Targets)
				{
					const FVector ToTarget = Candidate->GetLeadPoint() - ViewLocation;
					if (Candidate->IsDown() || (ToTarget - Right * FVector::DotProduct(ToTarget, Right)).Size() > MaxRange || FVector::DotProduct(ToTarget, ViewRotation.Vector()) < 100.0f)
					{
						continue;
					}
					const float Side = FMath::Abs(FVector::DotProduct(ToTarget, Right));
					if (Side < BestSide)
					{
						BestSide = Side;
						Target = Candidate;
					}
				}
				Current = Target;
				AcquiredAt = FPlatformTime::Seconds();
				ScreenError = FVector2D(FMath::FRandRange(-AimError, AimError), FMath::FRandRange(-AimError, AimError));
			}
			if (!Target)
			{
				return true;
			}

			// a lead point off the screen slides the gallery towards it
			FVector2D Pixels;
			int32 SizeX = 0;
			int32 SizeY = 0;
			PC->GetViewportSize(SizeX, SizeY);
			const bool bOnScreen = PC->ProjectWorldLocationToScreen(Target->GetLeadPoint(), Pixels) && SizeX > 0 && SizeY > 0;
			const FVector2D Wanted = bOnScreen ? FVector2D(Pixels.X / SizeX, Pixels.Y / SizeY) + ScreenError : FVector2D(0.5, 0.5);
			if (!bOnScreen || Wanted.X < 0.05 || Wanted.X > 0.95)
			{
				Gallery->SetDriverScroll(FVector::DotProduct(Target->GetLeadPoint() - ViewLocation, Right) >= 0.0f ? 1.0f : -1.0f);
				return true;
			}

			const FVector2D Delta = Wanted - Aim->GetCrosshairNormalized();
			const double MaxStep = ScreensPerSecond * DeltaSeconds;
			Aim->AddAimInput(FVector2D(FMath::Clamp(Delta.X, -MaxStep, MaxStep), FMath::Clamp(Delta.Y, -MaxStep, MaxStep)));

			const bool bAligned = FMath::Abs(Delta.X) < FireTolerance && FMath::Abs(Delta.Y) < FireTolerance;
			if (bAligned && FPlatformTime::Seconds() - AcquiredAt >= ReactionSeconds)
			{
				Shooter->DoStartFiring();
				bTriggerDown = true;
				ScreenError = FVector2D(FMath::FRandRange(-AimError, AimError), FMath::FRandRange(-AimError, AimError));
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
