// CYB3RGUN THEGAME. Game mode running the flight range: one countdown, one run, one score (D-077).

#include "FlightRangeGameMode.h"
#include "FlightRangeHUD.h"
#include "FlightRangeSettings.h"
#include "FlightTarget.h"
#include "FlightTargetDefinition.h"
#include "LogoCrosshairWidget.h"
#include "ShooterCharacter.h"
#include "ShooterWeaponHolder.h"
#include "StyleHUDWidget.h"
#include "StyleScoringComponent.h"
#include "StyleSettings.h"
#include "WeaponDefinition.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogFlightRange, Log, All);

AFlightRangeGameMode::AFlightRangeGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	RangeHUDClass = UFlightRangeHUD::StaticClass();
}

const UFlightRangeSettings* AFlightRangeGameMode::GetSettings() const
{
	return Settings ? Settings.Get() : GetDefault<UFlightRangeSettings>();
}

void AFlightRangeGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	// the countdown runs on game time, so Overclock's slowed world would lengthen the round: it is locked here through the
	// style values' own switch, and everything else of the style system stays as the scenario or the project sets it
	const UStyleSettings* Base = UStyleSettings::Get(this);
	RoundStyle = DuplicateObject<UStyleSettings>(Base, this, TEXT("FlightRangeStyle"));
	RoundStyle->bOverclockAllowed = false;
	UE_LOG(LogFlightRange, Log, TEXT("Style values from %s, Overclock locked"), *GetNameSafe(Base));
}

void AFlightRangeGameMode::BeginPlay()
{
	Super::BeginPlay();

	// a director placed in the level wins, so a level can tune it; otherwise the range brings its own
	for (TActorIterator<AFlightSpawnDirector> It(GetWorld()); It; ++It)
	{
		Director = *It;
		break;
	}
	if (!Director)
	{
		Director = GetWorld()->SpawnActor<AFlightSpawnDirector>();
	}
	if (Director)
	{
		Director->OnTargetLaunched.AddUObject(this, &AFlightRangeGameMode::HandleTargetLaunched);
	}

	UE_LOG(LogFlightRange, Log, TEXT("Flight range ready, settings %s, round %.0f s, director %s"), *GetNameSafe(GetSettings()), GetSettings()->RoundSeconds, *GetNameSafe(Director));
}

void AFlightRangeGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(StartTimer);
	Super::EndPlay(EndPlayReason);
}

void AFlightRangeGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);

	CreateHUD(NewPlayer);

	// the pawn finishes its own BeginPlay first
	GetWorldTimerManager().SetTimerForNextTick(this, &AFlightRangeGameMode::SetupPlayer);
}

void AFlightRangeGameMode::CreateHUD(APlayerController* Player)
{
	if (!Player || !Player->IsLocalController())
	{
		return;
	}

	// the style HUD and the crosshair are the same in every scenario
	if (!StyleHUD)
	{
		StyleHUD = UStyleHUDWidget::CreateFor(Player);
		Crosshair = ULogoCrosshairWidget::CreateFor(Player);
	}

	if (!HUD && RangeHUDClass)
	{
		HUD = CreateWidget<UFlightRangeHUD>(Player, RangeHUDClass);
		if (HUD)
		{
			HUD->AddToViewport(1);
		}
	}
}

void AFlightRangeGameMode::SetupPlayer()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	if (!Pawn)
	{
		UE_LOG(LogFlightRange, Warning, TEXT("No player pawn to set up"));
		return;
	}

	// the player stands still and turns (D-078): look input stays, movement and jumping go
	if (ACharacter* Character = Cast<ACharacter>(Pawn))
	{
		Character->GetCharacterMovement()->StopMovementImmediately();
		Character->GetCharacterMovement()->DisableMovement();
	}
	PC->SetIgnoreMoveInput(true);

	const UFlightRangeSettings* Cfg = GetSettings();
	if (IShooterWeaponHolder* Holder = Cast<IShooterWeaponHolder>(Pawn))
	{
		// the first weapon comes up in hand (D-080), the rest follow in the switch order
		for (const UWeaponDefinition* Weapon : Cfg->Loadout)
		{
			if (Weapon)
			{
				const UWeaponDefinition* Granted = MakeRoundWeapon(Weapon);
				Holder->AddWeaponDefinition(Granted);
				UE_LOG(LogFlightRange, Log, TEXT("Granted weapon %s, %d rounds, %.2f s reload"), *GetNameSafe(Weapon), Granted->MagazineSize, Granted->ReloadSeconds);
			}
		}
	}

	// the field opens where the player stands and looks, on the ground under its feet
	FVector Feet = Pawn->GetActorLocation();
	if (const ACharacter* Character = Cast<ACharacter>(Pawn))
	{
		Feet.Z -= Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	}
	FieldTransform = FTransform(FRotator(0.0f, Pawn->GetActorRotation().Yaw, 0.0f), Feet);
	FRotator ViewRotation;
	PC->GetPlayerViewPoint(ShooterLocation, ViewRotation);

	SetRoundState(EFlightRoundState::GetReady);
	ReadyAt = GetWorld()->GetTimeSeconds() + Cfg->StartDelaySeconds;
	if (bAutoStart)
	{
		GetWorldTimerManager().SetTimer(StartTimer, this, &AFlightRangeGameMode::StartRound, FMath::Max(Cfg->StartDelaySeconds, 0.01f), false);
	}
}

