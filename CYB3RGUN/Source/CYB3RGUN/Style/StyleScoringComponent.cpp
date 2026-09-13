// CYB3RGUN THEGAME. The style record of one player: meter, combo, points and run statistics.

#include "StyleScoringComponent.h"
#include "StyleSettings.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogStyle, Log, All);

UStyleScoringComponent::UStyleScoringComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

UStyleScoringComponent* UStyleScoringComponent::Get(AActor* PawnOrController)
{
	if (const APawn* Pawn = Cast<APawn>(PawnOrController))
	{
		return ForController(Pawn->GetController());
	}
	return ForController(Cast<AController>(PawnOrController));
}

UStyleScoringComponent* UStyleScoringComponent::ForController(AController* Controller)
{
	APlayerController* PC = Cast<APlayerController>(Controller);
	if (!PC || !PC->IsLocalController())
	{
		return nullptr;
	}

	if (UStyleScoringComponent* Existing = PC->FindComponentByClass<UStyleScoringComponent>())
	{
		return Existing;
	}

	UStyleScoringComponent* Created = NewObject<UStyleScoringComponent>(PC, TEXT("StyleScoring"));
	Created->RegisterComponent();
	Created->LastCleanTime = Created->Now();
	return Created;
}

const UStyleSettings* UStyleScoringComponent::GetSettings() const
{
	return UStyleSettings::Get(this);
}

double UStyleScoringComponent::Now() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetTimeSeconds() : 0.0;
}

void UStyleScoringComponent::ResetRun()
{
	Stats = FStyleRunStats();
	Meter = 0.0f;
	Combo = 0;
	LastCleanTime = Now();
	LastHitTarget.Reset();
	LastHitTime = -1000.0;
	ResolutionDepth = 0;
	bResolutionHitTarget = false;
	bResolutionPenalty = false;
	ResolvedTargets.Reset();

	UE_LOG(LogStyle, Log, TEXT("Style run reset"));
	OnComboChanged.Broadcast(Combo, GetMultiplier());
}

void UStyleScoringComponent::RecordShotFired()
{
	++Stats.ShotsFired;
	OnShotFired.Broadcast(Stats.ShotsFired);
}

void UStyleScoringComponent::BeginShotResolution()
{
	if (ResolutionDepth++ == 0)
	{
		bResolutionHitTarget = false;
		bResolutionPenalty = false;
		ResolvedTargets.Reset();
	}
}

void UStyleScoringComponent::EndShotResolution()
{
	if (ResolutionDepth <= 0 || --ResolutionDepth > 0)
	{
		return;
	}

	// the best zone of each target pays once the whole shot is in, every pellet of it
	PayZoneBonuses();

	if (bResolutionHitTarget)
	{
		++Stats.ShotsHit;
	}
	else if (!bResolutionPenalty)
	{
		// the shot landed on nothing it could score on
		RecordMiss();
	}
	ResolvedTargets.Reset();
}

void UStyleScoringComponent::RecordTargetHit(AActor* Target, bool bHeadshot, bool bKill)
{
	RecordTargetHit(Target, bHeadshot ? EHitZone::Head : EHitZone::None, bKill);
}

void UStyleScoringComponent::RecordTargetHit(AActor* Target, EHitZone Zone, bool bKill)
{
	if (!Target)
	{
		return;
	}

	const bool bStandalone = ResolutionDepth == 0;
	if (bStandalone)
	{
		BeginShotResolution();
	}

	const UStyleSettings* Settings = GetSettings();
	bResolutionHitTarget = true;
	LastCleanTime = Now();

	// pellets of one shot on one target count once, a later pellet can still bring it down or land in a better zone
	bool bFirst = false;
	FResolvedTarget& Entry = FindOrAddTarget(Target, bFirst);
	if (bKill && !Entry.bKilled)
	{
		Entry.bKilled = true;
		ScoreKill(Target);
	}
	else if (bFirst && !bKill)
	{
		AddMeter(Settings->MeterPerHit);
		Award(EStyleEvent::Hit, Settings->HitPoints, Target, true);
	}
	NoteZone(Entry, Zone);

	if (bStandalone)
	{
		EndShotResolution();
	}
}

void UStyleScoringComponent::RecordDisarm(AActor* Target, bool bClean)
{
	if (!Target)
	{
		return;
	}

	const bool bStandalone = ResolutionDepth == 0;
	if (bStandalone)
	{
		BeginShotResolution();
	}

	// a disarm is a clean hit, the shot that did it is no miss
	const UStyleSettings* Settings = GetSettings();
	bResolutionHitTarget = true;
	LastCleanTime = Now();

	bool bFirst = false;
	FResolvedTarget& Entry = FindOrAddTarget(Target, bFirst);
	if (!Entry.bDisarmed)
	{
		Entry.bDisarmed = true;

		// the zone score of the weapon, or of the weapon arm, is the disarm's bonus and pays above a kill (D-050)
		const FHitZoneRule* Rule = UHitZoneSettings::Get()->FindRule(bClean ? EHitZone::Weapon : EHitZone::WeaponArm);
		++Stats.Disarms;
		AddMeter(Settings->MeterPerDisarm);
		AwardAt(EStyleEvent::Disarm, Rule ? Rule->Score : 0, Target, Entry.Multiplier);

		// the threat is gone as surely as after a kill, so a disarm carries the combo on
		SetCombo(Combo + 1);
	}

	if (bStandalone)
	{
		EndShotResolution();
	}
}

