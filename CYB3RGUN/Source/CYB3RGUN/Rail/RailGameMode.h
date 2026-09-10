// CYB3RGUN THEGAME. Game mode for rail routes: runs each beat's encounter and releases the hold when it is cleared.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RailTrack.h"
#include "RailGameMode.generated.h"

class AEncounterDirector;
class ACyberEnemy;
class ARailPawn;
class UEncounterHUD;
class URailCrosshairWidget;

/**
 *  Possesses the rail pawn placed in the level, or spawns the default pawn when there is none.
 *  Listens to the rider's beats: a beat with an encounter starts it on a director owned by this mode,
 *  and a holding beat is released when that encounter is finished. The encounter HUD and the crosshair
 *  follow the same pattern as the other scenarios.
 */
UCLASS()
class CYB3RGUN_API ARailGameMode : public AGameModeBase
{
	GENERATED_BODY()

protected:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Rail")
	TSubclassOf<UEncounterHUD> EncounterHUDClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Rail")
	TSubclassOf<URailCrosshairWidget> CrosshairWidgetClass;

	/** Seconds the encounter summary stays up after a beat is cleared and the ride moves on */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Rail", meta = (ClampMin = 0.0, Units = "s"))
	float SummaryHoldSeconds = 2.5f;

	UPROPERTY(Transient)
	TObjectPtr<AEncounterDirector> Director;

	UPROPERTY(Transient)
	TObjectPtr<ARailPawn> Rider;

	UPROPERTY(Transient)
	TObjectPtr<UEncounterHUD> HUD;

	UPROPERTY(Transient)
	TObjectPtr<URailCrosshairWidget> Crosshair;

	int32 ActiveBeatIndex = INDEX_NONE;
	int32 BeatsCleared = 0;
	int32 TotalKills = 0;

	FTimerHandle SummaryTimer;

public:

	ARailGameMode();

	UFUNCTION(BlueprintPure, Category="Rail")
	AEncounterDirector* GetDirector() const { return Director; }

	UFUNCTION(BlueprintPure, Category="Rail")
	ARailPawn* GetRider() const { return Rider; }

	UFUNCTION(BlueprintPure, Category="Rail")
	int32 GetBeatsCleared() const { return BeatsCleared; }

	UFUNCTION(BlueprintPure, Category="Rail")
	int32 GetTotalKills() const { return TotalKills; }

protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual APawn* SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot) override;
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

	UFUNCTION()
	void HandleBeatReached(ARailTrack* Track, int32 BeatIndex, const FRailBeat& Beat);

	UFUNCTION()
	void HandleBeatCleared(ARailTrack* Track, int32 BeatIndex, float HeldSeconds);

	UFUNCTION()
	void HandleEncounterFinished(int32 Kills, float Seconds);

	UFUNCTION()
	void HandleEnemyKilled(ACyberEnemy* Enemy, int32 Kills);

	UFUNCTION()
	void HandleRideFinished(float TotalDistance, float Seconds);

	void BindRider(ARailPawn* InRider);
	void HideSummary();
};
