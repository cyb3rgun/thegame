// CYB3RGUN THEGAME. Game mode for rail routes.

#include "RailGameMode.h"
#include "RailAimComponent.h"
#include "RailCrosshairWidget.h"
#include "RailPawn.h"
#include "EncounterDirector.h"
#include "EncounterHUD.h"
#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

#define LOCTEXT_NAMESPACE "RailGameMode"

DEFINE_LOG_CATEGORY_STATIC(LogRailGame, Log, All);

ARailGameMode::ARailGameMode()
{
	DefaultPawnClass = ARailPawn::StaticClass();
	CrosshairWidgetClass = URailCrosshairWidget::StaticClass();
}

void ARailGameMode::BeginPlay()
{
	Super::BeginPlay();

	// one director serves every beat, each beat hands it a definition and spawn points
	FActorSpawnParameters Params;
	Params.Name = TEXT("RailEncounterDirector");
	Director = GetWorld()->SpawnActor<AEncounterDirector>(Params);
	if (Director)
	{
		Director->OnEncounterFinished.AddUniqueDynamic(this, &ARailGameMode::HandleEncounterFinished);
		Director->OnEnemyKilled.AddUniqueDynamic(this, &ARailGameMode::HandleEnemyKilled);
	}

	if (HUD)
	{
		HUD->BindDirector(Director);
	}
}

void ARailGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(SummaryTimer);
	Super::EndPlay(EndPlayReason);
}

APawn* ARailGameMode::SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot)
{
	// a rail pawn placed in the level is the player's ride, it keeps its own place on the route
	for (TActorIterator<ARailPawn> It(GetWorld()); It; ++It)
	{
		if (!It->GetController())
		{
			UE_LOG(LogRailGame, Log, TEXT("Player rides the placed %s"), *It->GetName());
			return *It;
		}
	}
	return Super::SpawnDefaultPawnFor_Implementation(NewPlayer, StartSpot);
}

void ARailGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);

	if (!NewPlayer || !NewPlayer->IsLocalController())
	{
		return;
	}

	BindRider(Cast<ARailPawn>(NewPlayer->GetPawn()));

	if (!HUD && EncounterHUDClass)
	{
		HUD = CreateWidget<UEncounterHUD>(NewPlayer, EncounterHUDClass);
		if (HUD)
		{
			HUD->AddToViewport(1);
			HUD->BindDirector(Director);
		}
	}

	if (!Crosshair && CrosshairWidgetClass)
	{
		Crosshair = CreateWidget<URailCrosshairWidget>(NewPlayer, CrosshairWidgetClass);
		if (Crosshair)
		{
			Crosshair->AddToViewport(2);
			Crosshair->BindAim(Rider ? Rider->GetAim() : nullptr);
		}
	}
}

void ARailGameMode::BindRider(ARailPawn* InRider)
{
	if (!InRider || InRider == Rider)
	{
		return;
	}

	Rider = InRider;
	Rider->OnBeatReached.AddUniqueDynamic(this, &ARailGameMode::HandleBeatReached);
	Rider->OnBeatCleared.AddUniqueDynamic(this, &ARailGameMode::HandleBeatCleared);
	Rider->OnRideFinished.AddUniqueDynamic(this, &ARailGameMode::HandleRideFinished);
	UE_LOG(LogRailGame, Log, TEXT("Rail route ready, rider %s on %s"), *Rider->GetName(), *GetNameSafe(Rider->GetTrack()));
}

void ARailGameMode::HandleBeatReached(ARailTrack* Track, int32 BeatIndex, const FRailBeat& Beat)
{
	GetWorldTimerManager().ClearTimer(SummaryTimer);

	// a hostage taker set piece: reveal it, the hold is released once it resolved
	if (Beat.HostageTaker)
	{
		ActiveHostageBeatIndex = BeatIndex;
		Beat.HostageTaker->OnResolved.AddUniqueDynamic(this, &ARailGameMode::HandleHostageResolved);
		Beat.HostageTaker->Activate();
		UE_LOG(LogRailGame, Log, TEXT("Beat %d %s reveals hostage taker %s"), BeatIndex, *Beat.Name.ToString(), *Beat.HostageTaker->GetName());
		return;
	}

	if (Beat.Encounter && Director)
	{
		ActiveBeatIndex = BeatIndex;
		Director->SetDefinition(Beat.Encounter);
		Director->SetSpawnPoints(Beat.SpawnPoints);
		Director->StartEncounter();
		UE_LOG(LogRailGame, Log, TEXT("Beat %d %s starts encounter %s with %d spawn points"), BeatIndex, *Beat.Name.ToString(), *GetNameSafe(Beat.Encounter), Beat.SpawnPoints.Num());
		return;
	}

	// a holding beat without an encounter has nothing to clear, later setpieces will release it through their tag
	if (Beat.bHoldUntilCleared && Rider)
	{
		UE_LOG(LogRailGame, Warning, TEXT("Beat %d %s holds but has no encounter, released at once"), BeatIndex, *Beat.Name.ToString());
		Rider->ReleaseBeatHold(BeatIndex);
	}
}

