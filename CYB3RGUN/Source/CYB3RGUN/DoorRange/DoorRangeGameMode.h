// CYB3RGUN THEGAME. Game mode running the door range.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DoorTypes.h"
#include "StyleScenario.h"
#include "DoorRangeGameMode.generated.h"

class ADoorSlot;
class UDoorRangeSettings;
class UDoorRangeHUD;
class UStyleSettings;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDoorRangeScoreChangedDelegate, int32, Score, int32, Delta);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDoorRangeWaveChangedDelegate, int32, Wave, int32, WaveCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDoorRangeHostilesChangedDelegate, int32, Remaining, int32, Total);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FDoorRangeEventDelegate, EDoorRangeEvent, Event, int32, Delta, ADoorSlot*, Slot);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDoorRangeFinishedDelegate, const FDoorRangeStats&, Stats);

/**
 *  Runs the door range: twelve door slots around the player, a few open at a time,
 *  a pre-rolled occupant mix per wave, a difficulty ramp, the score and the statistics.
 *  Reads its tunables from a UDoorRangeSettings data asset and drives the HUD through delegates.
 */
UCLASS()
class CYB3RGUN_API ADoorRangeGameMode : public AGameModeBase, public IStyleScenario
{
	GENERATED_BODY()

protected:

	/** Tunables for this range. Falls back to class defaults when unset. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Door Range")
	TObjectPtr<UDoorRangeSettings> Settings;

	/** HUD widget created for the local player */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Door Range")
	TSubclassOf<UDoorRangeHUD> RangeHUDClass;

	/** Style values of this scenario, empty uses the project default */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Door Range")
	TObjectPtr<UStyleSettings> StyleSettings;

	/** Start the first wave automatically at BeginPlay */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Door Range")
	bool bAutoStart = true;

	/** All door slots found in the level, sorted by name for stable ordering */
	UPROPERTY(Transient)
	TArray<TObjectPtr<ADoorSlot>> Slots;

	UPROPERTY(Transient)
	TObjectPtr<UDoorRangeHUD> HUD;

	/** Occupants still to be opened in the current wave, pre-rolled so the hostile share is exact */
	TArray<EDoorOccupant> WaveQueue;

	FDoorRangeStats Stats;

	int32 Score = 0;
	int32 CurrentWave = 0;
	int32 HostilesTotalThisWave = 0;
	int32 HostilesRemainingThisWave = 0;

	/** Hostage takers in the current wave, counted among its hostiles */
	int32 HostageTakersThisWave = 0;
	int32 OpenDoors = 0;
	bool bRangeActive = false;
	bool bRangeComplete = false;

	FTimerHandle OpenTimer;
	FTimerHandle WaveTimer;

public:

	UPROPERTY(BlueprintAssignable, Category="Door Range")
	FDoorRangeScoreChangedDelegate OnScoreChanged;

	UPROPERTY(BlueprintAssignable, Category="Door Range")
	FDoorRangeWaveChangedDelegate OnWaveChanged;

	UPROPERTY(BlueprintAssignable, Category="Door Range")
	FDoorRangeHostilesChangedDelegate OnHostilesRemainingChanged;

	UPROPERTY(BlueprintAssignable, Category="Door Range")
	FDoorRangeEventDelegate OnRangeEvent;

	UPROPERTY(BlueprintAssignable, Category="Door Range")
	FDoorRangeFinishedDelegate OnRangeFinished;

public:

	ADoorRangeGameMode();

	/** Starts the range from wave one. Safe to call again after the range completed. */
	UFUNCTION(BlueprintCallable, Category="Door Range")
	void StartRange();

	UFUNCTION(BlueprintPure, Category="Door Range")
	int32 GetScore() const { return Score; }

	UFUNCTION(BlueprintPure, Category="Door Range")
	int32 GetCurrentWave() const { return CurrentWave; }

	UFUNCTION(BlueprintPure, Category="Door Range")
	int32 GetWaveCount() const;

	UFUNCTION(BlueprintPure, Category="Door Range")
	int32 GetHostilesRemaining() const { return HostilesRemainingThisWave; }

	UFUNCTION(BlueprintPure, Category="Door Range")
	int32 GetHostilesTotalThisWave() const { return HostilesTotalThisWave; }

	UFUNCTION(BlueprintPure, Category="Door Range")
	bool IsRangeActive() const { return bRangeActive; }

	UFUNCTION(BlueprintPure, Category="Door Range")
	bool IsRangeComplete() const { return bRangeComplete; }

	UFUNCTION(BlueprintPure, Category="Door Range")
	FDoorRangeStats GetStats() const { return Stats; }

	UFUNCTION(BlueprintPure, Category="Door Range")
	const UDoorRangeSettings* GetSettings() const;

	//~ Begin IStyleScenario
	virtual const UStyleSettings* GetStyleSettings() const override { return StyleSettings; }
	//~ End IStyleScenario

	/** Door slots in this level, sorted by name */
	const TArray<TObjectPtr<ADoorSlot>>& GetSlots() const { return Slots; }

protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

	UFUNCTION()
	void HandleSlotHit(ADoorSlot* Slot, EDoorOccupant Occupant, float ExposureFraction);

	UFUNCTION()
	void HandleSlotClosed(ADoorSlot* Slot, EDoorOccupant Occupant, bool bWasHit);

	UFUNCTION()
	void HandleSlotDrawn(ADoorSlot* Slot);

	void CollectSlots();
	void CreateHUD(APlayerController* Player);
	void GrantStartingWeapon();
	void StartWave(int32 WaveNumber);
	void BuildWaveQueue(const FDoorWaveSettings& Wave);
	void TryOpenDoor();
	void EndWave();
	void FinishRange();
	void AddScore(int32 Delta, const FString& Reason);
	void PlayEventSound(EDoorRangeEvent Event, const ADoorSlot* Slot) const;
	ADoorSlot* PickAvailableSlot() const;
};
