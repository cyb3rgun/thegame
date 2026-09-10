// CYB3RGUN THEGAME. Tunables for the door range.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DoorRangeSettings.generated.h"

class UMaterialInterface;
class AShooterWeapon;

/**
 *  Timings, probabilities, wave layout and scoring values for the door range.
 *  Designers tune one asset; the game mode reads it at BeginPlay.
 */
UCLASS(BlueprintType)
class CYB3RGUN_API UDoorRangeSettings : public UDataAsset
{
	GENERATED_BODY()

public:

	/** Seconds for the door panel to swing open */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Timing", meta = (ClampMin = 0.05, Units = "s"))
	float OpenDuration = 0.4f;

	/** Seconds the occupant stays exposed with the door fully open */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Timing", meta = (ClampMin = 0.0, Units = "s"))
	float ExposureWindow = 1.5f;

	/** Seconds for the door panel to swing shut */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Timing", meta = (ClampMin = 0.05, Units = "s"))
	float CloseDuration = 0.4f;

	/** Seconds between two door openings */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Timing", meta = (ClampMin = 0.0, Units = "s"))
	float TimeBetweenOpenings = 0.6f;

	/** Pause between waves */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Timing", meta = (ClampMin = 0.0, Units = "s"))
	float TimeBetweenWaves = 2.0f;

	/** Chance that an opening door shows a hostile */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Probabilities", meta = (ClampMin = 0.0, ClampMax = 1.0))
	float HostileShare = 0.6f;

	/** Chance that an opening door shows nothing. The rest of the probability goes to friendlies. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Probabilities", meta = (ClampMin = 0.0, ClampMax = 1.0))
	float EmptyShare = 0.1f;

	/** Number of waves in one range session */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Waves", meta = (ClampMin = 1))
	int32 WaveCount = 3;

	/** Door openings per wave */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Waves", meta = (ClampMin = 1))
	int32 OpeningsPerWave = 12;

	/** Maximum doors open at the same time */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Waves", meta = (ClampMin = 1, ClampMax = 12))
	int32 VisibleDoors = 3;

	/** Base points for hitting a hostile */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Scoring", meta = (ClampMin = 0))
	int32 HitHostileScore = 100;

	/** Extra points for a hostile hit at the very end of the exposure window, scaled down for earlier hits. Waiting for the draw scores higher. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Scoring", meta = (ClampMin = 0))
	int32 DrawBonusMax = 100;

	/** Points lost for hitting a friendly */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Scoring", meta = (ClampMin = 0))
	int32 HitFriendlyPenalty = 150;

	/** Points lost when a hostile closes without being hit */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Scoring", meta = (ClampMin = 0))
	int32 MissedHostilePenalty = 50;

	/** Material applied to hostile placeholder occupants */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Occupants")
	TObjectPtr<UMaterialInterface> HostileMaterial;

	/** Material applied to friendly placeholder occupants */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Occupants")
	TObjectPtr<UMaterialInterface> FriendlyMaterial;

	/** Weapon handed to the player when the range starts */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon")
	TSubclassOf<AShooterWeapon> StartingWeaponClass;
};
