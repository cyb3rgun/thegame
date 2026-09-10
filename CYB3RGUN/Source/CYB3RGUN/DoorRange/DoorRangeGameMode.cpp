// CYB3RGUN THEGAME. Game mode running the door range.

#include "DoorRangeGameMode.h"
#include "DoorSlot.h"
#include "DoorRangeSettings.h"
#include "ShooterWeapon.h"
#include "ShooterWeaponHolder.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogDoorRange, Log, All);

ADoorRangeGameMode::ADoorRangeGameMode()
{
}

const UDoorRangeSettings* ADoorRangeGameMode::GetSettings() const
{
	return Settings ? Settings.Get() : GetDefault<UDoorRangeSettings>();
}

int32 ADoorRangeGameMode::GetWaveCount() const
{
	return GetSettings()->WaveCount;
}

void ADoorRangeGameMode::BeginPlay()
{
	Super::BeginPlay();

	CollectSlots();

	UE_LOG(LogDoorRange, Log, TEXT("Door range ready with %d slots"), Slots.Num());

	if (bAutoStart)
	{
		StartRange();
	}
}

void ADoorRangeGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(OpenTimer);
	GetWorldTimerManager().ClearTimer(WaveTimer);

	Super::EndPlay(EndPlayReason);
}

void ADoorRangeGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);

	// hand out the weapon on the next tick so the pawn has finished its own BeginPlay
	GetWorldTimerManager().SetTimerForNextTick(this, &ADoorRangeGameMode::GrantStartingWeapon);
}

void ADoorRangeGameMode::GrantStartingWeapon()
{
	const UDoorRangeSettings* Cfg = GetSettings();
	if (!Cfg->StartingWeaponClass)
	{
		UE_LOG(LogDoorRange, Warning, TEXT("No starting weapon class set in the door range settings"));
		return;
	}

	if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		if (IShooterWeaponHolder* Holder = Cast<IShooterWeaponHolder>(PC->GetPawn()))
		{
			Holder->AddWeaponClass(Cfg->StartingWeaponClass);
			UE_LOG(LogDoorRange, Log, TEXT("Granted starting weapon %s"), *GetNameSafe(Cfg->StartingWeaponClass));
		}
		else
		{
			UE_LOG(LogDoorRange, Warning, TEXT("Player pawn does not implement ShooterWeaponHolder, no weapon granted"));
		}
	}
}

void ADoorRangeGameMode::CollectSlots()
{
	Slots.Reset();

	for (TActorIterator<ADoorSlot> It(GetWorld()); It; ++It)
	{
		Slots.Add(*It);
	}

	Slots.Sort([](const ADoorSlot& A, const ADoorSlot& B)
	{
		return A.GetName() < B.GetName();
	});

	for (ADoorSlot* Slot : Slots)
	{
		Slot->OnSlotHit.AddUniqueDynamic(this, &ADoorRangeGameMode::HandleSlotHit);
		Slot->OnSlotClosed.AddUniqueDynamic(this, &ADoorRangeGameMode::HandleSlotClosed);
	}
}

void ADoorRangeGameMode::StartRange()
{
	if (Slots.IsEmpty())
	{
		UE_LOG(LogDoorRange, Warning, TEXT("Cannot start the door range, no door slots in the level"));
		return;
	}

	Score = 0;
	OpenDoors = 0;
	bRangeComplete = false;
	bRangeActive = true;

	OnScoreChanged.Broadcast(Score, 0);
	StartWave(1);
}

void ADoorRangeGameMode::StartWave(int32 WaveNumber)
{
	CurrentWave = WaveNumber;
	OpeningsThisWave = 0;

	UE_LOG(LogDoorRange, Log, TEXT("Wave %d of %d starts"), CurrentWave, GetWaveCount());
	OnWaveChanged.Broadcast(CurrentWave, GetWaveCount());
	UpdateDebugDisplay(FString::Printf(TEXT("Wave %d starts"), CurrentWave));

	const float Interval = FMath::Max(GetSettings()->TimeBetweenOpenings, 0.05f);
	GetWorldTimerManager().SetTimer(OpenTimer, this, &ADoorRangeGameMode::TryOpenDoor, Interval, true, 0.0f);
}

void ADoorRangeGameMode::TryOpenDoor()
{
	const UDoorRangeSettings* Cfg = GetSettings();

	if (OpeningsThisWave >= Cfg->OpeningsPerWave)
	{
		// all openings of this wave are scheduled, wait for the last doors to close
		GetWorldTimerManager().ClearTimer(OpenTimer);
		if (OpenDoors == 0)
		{
			EndWave();
		}
		return;
	}

	if (OpenDoors >= Cfg->VisibleDoors)
	{
		return;
	}

	ADoorSlot* Slot = PickAvailableSlot();
	if (!Slot)
	{
		return;
	}

	const EDoorOccupant Occupant = RollOccupant();
	UMaterialInterface* Material = nullptr;
	switch (Occupant)
	{
	case EDoorOccupant::Hostile:
		Material = Cfg->HostileMaterial;
		break;
	case EDoorOccupant::Friendly:
		Material = Cfg->FriendlyMaterial;
		break;
	default:
		break;
	}

	Slot->Open(Occupant, Material, Cfg->OpenDuration, Cfg->ExposureWindow, Cfg->CloseDuration);
	++OpenDoors;
	++OpeningsThisWave;

	UE_LOG(LogDoorRange, Verbose, TEXT("%s opens with occupant %d (opening %d of %d)"), *Slot->GetName(), static_cast<int32>(Occupant), OpeningsThisWave, Cfg->OpeningsPerWave);
}

