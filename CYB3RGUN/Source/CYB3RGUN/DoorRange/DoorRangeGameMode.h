// CYB3RGUN THEGAME. Game mode running the door range.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DoorTypes.h"
#include "DoorRangeGameMode.generated.h"

class ADoorSlot;
class UDoorRangeSettings;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDoorRangeScoreChangedDelegate, int32, Score, int32, Delta);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDoorRangeWaveChangedDelegate, int32, Wave, int32, WaveCount);

/**
 *  Runs the door range: twelve door slots around the player, a few open at a time,
 *  random occupants, waves, and the score.
 *  Reads its tunables from a UDoorRangeSettings data asset.
 */
UCLASS()
class CYB3RGUN_API ADoorRangeGameMode : public AGameModeBase
{
	GENERATED_BODY()

protected:

	/** Tunables for this range. Falls back to class defaults when unset. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Door Range")
	TObjectPtr<UDoorRangeSettings> Settings;

	/** Start the first wave automatically at BeginPlay */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Door Range")
	bool bAutoStart = true;

	/** Print score and wave as on screen debug text. Placeholder until a real HUD exists. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Door Range")
	bool bShowDebugScore = true;

	/** All door slots found in the level, sorted by name for stable ordering */
	UPROPERTY(Transient)
	TArray<TObjectPtr<ADoorSlot>> Slots;

	int32 Score = 0;
	int32 CurrentWave = 0;
	int32 OpeningsThisWave = 0;
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
	bool IsRangeActive() const { return bRangeActive; }

	UFUNCTION(BlueprintPure, Category="Door Range")
	bool IsRangeComplete() const { return bRangeComplete; }

	UFUNCTION(BlueprintPure, Category="Door Range")
	const UDoorRangeSettings* GetSettings() const;

protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

	UFUNCTION()
	void HandleSlotHit(ADoorSlot* Slot, EDoorOccupant Occupant, float ExposureFraction);

	UFUNCTION()
	void HandleSlotClosed(ADoorSlot* Slot, EDoorOccupant Occupant, bool bWasHit);

	void CollectSlots();
	void StartWave(int32 WaveNumber);
	void TryOpenDoor();
	void EndWave();
	void FinishRange();
	void AddScore(int32 Delta, const FString& Reason);
	void UpdateDebugDisplay(const FString& LastEvent) const;
	void GrantStartingWeapon();

	EDoorOccupant RollOccupant() const;
	ADoorSlot* PickAvailableSlot() const;
};
