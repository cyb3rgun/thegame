// CYB3RGUN THEGAME. Game mode running the door range.

#include "DoorRangeGameMode.h"
#include "DoorSlot.h"
#include "DoorRangeSettings.h"
#include "DoorRangeHUD.h"
#include "ShooterWeapon.h"
#include "ShooterWeaponHolder.h"
#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
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
	return GetSettings()->GetWaveCount();
}

void ADoorRangeGameMode::BeginPlay()
{
	Super::BeginPlay();

	CollectSlots();

	UE_LOG(LogDoorRange, Log, TEXT("Door range ready with %d slots, settings %s"), Slots.Num(), *GetNameSafe(GetSettings()));

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

	CreateHUD(NewPlayer);

	// hand out the weapon on the next tick so the pawn has finished its own BeginPlay
	GetWorldTimerManager().SetTimerForNextTick(this, &ADoorRangeGameMode::GrantStartingWeapon);
}

void ADoorRangeGameMode::CreateHUD(APlayerController* Player)
{
	if (HUD || !RangeHUDClass || !Player || !Player->IsLocalController())
	{
		return;
	}

	HUD = CreateWidget<UDoorRangeHUD>(Player, RangeHUDClass);
	if (HUD)
	{
		HUD->AddToViewport(1);
		UE_LOG(LogDoorRange, Log, TEXT("Door range HUD created: %s"), *GetNameSafe(HUD));
	}
	else
	{
		UE_LOG(LogDoorRange, Warning, TEXT("Could not create the door range HUD from %s"), *GetNameSafe(RangeHUDClass));
	}
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
		Slot->OnSlotDrawn.AddUniqueDynamic(this, &ADoorRangeGameMode::HandleSlotDrawn);
	}
}

void ADoorRangeGameMode::StartRange()
{
	if (Slots.IsEmpty())
	{
		UE_LOG(LogDoorRange, Warning, TEXT("Cannot start the door range, no door slots in the level"));
		return;
	}

	GetWorldTimerManager().ClearTimer(OpenTimer);
	GetWorldTimerManager().ClearTimer(WaveTimer);

	for (ADoorSlot* Slot : Slots)
	{
		Slot->ForceClose(false);
	}

	Score = 0;
	OpenDoors = 0;
	Stats = FDoorRangeStats();
	bRangeComplete = false;
	bRangeActive = true;

	OnScoreChanged.Broadcast(Score, 0);
	StartWave(1);
}

void ADoorRangeGameMode::StartWave(int32 WaveNumber)
{
	const FDoorWaveSettings& Wave = GetSettings()->GetWave(WaveNumber);

	CurrentWave = WaveNumber;
	BuildWaveQueue(Wave);

	UE_LOG(LogDoorRange, Log, TEXT("Wave %d of %d starts: %d openings, %d hostiles, exposure %.2fs, cadence %.2fs"),
		CurrentWave, GetWaveCount(), Wave.Openings, HostilesTotalThisWave, Wave.ExposureWindow, Wave.TimeBetweenOpenings);

	OnWaveChanged.Broadcast(CurrentWave, GetWaveCount());
	OnHostilesRemainingChanged.Broadcast(HostilesRemainingThisWave, HostilesTotalThisWave);
	OnRangeEvent.Broadcast(EDoorRangeEvent::WaveStarted, CurrentWave, nullptr);

	GetWorldTimerManager().SetTimer(OpenTimer, this, &ADoorRangeGameMode::TryOpenDoor, FMath::Max(Wave.TimeBetweenOpenings, 0.05f), true, 0.0f);
}

