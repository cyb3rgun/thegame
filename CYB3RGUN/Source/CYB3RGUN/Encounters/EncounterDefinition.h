// CYB3RGUN THEGAME. Data asset describing an encounter: ordered waves with composition and triggers.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "EncounterDefinition.generated.h"

class UEnemyDefinition;

/** What starts a wave */
UENUM(BlueprintType)
enum class EEncounterWaveTrigger : uint8
{
	/** Seconds after the previous wave started */
	Time,
	/** When at most this many enemies are still alive after the previous wave finished spawning */
	Kills,
	/** When the director receives a signal */
	Signal
};

USTRUCT(BlueprintType)
struct FEnemySpawnEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UEnemyDefinition> Definition;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = 1))
	int32 Count = 1;
};

USTRUCT(BlueprintType)
struct FEncounterWave
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText Name;

	/** Enemy types and counts in this wave */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FEnemySpawnEntry> Composition;

	/** Seconds between two spawns inside the wave */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = 0.0, Units = "s"))
	float SpawnInterval = 1.0f;

	/** Seconds between the trigger firing and the first spawn */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = 0.0, Units = "s"))
	float StartDelay = 0.0f;

	/** How this wave starts. The first wave starts with the encounter and ignores this. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EEncounterWaveTrigger Trigger = EEncounterWaveTrigger::Time;

	/** Time trigger: seconds after the previous wave started */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = 0.0, Units = "s", EditCondition = "Trigger == EEncounterWaveTrigger::Time"))
	float TriggerSeconds = 20.0f;

	/** Kills trigger: start when at most this many enemies are alive */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = 0, EditCondition = "Trigger == EEncounterWaveTrigger::Kills"))
	int32 TriggerAliveAtMost = 0;

	/** Mix the spawn order instead of spawning type by type */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bShuffle = true;

	int32 GetEnemyCount() const;
};

/**
 *  Ordered waves for one encounter. A director consumes it; nothing is hand placed.
 *  Pacing rule: build up, peak, then relief. Do not author uniform waves.
 */
UCLASS(BlueprintType)
class CYB3RGUN_API UEncounterDefinition : public UDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Encounter")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Encounter")
	TArray<FEncounterWave> Waves;

	UFUNCTION(BlueprintPure, Category="Encounter")
	int32 GetWaveCount() const { return Waves.Num(); }

	UFUNCTION(BlueprintPure, Category="Encounter")
	int32 GetTotalEnemyCount() const;
};
