// CYB3RGUN THEGAME. Tunables for the door range.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DoorTypes.h"
#include "DoorRangeSettings.generated.h"

class UMaterialInterface;
class USoundBase;
class AShooterWeapon;

/**
 *  Timings, difficulty ramp, scoring values and feedback assets for the door range.
 *  One asset per preset: a calm training range and a frantic entertainment range
 *  share the module and differ only in this data.
 */
UCLASS(BlueprintType)
class CYB3RGUN_API UDoorRangeSettings : public UDataAsset
{
	GENERATED_BODY()

public:

	UDoorRangeSettings();

	/** One entry per wave. The last entry repeats if the range asks for more waves than listed. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Difficulty Ramp")
	TArray<FDoorWaveSettings> Waves;

	/** Seconds for the door panel to swing open */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Timing", meta = (ClampMin = 0.05, Units = "s"))
	float OpenDuration = 0.4f;

	/** Seconds for the door panel to swing shut */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Timing", meta = (ClampMin = 0.05, Units = "s"))
	float CloseDuration = 0.4f;

	/** Pause between waves */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Timing", meta = (ClampMin = 0.0, Units = "s"))
	float TimeBetweenWaves = 3.0f;

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

	/** Material put on every slot of the hostile occupant body */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Occupants")
	TObjectPtr<UMaterialInterface> HostileMaterial;

	/** Material put on every slot of the friendly occupant body */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Occupants")
	TObjectPtr<UMaterialInterface> FriendlyMaterial;

	/** Weapon handed to the player when the range starts, the one in hand at the start */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon")
	TSubclassOf<AShooterWeapon> StartingWeaponClass;

	/** Further weapons handed out before the starting weapon, reached with the switch action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon")
	TArray<TSubclassOf<AShooterWeapon>> AdditionalWeaponClasses;

	/** Played at the slot when a hostile is hit */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Feedback")
	TObjectPtr<USoundBase> HostileHitSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Feedback", meta = (ClampMin = 0.1, ClampMax = 4.0))
	float HostileHitPitch = 1.5f;

	/** Played at the slot when a friendly is hit */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Feedback")
	TObjectPtr<USoundBase> FriendlyHitSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Feedback", meta = (ClampMin = 0.1, ClampMax = 4.0))
	float FriendlyHitPitch = 1.0f;

	/** Played at the slot when a hostile closes without being hit */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Feedback")
	TObjectPtr<USoundBase> EscapeSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Feedback", meta = (ClampMin = 0.1, ClampMax = 4.0))
	float EscapePitch = 0.5f;

	/** Played at the slot when a hostile starts rising */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Feedback")
	TObjectPtr<USoundBase> TelegraphSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Feedback", meta = (ClampMin = 0.1, ClampMax = 4.0))
	float TelegraphPitch = 2.0f;

	/** Played at the slot the moment a hostile is drawn and shootable */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Feedback")
	TObjectPtr<USoundBase> DrawSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Feedback", meta = (ClampMin = 0.1, ClampMax = 4.0))
	float DrawPitch = 1.2f;

	/** Played at the slot when a door starts opening and when it has closed */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Feedback")
	TObjectPtr<USoundBase> DoorSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Feedback", meta = (ClampMin = 0.1, ClampMax = 4.0))
	float DoorPitch = 0.8f;

	/** Number of waves in one range session */
	UFUNCTION(BlueprintPure, Category="Door Range")
	int32 GetWaveCount() const { return FMath::Max(Waves.Num(), 1); }

	/** Settings for a one based wave number, clamped to the last entry */
	const FDoorWaveSettings& GetWave(int32 WaveNumber) const;

	/** Blueprint copy of GetWave */
	UFUNCTION(BlueprintPure, Category="Door Range", meta = (DisplayName = "Get Wave Settings"))
	FDoorWaveSettings GetWaveSettings(int32 WaveNumber) const { return GetWave(WaveNumber); }
};
