// CYB3RGUN THEGAME. Console commands that drive the door range for automated verification.
// Not compiled into shipping builds.

#include "DoorRangeGameMode.h"
#include "DoorSlot.h"
#include "DoorRangeSettings.h"
#include "EngineUtils.h"
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
		bool bHostage = false;

		/** Bone of the occupant to aim at, from Bone=<name> */
		FName Bone;

		/** Aim at the occupant's pistol */
		bool bWeapon = false;
	};

	ADoorRangeGameMode* GetRange(UWorld* World)
	{
		return World ? Cast<ADoorRangeGameMode>(World->GetAuthGameMode()) : nullptr;
	}

	/** Turns the local player toward a slot that currently shows the wanted occupant: its chest, its head, a named bone, its
	 *  pistol or a taker's hostage. Returns the slot or null. */
	ADoorSlot* AimAt(UWorld* World, const FFireOptions& Options)
	{
		ADoorRangeGameMode* Range = GetRange(World);
		APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
		if (!Range || !PC || !PC->GetPawn())
		{
			return nullptr;
		}

		for (ADoorSlot* Slot : Range->GetSlots())
		{
			if (!Slot || Slot->GetOccupant() != Options.Occupant || !Slot->IsDrawn())
			{
				continue;
			}

			FVector Point;
			if (Options.bWeapon)
			{
				if (!Slot->GetOccupantWeaponPoint(Point))
				{
					continue;
				}
			}
			else if (!Options.Bone.IsNone())
			{
				if (!Slot->GetOccupantBonePoint(Options.Bone, Options.bHostage, Point))
				{
					continue;
				}
			}
			else
			{
				Point = Options.bHostage ? Slot->GetHostageAimPoint() : (Options.bHead ? Slot->GetOccupantHeadPoint() : Slot->GetOccupantAimPoint());
			}

			const FVector Eye = PC->GetPawn()->GetPawnViewLocation();
			PC->SetControlRotation(UKismetMathLibrary::FindLookAtRotation(Eye, Point));
			return Slot;
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
			if (Arg.Equals(TEXT("Taker"), ESearchCase::IgnoreCase))
			{
				return EDoorOccupant::HostageTaker;
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
			Options.bHostage |= Arg.Equals(TEXT("Hostage"), ESearchCase::IgnoreCase);
			Options.bWeapon |= Arg.Equals(TEXT("Weapon"), ESearchCase::IgnoreCase);
			if (Arg.StartsWith(TEXT("Bone="), ESearchCase::IgnoreCase))
			{
				Options.Bone = FName(*Arg.Mid(5));
			}
		}
		return Options;
	}
}

static FAutoConsoleCommandWithWorldAndArgs GDoorRangeFireCommand(
	TEXT("DoorRange.Fire"),
	TEXT("Aims the local player at a drawn occupant and fires once. Arguments in any order: Hostile (default), Friendly or Taker for a hostage taker, Hostage to aim at a taker's hostage, Head to aim at the head, Pair for a second shot three tenths of a second later, past the refire and inside the pair window, Miss to fire into the sky, Weapon to aim at the pistol, Bone=<name> to aim at a bone such as Bone=thigh_r or Bone=lowerarm_r. Without a matching target it fires straight ahead."),
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
			Target = DoorRangeDebug::AimAt(World, Options);
		}
		const FString BoneText = Options.Bone.IsNone() ? FString() : FString::Printf(TEXT(", bone %s"), *Options.Bone.ToString());
		UE_LOG(LogDoorRangeDebug, Log, TEXT("DoorRange.Fire: target %s%s%s%s%s%s"), Target ? *Target->GetName() : (Options.bMiss ? TEXT("the sky") : TEXT("none, firing ahead")),
			Options.bHead ? TEXT(", head") : TEXT(""), Options.bPair ? TEXT(", pair") : TEXT(""), Options.bHostage ? TEXT(", hostage") : TEXT(""),
			Options.bWeapon ? TEXT(", weapon") : TEXT(""), *BoneText);

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
	TEXT("Aims the local player at a drawn occupant without firing. Arguments: Hostile (default), Friendly or Taker, Head to aim at the head, Hostage to aim at a taker's hostage, Weapon to aim at the pistol, Bone=<name> to aim at a bone."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		const DoorRangeDebug::FFireOptions Options = DoorRangeDebug::ParseOptions(Args);
		ADoorSlot* Target = DoorRangeDebug::AimAt(World, Options);
		UE_LOG(LogDoorRangeDebug, Log, TEXT("DoorRange.Aim: target %s"), Target ? *Target->GetName() : TEXT("none"));
	}));

static FAutoConsoleCommandWithWorldAndArgs GDoorRangeOpenCommand(
	TEXT("DoorRange.Open"),
	TEXT("Opens a free door right away with the given occupant, outside the wave's own roll. Argument: Hostile (default), Friendly or Taker."),
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
				Params.HostageMaterial = Cfg->FriendlyMaterial;
				Params.OpenDuration = Cfg->OpenDuration;
				Params.CloseDuration = Cfg->CloseDuration;
				Params.ExposureWindow = Wave.ExposureWindow * (Occupant == EDoorOccupant::HostageTaker ? Cfg->HostageTakerExposureScale : 1.0f);
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
			UE_LOG(LogDoorRangeDebug, Log, TEXT("DoorRange.Status: score %d, wave %d/%d, hostiles left %d/%d, hit %d, escaped %d, friendlies hit %d, hostages freed %d, hostages hit %d, active %d, complete %d"),
				Range->GetScore(), Range->GetCurrentWave(), Range->GetWaveCount(), Range->GetHostilesRemaining(), Range->GetHostilesTotalThisWave(),
				Stats.HostilesHit, Stats.HostilesEscaped, Stats.FriendliesHit, Stats.HostagesRescued, Stats.HostagesHit, Range->IsRangeActive() ? 1 : 0, Range->IsRangeComplete() ? 1 : 0);
		}
		else
		{
			UE_LOG(LogDoorRangeDebug, Log, TEXT("DoorRange.Status: no door range game mode in this world"));
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GDoorRangeRepairSlotsCommand(
	TEXT("DoorRange.RepairSlots"),
	TEXT("Counts door slot hostile parts saved outside the hostile root. With the argument fix it moves them and marks the level for saving."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		const bool bFix = Args.Num() > 0 && Args[0].Equals(TEXT("fix"), ESearchCase::IgnoreCase);
		int32 Slots = 0;
		int32 Stale = 0;
		for (TActorIterator<ADoorSlot> It(World); It; ++It)
		{
			const int32 SlotStale = It->RepairHostileSet(bFix);
			if (SlotStale > 0)
			{
				UE_LOG(LogDoorRangeDebug, Log, TEXT("DoorRange.RepairSlots: %s has %d hostile parts outside the hostile root"), *It->GetName(), SlotStale);
			}
			Stale += SlotStale;
			++Slots;
		}
		UE_LOG(LogDoorRangeDebug, Log, TEXT("DoorRange.RepairSlots: %d slots in %s, %d stale hostile parts, %s"), Slots, *GetNameSafe(World), Stale, bFix ? TEXT("moved") : TEXT("checked only"));
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