void ADoorRangeGameMode::BuildWaveQueue(const FDoorWaveSettings& Wave)
{
	const int32 Openings = FMath::Max(Wave.Openings, 1);
	const int32 Hostiles = FMath::Clamp(FMath::RoundToInt(Openings * Wave.HostileShare), 0, Openings);
	const int32 Empties = FMath::Clamp(FMath::RoundToInt(Openings * Wave.EmptyShare), 0, Openings - Hostiles);
	const int32 Friendlies = Openings - Hostiles - Empties;

	WaveQueue.Reset(Openings);
	for (int32 i = 0; i < Hostiles; ++i)
	{
		WaveQueue.Add(EDoorOccupant::Hostile);
	}
	for (int32 i = 0; i < Empties; ++i)
	{
		WaveQueue.Add(EDoorOccupant::Empty);
	}
	for (int32 i = 0; i < Friendlies; ++i)
	{
		WaveQueue.Add(EDoorOccupant::Friendly);
	}

	// Fisher-Yates shuffle
	for (int32 i = WaveQueue.Num() - 1; i > 0; --i)
	{
		WaveQueue.Swap(i, FMath::RandRange(0, i));
	}

	HostilesTotalThisWave = Hostiles;
	HostilesRemainingThisWave = Hostiles;
	Stats.HostilesTotal += Hostiles;
}

void ADoorRangeGameMode::TryOpenDoor()
{
	const UDoorRangeSettings* Cfg = GetSettings();
	const FDoorWaveSettings& Wave = Cfg->GetWave(CurrentWave);

	if (WaveQueue.IsEmpty())
	{
		// all openings of this wave are done, wait for the last doors to close
		GetWorldTimerManager().ClearTimer(OpenTimer);
		if (OpenDoors == 0)
		{
			EndWave();
		}
		return;
	}

	if (OpenDoors >= Wave.VisibleDoors)
	{
		return;
	}

	ADoorSlot* Slot = PickAvailableSlot();
	if (!Slot)
	{
		return;
	}

	FDoorOpenParams Params;
	Params.Occupant = WaveQueue.Pop();
	Params.OpenDuration = Cfg->OpenDuration;
	Params.CloseDuration = Cfg->CloseDuration;
	Params.ExposureWindow = Wave.ExposureWindow;
	Params.TelegraphDuration = Wave.TelegraphDuration;
	Params.DoorSound = Cfg->DoorSound;
	Params.DoorPitch = Cfg->DoorPitch;
	Params.TelegraphSound = Cfg->TelegraphSound;
	Params.TelegraphPitch = Cfg->TelegraphPitch;
	Params.DrawSound = Cfg->DrawSound;
	Params.DrawPitch = Cfg->DrawPitch;

	switch (Params.Occupant)
	{
	case EDoorOccupant::Hostile:
		Params.OccupantMaterial = Cfg->HostileMaterial;
		break;
	case EDoorOccupant::Friendly:
		Params.OccupantMaterial = Cfg->FriendlyMaterial;
		break;
	default:
		break;
	}

	Slot->Open(Params);
	++OpenDoors;

	UE_LOG(LogDoorRange, Verbose, TEXT("%s opens with occupant %d (%d left in wave %d)"), *Slot->GetName(), static_cast<int32>(Params.Occupant), WaveQueue.Num(), CurrentWave);
}

void ADoorRangeGameMode::HandleSlotDrawn(ADoorSlot* Slot)
{
	OnRangeEvent.Broadcast(EDoorRangeEvent::HostileDrawn, 0, Slot);
}

void ADoorRangeGameMode::HandleSlotHit(ADoorSlot* Slot, EDoorOccupant Occupant, float ExposureFraction)
{
	const UDoorRangeSettings* Cfg = GetSettings();

	switch (Occupant)
	{
	case EDoorOccupant::Hostile:
	{
		const int32 DrawBonus = FMath::RoundToInt(Cfg->DrawBonusMax * FMath::Clamp(ExposureFraction, 0.0f, 1.0f));
		const int32 Delta = Cfg->HitHostileScore + DrawBonus;
		++Stats.HostilesHit;
		HostilesRemainingThisWave = FMath::Max(HostilesRemainingThisWave - 1, 0);
		AddScore(Delta, FString::Printf(TEXT("Hostile hit on %s, draw bonus %d, exposure %.2f, state %d"), *Slot->GetName(), DrawBonus, ExposureFraction, static_cast<int32>(Slot->GetDoorState())));
		OnHostilesRemainingChanged.Broadcast(HostilesRemainingThisWave, HostilesTotalThisWave);
		OnRangeEvent.Broadcast(EDoorRangeEvent::HostileHit, Delta, Slot);
		PlayEventSound(EDoorRangeEvent::HostileHit, Slot);
		break;
	}
	case EDoorOccupant::Friendly:
	{
		const int32 Delta = -Cfg->HitFriendlyPenalty;
		++Stats.FriendliesHit;
		AddScore(Delta, FString::Printf(TEXT("Friendly hit on %s"), *Slot->GetName()));
		OnRangeEvent.Broadcast(EDoorRangeEvent::FriendlyHit, Delta, Slot);
		PlayEventSound(EDoorRangeEvent::FriendlyHit, Slot);
		break;
	}
	default:
		break;
	}
}

