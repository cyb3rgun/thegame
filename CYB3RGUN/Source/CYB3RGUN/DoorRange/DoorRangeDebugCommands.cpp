// CYB3RGUN THEGAME. Console commands that drive the door range for automated verification.
// Not compiled into shipping builds.

#include "DoorRangeGameMode.h"
#include "DoorSlot.h"
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
	ADoorRangeGameMode* GetRange(UWorld* World)
	{
		return World ? Cast<ADoorRangeGameMode>(World->GetAuthGameMode()) : nullptr;
	}

	/** Turns the local player toward a slot that currently shows the wanted occupant. Returns the slot or null. */
	ADoorSlot* AimAt(UWorld* World, EDoorOccupant Wanted)
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
				const FRotator Look = UKismetMathLibrary::FindLookAtRotation(Eye, Slot->GetOccupantAimPoint());
				PC->SetControlRotation(Look);
				return Slot;
			}
		}
		return nullptr;
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
		if (Args.Num() > 0 && Args[0].Equals(TEXT("Friendly"), ESearchCase::IgnoreCase))
		{
			return EDoorOccupant::Friendly;
		}
		return EDoorOccupant::Hostile;
	}
}

static FAutoConsoleCommandWithWorldAndArgs GDoorRangeFireCommand(
	TEXT("DoorRange.Fire"),
	TEXT("Aims the local player at a drawn occupant and fires once. Argument: Hostile (default) or Friendly. Without a matching target it fires straight ahead."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		const EDoorOccupant Wanted = DoorRangeDebug::ParseOccupant(Args);
		ADoorSlot* Target = DoorRangeDebug::AimAt(World, Wanted);
		UE_LOG(LogDoorRangeDebug, Log, TEXT("DoorRange.Fire: target %s"), Target ? *Target->GetName() : TEXT("none, firing ahead"));

		// the camera follows the control rotation on the next update, so fire one tick later
		if (World)
		{
			World->GetTimerManager().SetTimerForNextTick([World]()
			{
				DoorRangeDebug::Fire(World);
			});
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GDoorRangeAimCommand(
	TEXT("DoorRange.Aim"),
	TEXT("Aims the local player at a drawn occupant without firing. Argument: Hostile (default) or Friendly."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		ADoorSlot* Target = DoorRangeDebug::AimAt(World, DoorRangeDebug::ParseOccupant(Args));
		UE_LOG(LogDoorRangeDebug, Log, TEXT("DoorRange.Aim: target %s"), Target ? *Target->GetName() : TEXT("none"));
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