void ADoorRangeGameMode::HandleSlotHit(ADoorSlot* Slot, EDoorOccupant Occupant, float ExposureFraction)
{
	const UDoorRangeSettings* Cfg = GetSettings();

	switch (Occupant)
	{
	case EDoorOccupant::Hostile:
	{
		const int32 DrawBonus = FMath::RoundToInt(Cfg->DrawBonusMax * FMath::Clamp(ExposureFraction, 0.0f, 1.0f));
		AddScore(Cfg->HitHostileScore + DrawBonus, FString::Printf(TEXT("Hostile hit on %s, draw bonus %d"), *Slot->GetName(), DrawBonus));
		break;
	}
	case EDoorOccupant::Friendly:
		AddScore(-Cfg->HitFriendlyPenalty, FString::Printf(TEXT("Friendly hit on %s"), *Slot->GetName()));
		break;
	default:
		break;
	}
}

void ADoorRangeGameMode::HandleSlotClosed(ADoorSlot* Slot, EDoorOccupant Occupant, bool bWasHit)
{
	OpenDoors = FMath::Max(OpenDoors - 1, 0);

	if (Occupant == EDoorOccupant::Hostile && !bWasHit)
	{
		AddScore(-GetSettings()->MissedHostilePenalty, FString::Printf(TEXT("Hostile escaped from %s"), *Slot->GetName()));
	}

	if (bRangeActive && OpeningsThisWave >= GetSettings()->OpeningsPerWave && OpenDoors == 0)
	{
		EndWave();
	}
}

void ADoorRangeGameMode::EndWave()
{
	GetWorldTimerManager().ClearTimer(OpenTimer);

	UE_LOG(LogDoorRange, Log, TEXT("Wave %d ends, score %d"), CurrentWave, Score);

	if (CurrentWave >= GetWaveCount())
	{
		FinishRange();
		return;
	}

	UpdateDebugDisplay(FString::Printf(TEXT("Wave %d complete"), CurrentWave));

	const int32 NextWave = CurrentWave + 1;
	GetWorldTimerManager().SetTimer(WaveTimer, [this, NextWave]()
	{
		StartWave(NextWave);
	}, FMath::Max(GetSettings()->TimeBetweenWaves, 0.01f), false);
}

void ADoorRangeGameMode::FinishRange()
{
	bRangeActive = false;
	bRangeComplete = true;

	UE_LOG(LogDoorRange, Log, TEXT("Range complete, final score %d"), Score);
	UpdateDebugDisplay(TEXT("Range complete"));
}

void ADoorRangeGameMode::AddScore(int32 Delta, const FString& Reason)
{
	Score += Delta;

	UE_LOG(LogDoorRange, Log, TEXT("Score %+d -> %d (%s)"), Delta, Score, *Reason);
	OnScoreChanged.Broadcast(Score, Delta);
	UpdateDebugDisplay(FString::Printf(TEXT("%s (%+d)"), *Reason, Delta));
}

void ADoorRangeGameMode::UpdateDebugDisplay(const FString& LastEvent) const
{
	if (!bShowDebugScore || !GEngine)
	{
		return;
	}

	const FString Text = FString::Printf(TEXT("DOOR RANGE   Score %d   Wave %d / %d   %s"), Score, CurrentWave, GetWaveCount(), *LastEvent);
	GEngine->AddOnScreenDebugMessage(1, 30.0f, FColor::Yellow, Text);
}

EDoorOccupant ADoorRangeGameMode::RollOccupant() const
{
	const UDoorRangeSettings* Cfg = GetSettings();
	const float Roll = FMath::FRand();

	if (Roll < Cfg->HostileShare)
	{
		return EDoorOccupant::Hostile;
	}
	if (Roll < Cfg->HostileShare + Cfg->EmptyShare)
	{
		return EDoorOccupant::Empty;
	}
	return EDoorOccupant::Friendly;
}

ADoorSlot* ADoorRangeGameMode::PickAvailableSlot() const
{
	TArray<ADoorSlot*> Available;
	for (ADoorSlot* Slot : Slots)
	{
		if (Slot && Slot->IsAvailable())
		{
			Available.Add(Slot);
		}
	}

	if (Available.IsEmpty())
	{
		return nullptr;
	}

	return Available[FMath::RandRange(0, Available.Num() - 1)];
}