void ADoorRangeGameMode::HandleSlotClosed(ADoorSlot* Slot, EDoorOccupant Occupant, bool bWasHit)
{
	OpenDoors = FMath::Max(OpenDoors - 1, 0);

	if (bRangeActive && Occupant == EDoorOccupant::Hostile && !bWasHit)
	{
		const int32 Delta = -GetSettings()->MissedHostilePenalty;
		++Stats.HostilesEscaped;
		HostilesRemainingThisWave = FMath::Max(HostilesRemainingThisWave - 1, 0);
		AddScore(Delta, FString::Printf(TEXT("Hostile escaped from %s"), *Slot->GetName()));
		OnHostilesRemainingChanged.Broadcast(HostilesRemainingThisWave, HostilesTotalThisWave);
		OnRangeEvent.Broadcast(EDoorRangeEvent::HostileEscaped, Delta, Slot);
		PlayEventSound(EDoorRangeEvent::HostileEscaped, Slot);
	}

	if (bRangeActive && WaveQueue.IsEmpty() && OpenDoors == 0)
	{
		EndWave();
	}
}

void ADoorRangeGameMode::EndWave()
{
	GetWorldTimerManager().ClearTimer(OpenTimer);
	Stats.WavesPlayed = CurrentWave;

	UE_LOG(LogDoorRange, Log, TEXT("Wave %d ends, score %d"), CurrentWave, Score);
	OnRangeEvent.Broadcast(EDoorRangeEvent::WaveEnded, CurrentWave, nullptr);

	if (CurrentWave >= GetWaveCount())
	{
		FinishRange();
		return;
	}

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
	Stats.FinalScore = Score;

	UE_LOG(LogDoorRange, Log, TEXT("Range complete: final score %d, hostiles hit %d of %d, escaped %d, friendlies hit %d, waves %d"),
		Stats.FinalScore, Stats.HostilesHit, Stats.HostilesTotal, Stats.HostilesEscaped, Stats.FriendliesHit, Stats.WavesPlayed);

	OnRangeEvent.Broadcast(EDoorRangeEvent::RangeFinished, Score, nullptr);
	OnRangeFinished.Broadcast(Stats);
}

void ADoorRangeGameMode::AddScore(int32 Delta, const FString& Reason)
{
	Score += Delta;

	UE_LOG(LogDoorRange, Log, TEXT("Score %+d -> %d (%s)"), Delta, Score, *Reason);
	OnScoreChanged.Broadcast(Score, Delta);
}

void ADoorRangeGameMode::PlayEventSound(EDoorRangeEvent Event, const ADoorSlot* Slot) const
{
	const UDoorRangeSettings* Cfg = GetSettings();
	USoundBase* Sound = nullptr;
	float Pitch = 1.0f;

	switch (Event)
	{
	case EDoorRangeEvent::HostileHit:
		Sound = Cfg->HostileHitSound;
		Pitch = Cfg->HostileHitPitch;
		break;
	case EDoorRangeEvent::FriendlyHit:
		Sound = Cfg->FriendlyHitSound;
		Pitch = Cfg->FriendlyHitPitch;
		break;
	case EDoorRangeEvent::HostileEscaped:
		Sound = Cfg->EscapeSound;
		Pitch = Cfg->EscapePitch;
		break;
	default:
		break;
	}

	if (Sound)
	{
		const FVector Location = Slot ? Slot->GetOccupantAimPoint() : FVector::ZeroVector;
		UGameplayStatics::PlaySoundAtLocation(this, Sound, Location, 1.0f, Pitch);
	}
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
