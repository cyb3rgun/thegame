// CYB3RGUN THEGAME. Console commands that drive the door range for automated verification.
// Not compiled into shipping builds.

#include "DoorRangeGameMode.h"
#include "DoorSlot.h"
#include "DoorRangeSettings.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Misc/OutputDeviceNull.h"
#include "TimerManager.h"

#if !UE_BUILD_SHIPPING

DEFINE_LOG_CATEGORY_STATIC(LogDoorRangeDebug, Log, All);

namespace DoorRangeDebug
{
	/** What DoorRange.Fire should do */
	struct FFireOptions
	{
		EDoorOccupant Occupant = EDoorOccupant::Hostile;
		bool bHead = false;
		bool bPair = false;
		bool bMiss = false;
	};

	ADoorRangeGameMode* GetRange(UWorld* World)
	{
		return World ? Cast<ADoorRangeGameMode>(World->GetAuthGameMode()) : nullptr;
	}

	/** Turns the local player toward a slot that currently shows the wanted occupant. Returns the slot or null. */
	ADoorSlot* AimAt(UWorld* World, EDoorOccupant Wanted, bool bHead)
	{
		ADoorRangeGameMode* Range = GetRange(World);
		APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
		if (!Range || !PC || !PC->GetPawn())
		{
			return nullptr;
		}

		for (ADoorSlot* Slot : Range->GetSlots())
		{
			if (Slot && Slot->GetOccupant() == Wanted && Slot->IsDrawn())
			{
				const FVector Eye = PC->GetPawn()->GetPawnViewLocation();
				const FVector Point = bHead ? Slot->GetOccupantHeadPoint() : Slot->GetOccupantAimPoint();
				PC->SetControlRotation(UKismetMathLibrary::FindLookAtRotation(Eye, Point));
				return Slot;
			}
		}
		return nullptr;
	}

	/** Turns the local player up into the sky, where a shot lands on nothing */
	void AimAtSky(UWorld* World)
	{
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0))
		{
			FRotator Look = PC->GetControlRotation();
			Look.Pitch = 60.0f;
			PC->SetControlRotation(Look);
		}
	}

	/** Pulls the trigger of the current weapon once through the character's own input handlers. */
	void Fire(UWorld* World)
	{
		APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
		if (!PC || !PC->GetPawn())
		{
			return;
		}

		FOutputDeviceNull Ar;
		PC->GetPawn()->CallFunctionByNameWithArguments(TEXT("DoStartFiring"), Ar, nullptr, true);
		PC->GetPawn()->CallFunctionByNameWithArguments(TEXT("DoStopFiring"), Ar, nullptr, true);
	}

	EDoorOccupant ParseOccupant(const TArray<FString>& Args)
	{
		for (const FString& Arg : Args)
		{
			if (Arg.Equals(TEXT("Friendly"), ESearchCase::IgnoreCase))
			{
				return EDoorOccupant::Friendly;
			}
		}
		return EDoorOccupant::Hostile;
	}

	FFireOptions ParseOptions(const TArray<FString>& Args)
	{
		FFireOptions Options;
		Options.Occupant = ParseOccupant(Args);
		for (const FString& Arg : Args)
		{
			Options.bHead |= Arg.Equals(TEXT("Head"), ESearchCase::IgnoreCase);
			Options.bPair |= Arg.Equals(TEXT("Pair"), ESearchCase::IgnoreCase);
			Options.bMiss |= Arg.Equals(TEXT("Miss"), ESearchCase::IgnoreCase);
		}
		return Options;
	}
}