void ARailGameMode::HandleEncounterFinished(int32 Kills, float Seconds)
{
	if (ActiveBeatIndex == INDEX_NONE)
	{
		return;
	}

	const int32 Cleared = ActiveBeatIndex;
	ActiveBeatIndex = INDEX_NONE;
	++BeatsCleared;
	UE_LOG(LogRailGame, Log, TEXT("Beat %d encounter cleared: %d kills in %.1f s"), Cleared, Kills, Seconds);

	if (Rider)
	{
		Rider->ReleaseBeatHold(Cleared);
	}
}

void ARailGameMode::HandleBeatCleared(ARailTrack* Track, int32 BeatIndex, float HeldSeconds)
{
	GetWorldTimerManager().SetTimer(SummaryTimer, this, &ARailGameMode::HideSummary, FMath::Max(SummaryHoldSeconds, 0.01f), false);
}

void ARailGameMode::HideSummary()
{
	if (HUD)
	{
		HUD->HideSummary();
	}
}

void ARailGameMode::HandleEnemyKilled(ACyberEnemy* Enemy, int32 Kills)
{
	++TotalKills;
}

void ARailGameMode::HandleHostageResolved(AHostageTaker* Taker, EHostageOutcome Outcome)
{
	switch (Outcome)
	{
	case EHostageOutcome::Rescued:
		++HostagesRescued;
		break;
	case EHostageOutcome::HostageHit:
		++HostagesHit;
		break;
	default:
		++HostagesLost;
		break;
	}

	UE_LOG(LogRailGame, Log, TEXT("Hostage taker %s resolved: %s"), *GetNameSafe(Taker), *StaticEnum<EHostageOutcome>()->GetNameStringByValue(static_cast<int64>(Outcome)));

	if (ActiveHostageBeatIndex != INDEX_NONE)
	{
		const int32 Cleared = ActiveHostageBeatIndex;
		ActiveHostageBeatIndex = INDEX_NONE;
		++BeatsCleared;
		if (Rider)
		{
			Rider->ReleaseBeatHold(Cleared);
		}
	}
}

void ARailGameMode::HandleRideFinished(float TotalDistance, float Seconds)
{
	const URailAimComponent* Aim = Rider ? Rider->GetAim() : nullptr;
	const int32 Shots = Aim ? Aim->GetShotsFired() : 0;
	const int32 Hits = Aim ? Aim->GetShotsHit() : 0;

	UE_LOG(LogRailGame, Log, TEXT("Route complete: %.0f cm in %.1f s, beats cleared %d, kills %d, shots %d, hits %d, health %.0f, hits blocked by cover %d, hostages freed %d, hit %d, lost %d"),
		TotalDistance, Seconds, BeatsCleared, TotalKills, Shots, Hits, Rider ? Rider->GetHealth() : 0.0f, Rider ? Rider->GetHitsBlockedByCover() : 0,
		HostagesRescued, HostagesHit, HostagesLost);

	GetWorldTimerManager().ClearTimer(SummaryTimer);
	if (HUD)
	{
		HUD->ShowSummary(FText::Format(LOCTEXT("RouteComplete", "ROUTE COMPLETE\n\nDistance {0} m\nTime {1} s\nKills {2}\nHits {3} / {4}\nHostages freed {5} / {6}"),
			FText::AsNumber(FMath::RoundToInt(TotalDistance / 100.0f)), FText::AsNumber(FMath::RoundToInt(Seconds)),
			FText::AsNumber(TotalKills), FText::AsNumber(Hits), FText::AsNumber(Shots),
			FText::AsNumber(HostagesRescued), FText::AsNumber(HostagesRescued + HostagesHit + HostagesLost)));
	}
}

#undef LOCTEXT_NAMESPACE