UStyleScoringComponent::FResolvedTarget& UStyleScoringComponent::FindOrAddTarget(AActor* Target, bool& bOutAdded)
{
	if (FResolvedTarget* Seen = ResolvedTargets.FindByPredicate([Target](const FResolvedTarget& Entry) { return Entry.Target.Get() == Target; }))
	{
		bOutAdded = false;
		return *Seen;
	}

	bOutAdded = true;
	FResolvedTarget& Entry = ResolvedTargets.AddDefaulted_GetRef();
	Entry.Target = Target;
	Entry.Multiplier = GetMultiplier();

	// a second hit on the same target inside the window is a controlled pair, worth more than two separate hits
	const UStyleSettings* Settings = GetSettings();
	const double Time = Now();
	if (LastHitTarget.Get() == Target && Time - LastHitTime <= Settings->ControlledPairWindow)
	{
		++Stats.ControlledPairs;
		AddMeter(Settings->MeterPerControlledPair);
		Award(EStyleEvent::ControlledPair, Settings->ControlledPairBonus, Target, true);
	}
	LastHitTarget = Target;
	LastHitTime = Time;
	return Entry;
}

void UStyleScoringComponent::NoteZone(FResolvedTarget& Entry, EHitZone Zone) const
{
	// a weapon arm that did not disarm is an arm like the other, its own score belongs to the disarm; the weapon zone pays through RecordDisarm
	const EHitZone BonusZone = Zone == EHitZone::WeaponArm ? EHitZone::OffArm : Zone;
	const bool bHasBonus = BonusZone == EHitZone::Head || BonusZone == EHitZone::Leg || BonusZone == EHitZone::OffArm;

	// the head bonus crowns a kill, as the headshot always has
	if (!bHasBonus || (BonusZone == EHitZone::Head && !Entry.bKilled))
	{
		return;
	}

	const FHitZoneRule* Rule = UHitZoneSettings::Get()->FindRule(BonusZone);
	const int32 Points = Rule ? Rule->Score : 0;
	if (Entry.BonusZone == EHitZone::None || Points > Entry.BonusPoints)
	{
		Entry.BonusZone = BonusZone;
		Entry.BonusPoints = Points;
	}
}

void UStyleScoringComponent::PayZoneBonuses()
{
	const UStyleSettings* Settings = GetSettings();

	// taken out first, a listener of the awards must not see the list change under it
	const TArray<FResolvedTarget> Resolved = MoveTemp(ResolvedTargets);
	ResolvedTargets.Reset();

	for (const FResolvedTarget& Entry : Resolved)
	{
		AActor* Target = Entry.Target.Get();
		switch (Entry.BonusZone)
		{
		case EHitZone::Head:
			++Stats.Headshots;
			AddMeter(Settings->MeterPerHeadshot);
			AwardAt(EStyleEvent::Headshot, Entry.BonusPoints, Target, Entry.Multiplier);
			break;
		case EHitZone::Leg:
			++Stats.LegShots;
			AwardAt(EStyleEvent::LegShot, Entry.BonusPoints, Target, Entry.Multiplier);
			break;
		case EHitZone::OffArm:
			++Stats.ArmShots;
			AwardAt(EStyleEvent::ArmShot, Entry.BonusPoints, Target, Entry.Multiplier);
			break;
		default:
			break;
		}
	}
}

void UStyleScoringComponent::ScoreKill(AActor* Target)
{
	const UStyleSettings* Settings = GetSettings();

	++Stats.Kills;
	AddMeter(Settings->MeterPerKill);
	Award(EStyleEvent::Kill, Settings->KillPoints, Target, true);

	// the kill itself is paid at the multiplier it was earned under, the next one gets the raised one
	SetCombo(Combo + 1);
}

void UStyleScoringComponent::RecordNonTargetHit(AActor* Victim)
{
	const bool bStandalone = ResolutionDepth == 0;
	if (bStandalone)
	{
		BeginShotResolution();
	}

	const UStyleSettings* Settings = GetSettings();
	++Stats.Penalties;
	bResolutionPenalty = true;

	// innocents are never targets: the meter collapses and the combo is gone (D-044)
	Meter = FMath::Clamp(Settings->MeterAfterPenalty, 0.0f, Settings->MeterMax);
	SetCombo(0);
	LastHitTarget.Reset();
	Award(EStyleEvent::Penalty, -Settings->NonTargetPenalty, Victim, false);

	if (bStandalone)
	{
		EndShotResolution();
	}
}