static FAutoConsoleCommandWithWorldAndArgs GDoorRangeFireCommand(
	TEXT("DoorRange.Fire"),
	TEXT("Aims the local player at a drawn occupant and fires once. Arguments in any order: Hostile (default) or Friendly, Head to aim at the head, Pair for a second shot three tenths of a second later, past the refire and inside the pair window, Miss to fire into the sky. Without a matching target it fires straight ahead."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		const DoorRangeDebug::FFireOptions Options = DoorRangeDebug::ParseOptions(Args);
		ADoorSlot* Target = nullptr;
		if (Options.bMiss)
		{
			DoorRangeDebug::AimAtSky(World);
		}
		else
		{
			Target = DoorRangeDebug::AimAt(World, Options.Occupant, Options.bHead);
		}
		UE_LOG(LogDoorRangeDebug, Log, TEXT("DoorRange.Fire: target %s%s%s"), Target ? *Target->GetName() : (Options.bMiss ? TEXT("the sky") : TEXT("none, firing ahead")),
			Options.bHead ? TEXT(", head") : TEXT(""), Options.bPair ? TEXT(", pair") : TEXT(""));

		// weapons aim through the screen space path (D-019), which reads the view the player last saw.
		// The snap needs a few camera updates before the crosshair shows the target, so the trigger waits for them
		if (World)
		{
			FTimerHandle Handle;
			World->GetTimerManager().SetTimer(Handle, [World]()
			{
				DoorRangeDebug::Fire(World);
			}, 0.1f, false);

			// the second shot of a pair keeps the aim of the first, the target no longer counts as drawn once hit
			if (Options.bPair)
			{
				FTimerHandle PairHandle;
				World->GetTimerManager().SetTimer(PairHandle, [World]()
				{
					DoorRangeDebug::Fire(World);
				}, 0.4f, false);
			}
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GDoorRangeAimCommand(
	TEXT("DoorRange.Aim"),
	TEXT("Aims the local player at a drawn occupant without firing. Arguments: Hostile (default) or Friendly, Head to aim at the head."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		const DoorRangeDebug::FFireOptions Options = DoorRangeDebug::ParseOptions(Args);
		ADoorSlot* Target = DoorRangeDebug::AimAt(World, Options.Occupant, Options.bHead);
		UE_LOG(LogDoorRangeDebug, Log, TEXT("DoorRange.Aim: target %s"), Target ? *Target->GetName() : TEXT("none"));
	}));

static FAutoConsoleCommandWithWorldAndArgs GDoorRangeOpenCommand(
	TEXT("DoorRange.Open"),
	TEXT("Opens a free door right away with the given occupant, outside the wave's own roll. Argument: Hostile (default) or Friendly."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		ADoorRangeGameMode* Range = DoorRangeDebug::GetRange(World);
		if (!Range)
		{
			return;
		}

		const UDoorRangeSettings* Cfg = Range->GetSettings();
		const FDoorWaveSettings& Wave = Cfg->GetWave(FMath::Max(Range->GetCurrentWave(), 1));
		const EDoorOccupant Occupant = DoorRangeDebug::ParseOccupant(Args);

		for (ADoorSlot* Slot : Range->GetSlots())
		{
			if (Slot && Slot->IsAvailable())
			{
				FDoorOpenParams Params;
				Params.Occupant = Occupant;
				Params.OccupantMaterial = Occupant == EDoorOccupant::Friendly ? Cfg->FriendlyMaterial : Cfg->HostileMaterial;
				Params.OpenDuration = Cfg->OpenDuration;
				Params.CloseDuration = Cfg->CloseDuration;
				Params.ExposureWindow = Wave.ExposureWindow;
				Params.TelegraphDuration = Wave.TelegraphDuration;
				Slot->Open(Params);
				UE_LOG(LogDoorRangeDebug, Log, TEXT("DoorRange.Open: %s opens with occupant %d"), *Slot->GetName(), static_cast<int32>(Occupant));
				return;
			}
		}
		UE_LOG(LogDoorRangeDebug, Log, TEXT("DoorRange.Open: no free door"));
	}));

static FAutoConsoleCommandWithWorld GDoorRangeStatusCommand(
	TEXT("DoorRange.Status"),
	TEXT("Logs score, wave and statistics of the running door range."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		if (ADoorRangeGameMode* Range = DoorRangeDebug::GetRange(World))
		{
			const FDoorRangeStats& Stats = Range->GetStats();
			UE_LOG(LogDoorRangeDebug, Log, TEXT("DoorRange.Status: score %d, wave %d/%d, hostiles left %d/%d, hit %d, escaped %d, friendlies hit %d, active %d, complete %d"),
				Range->GetScore(), Range->GetCurrentWave(), Range->GetWaveCount(), Range->GetHostilesRemaining(), Range->GetHostilesTotalThisWave(),
				Stats.HostilesHit, Stats.HostilesEscaped, Stats.FriendliesHit, Range->IsRangeActive() ? 1 : 0, Range->IsRangeComplete() ? 1 : 0);
		}
		else
		{
			UE_LOG(LogDoorRangeDebug, Log, TEXT("DoorRange.Status: no door range game mode in this world"));
		}
	}));

static FAutoConsoleCommandWithWorld GDoorRangeRestartCommand(
	TEXT("DoorRange.Restart"),
	TEXT("Restarts the running door range from wave one."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		if (ADoorRangeGameMode* Range = DoorRangeDebug::GetRange(World))
		{
			Range->StartRange();
		}
	}));

#endif