const UWeaponDefinition* AFlightRangeGameMode::MakeRoundWeapon(const UWeaponDefinition* Weapon)
{
	const FFlightWeaponOverride* Override = GetSettings()->FindWeaponOverride(Weapon);
	if (!Override || (Override->MagazineSize <= 0 && Override->ReloadSeconds <= 0.0f))
	{
		return Weapon;
	}

	// a copy for this round carries the mode's handling; the shared definition every other mode uses stays untouched
	UWeaponDefinition* Copy = DuplicateObject<UWeaponDefinition>(Weapon, this, MakeUniqueObjectName(this, UWeaponDefinition::StaticClass(), FName(Weapon->GetName() + TEXT("_FlightRange"))));
	if (Override->MagazineSize > 0)
	{
		Copy->MagazineSize = Override->MagazineSize;
	}
	if (Override->ReloadSeconds > 0.0f)
	{
		Copy->ReloadSeconds = Override->ReloadSeconds;
	}
	RoundWeapons.Add(Copy);
	return Copy;
}

void AFlightRangeGameMode::StartRound()
{
	const UFlightRangeSettings* Cfg = GetSettings();
	GetWorldTimerManager().ClearTimer(StartTimer);

	Score = 0;
	Stats = FFlightRangeStats();
	TimeLeft = Cfg->RoundSeconds;
	LastBroadcastSeconds = -1;

	if (UStyleScoringComponent* Style = UStyleScoringComponent::Get(UGameplayStatics::GetPlayerController(GetWorld(), 0)))
	{
		Style->ResetRun();
	}
	if (Crosshair)
	{
		Crosshair->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	if (Director)
	{
		Director->StartLaunching(Cfg, FieldTransform, ShooterLocation);
	}

	OnScoreChanged.Broadcast(Score, 0);
	SetRoundState(EFlightRoundState::Running);
	PlaySound2D(Cfg->StartSound);
	UE_LOG(LogFlightRange, Log, TEXT("Round starts: %.0f s"), TimeLeft);
}

void AFlightRangeGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (RoundState != EFlightRoundState::Running)
	{
		return;
	}

	// the countdown runs on game time, so a slowed world slows it with the targets
	TimeLeft = FMath::Max(TimeLeft - DeltaSeconds, 0.0f);
	const int32 Seconds = GetSecondsLeft();
	if (Seconds != LastBroadcastSeconds)
	{
		LastBroadcastSeconds = Seconds;
		OnTimeChanged.Broadcast(Seconds);
		const UFlightRangeSettings* Cfg = GetSettings();
		if (Seconds > 0 && Seconds <= Cfg->LastSecondsCount)
		{
			PlaySound2D(Cfg->LastSecondsSound);
		}
	}

	if (TimeLeft <= 0.0f)
	{
		FinishRound();
	}
}

int32 AFlightRangeGameMode::GetSecondsLeft() const
{
	return RoundState == EFlightRoundState::GetReady ? FMath::CeilToInt(GetSettings()->RoundSeconds) : FMath::CeilToInt(TimeLeft);
}

float AFlightRangeGameMode::GetReadySecondsLeft() const
{
	return RoundState == EFlightRoundState::GetReady ? FMath::Max(static_cast<float>(ReadyAt - GetWorld()->GetTimeSeconds()), 0.0f) : 0.0f;
}