void UStyleScoringComponent::RecordRescue(AActor* FreedFrom)
{
	const UStyleSettings* Settings = GetSettings();
	++Stats.Rescues;
	LastCleanTime = Now();
	AddMeter(Settings->MeterPerRescue);
	Award(EStyleEvent::Rescue, Settings->RescueBonus, FreedFrom, true);
}

void UStyleScoringComponent::RecordMiss()
{
	const UStyleSettings* Settings = GetSettings();
	++Stats.Misses;
	SetCombo(FMath::Max(Combo - Settings->MissComboLoss, 0));
	AddMeter(-Settings->MeterPerMiss);
	Award(EStyleEvent::Miss, 0, nullptr, false);
}

void UStyleScoringComponent::Award(EStyleEvent Event, int32 BasePoints, AActor* Target, bool bMultiplied)
{
	AwardAt(Event, BasePoints, Target, bMultiplied ? GetMultiplier() : 1.0f);
}

void UStyleScoringComponent::AwardAt(EStyleEvent Event, int32 BasePoints, AActor* Target, float Multiplier)
{
	const int32 Points = FMath::RoundToInt(BasePoints * Multiplier);
	Stats.StylePoints += Points;

	UE_LOG(LogStyle, Log, TEXT("%s %+d at x%.1f on %s: style %d, meter %.0f, combo %d"),
		*StaticEnum<EStyleEvent>()->GetNameStringByValue(static_cast<int64>(Event)), Points, Multiplier, *GetNameSafe(Target), Stats.StylePoints, Meter, Combo);

	OnStyleEvent.Broadcast(Event, Points, Target);
}

void UStyleScoringComponent::AddMeter(float Delta)
{
	Meter = FMath::Clamp(Meter + Delta, 0.0f, GetSettings()->MeterMax);
	Stats.PeakMeter = FMath::Max(Stats.PeakMeter, Meter);
}

void UStyleScoringComponent::SetCombo(int32 NewCombo)
{
	if (NewCombo == Combo)
	{
		return;
	}
	Combo = NewCombo;
	Stats.BestCombo = FMath::Max(Stats.BestCombo, Combo);
	OnComboChanged.Broadcast(Combo, GetMultiplier());
}

float UStyleScoringComponent::GetMultiplier() const
{
	const UStyleSettings* Settings = GetSettings();
	const int32 Steps = Combo / FMath::Max(Settings->KillsPerMultiplierStep, 1);
	return FMath::Min(1.0f + Steps * Settings->MultiplierStep, Settings->MaxMultiplier);
}

float UStyleScoringComponent::GetMeterFraction() const
{
	return Meter / FMath::Max(GetSettings()->MeterMax, 1.0f);
}

FText UStyleScoringComponent::GetRankLabel() const
{
	return GetSettings()->GetRankLabel(Meter);
}

void UStyleScoringComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// the meter holds for a moment after a clean action, then drains while nothing clean happens
	const UStyleSettings* Settings = GetSettings();
	if (Meter > 0.0f && Now() - LastCleanTime > Settings->MeterDecayDelay)
	{
		Meter = FMath::Max(Meter - Settings->MeterDecayPerSecond * DeltaTime, 0.0f);
	}
}

void UStyleScoringComponent::LogStatus() const
{
	UE_LOG(LogStyle, Display, TEXT("Style %d, meter %.0f %s, combo %d at x%.1f, best combo %d, shots %d, hits %d, accuracy %.0f%%, kills %d, headshots %d, disarms %d, leg shots %d, arm shots %d, pairs %d, rescues %d, penalties %d, misses %d"),
		Stats.StylePoints, Meter, *GetRankLabel().ToString(), Combo, GetMultiplier(), Stats.BestCombo, Stats.ShotsFired, Stats.ShotsHit,
		Stats.GetAccuracy() * 100.0f, Stats.Kills, Stats.Headshots, Stats.Disarms, Stats.LegShots, Stats.ArmShots, Stats.ControlledPairs, Stats.Rescues, Stats.Penalties, Stats.Misses);
}

#if !UE_BUILD_SHIPPING

static FAutoConsoleCommandWithWorld GStyleStatusCommand(
	TEXT("Style.Status"),
	TEXT("Logs the local player's style meter, combo and run statistics."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		if (UStyleScoringComponent* Style = UStyleScoringComponent::ForController(World ? World->GetFirstPlayerController() : nullptr))
		{
			Style->LogStatus();
		}
	}));

static FAutoConsoleCommandWithWorld GStyleResetCommand(
	TEXT("Style.Reset"),
	TEXT("Starts a new style run for the local player."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		if (UStyleScoringComponent* Style = UStyleScoringComponent::ForController(World ? World->GetFirstPlayerController() : nullptr))
		{
			Style->ResetRun();
		}
	}));

#endif