FFlightRangeStats AFlightRangeGameMode::GetStats() const
{
	FFlightRangeStats Current = Stats;
	Current.FinalScore = Score;

	// shots come from the style record, which sees every trigger pull and how it resolved
	if (const UStyleScoringComponent* Style = UStyleScoringComponent::Get(UGameplayStatics::GetPlayerController(GetWorld(), 0)))
	{
		Current.ShotsFired = Style->GetStats().ShotsFired;
		Current.Misses = Style->GetStats().Misses;
		Current.Accuracy = Style->GetStats().GetAccuracy();
	}
	if (Director)
	{
		Current.LaunchesLeft = Director->GetLaunchCount(EFlightSource::Left);
		Current.LaunchesRight = Director->GetLaunchCount(EFlightSource::Right);
		Current.LaunchesCover = Director->GetLaunchCount(EFlightSource::Cover);
	}
	return Current;
}

void AFlightRangeGameMode::FinishRound()
{
	if (RoundState == EFlightRoundState::Finished)
	{
		return;
	}

	// finished before the targets still in the air are retired, so they do not count as escaped
	TimeLeft = 0.0f;
	RoundState = EFlightRoundState::Finished;
	if (Director)
	{
		Director->StopLaunching(true);
	}

	// the round is over for the player too: the trigger lets go and the input stops
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (AShooterCharacter* Shooter = PC ? Cast<AShooterCharacter>(PC->GetPawn()) : nullptr)
	{
		Shooter->DoStopFiring();
		Shooter->DisableInput(PC);
	}
	if (Crosshair)
	{
		Crosshair->SetVisibility(ESlateVisibility::Collapsed);
	}

	Stats = GetStats();
	OnRoundStateChanged.Broadcast(RoundState);
	OnTimeChanged.Broadcast(0);
	PlaySound2D(GetSettings()->EndSound);

	UE_LOG(LogFlightRange, Log, TEXT("Round complete: score %d, hits %d of %d launched, escaped %d, shots %d, misses %d, accuracy %.0f%%, best hit %d, launches left %d right %d cover %d"),
		Stats.FinalScore, Stats.TargetsHit, Stats.TargetsLaunched, Stats.TargetsEscaped, Stats.ShotsFired, Stats.Misses, Stats.Accuracy * 100.0f,
		Stats.BestHit, Stats.LaunchesLeft, Stats.LaunchesRight, Stats.LaunchesCover);
	OnRoundFinished.Broadcast(Stats);
}

void AFlightRangeGameMode::SetRoundState(EFlightRoundState NewState)
{
	RoundState = NewState;
	OnRoundStateChanged.Broadcast(RoundState);
}

void AFlightRangeGameMode::HandleTargetLaunched(AFlightTarget* Target, EFlightSource Source)
{
	if (!Target)
	{
		return;
	}
	++Stats.TargetsLaunched;
	Target->OnHit.AddUObject(this, &AFlightRangeGameMode::HandleTargetHit);
	Target->OnDone.AddUObject(this, &AFlightRangeGameMode::HandleTargetDone);
}

void AFlightRangeGameMode::HandleTargetHit(AFlightTarget* Target, EHitZone Zone, AController* InstigatedBy)
{
	if (RoundState != EFlightRoundState::Running || !Target || !Target->GetDefinition())
	{
		return;
	}

	const int32 Points = GetSettings()->ComputeScore(Target->GetDefinition()->BaseScore, Target->GetShooterDistance(), Target->GetFlightSpeed(), Target->GetSize());
	++Stats.TargetsHit;
	Stats.BestHit = FMath::Max(Stats.BestHit, Points);
	Score += Points;

	UE_LOG(LogFlightRange, Log, TEXT("Score +%d -> %d (%s, %s, path %s, %.0f cm, %.0f cm/s, size %.2f)"), Points, Score, *GetNameSafe(Target->GetDefinition()), Zone == EHitZone::Head ? TEXT("head") : TEXT("body"),
		*StaticEnum<EFlightPathType>()->GetNameStringByValue(static_cast<int64>(Target->GetPathType())), Target->GetShooterDistance(), Target->GetFlightSpeed(), Target->GetSize());

	OnScoreChanged.Broadcast(Score, Points);
	OnTargetScored.Broadcast(Points, Target->GetActorLocation());
}

void AFlightRangeGameMode::HandleTargetDone(AFlightTarget* Target, bool bWasHit)
{
	if (RoundState == EFlightRoundState::Running && !bWasHit)
	{
		++Stats.TargetsEscaped;
	}
}

void AFlightRangeGameMode::PlaySound2D(USoundBase* Sound) const
{
	if (Sound)
	{
		UGameplayStatics::PlaySound2D(this, Sound);
	}
}
