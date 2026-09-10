// CYB3RGUN THEGAME. Runs an encounter definition: spawns waves, tracks enemies, advances on triggers.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EncounterDefinition.h"
#include "EncounterDirector.generated.h"

class ACyberEnemy;
class AEnemySpawnPoint;
class UEnemyDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FEncounterWaveStartedDelegate, int32, Wave, int32, WaveCount, FText, WaveName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FEncounterCountsChangedDelegate, int32, Alive, int32, Kills, int32, Total);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FEncounterEnemyKilledDelegate, ACyberEnemy*, Enemy, int32, Kills);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FEncounterFinishedDelegate, int32, Kills, float, Seconds);

/**
 *  Consumes a UEncounterDefinition. Spawns enemies at spawn points, keeps the alive count,
 *  starts the next wave on its trigger and reports everything the HUD needs.
 */
UCLASS()
class CYB3RGUN_API AEncounterDirector : public AActor
{
	GENERATED_BODY()

protected:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Encounter")
	TObjectPtr<UEncounterDefinition> Definition;

	/** Spawn points to use. Empty means every AEnemySpawnPoint in the level. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Encounter")
	TArray<TObjectPtr<AEnemySpawnPoint>> SpawnPoints;

	/** Start at BeginPlay. The game mode usually starts the director itself once the player is ready. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Encounter")
	bool bAutoStart = false;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AEnemySpawnPoint>> ResolvedSpawnPoints;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UEnemyDefinition>> SpawnQueue;

	UPROPERTY(Transient)
	TSet<TObjectPtr<ACyberEnemy>> Alive;

	int32 CurrentWaveIndex = -1;
	int32 Kills = 0;
	int32 TotalSpawned = 0;
	int32 SpawnPointCursor = 0;
	bool bRunning = false;
	bool bFinished = false;
	bool bWaveSpawningDone = false;
	bool bNextWaveArmed = false;
	float StartTime = 0.0f;
	float WaveStartTime = 0.0f;

	FTimerHandle SpawnTimer;
	FTimerHandle WaveTimer;

public:

	UPROPERTY(BlueprintAssignable, Category="Encounter")
	FEncounterWaveStartedDelegate OnWaveStarted;

	UPROPERTY(BlueprintAssignable, Category="Encounter")
	FEncounterCountsChangedDelegate OnCountsChanged;

	UPROPERTY(BlueprintAssignable, Category="Encounter")
	FEncounterEnemyKilledDelegate OnEnemyKilled;

	UPROPERTY(BlueprintAssignable, Category="Encounter")
	FEncounterFinishedDelegate OnEncounterFinished;

public:

	AEncounterDirector();

	UFUNCTION(BlueprintCallable, Category="Encounter")
	void StartEncounter();

	/** Fires the Signal trigger of the next wave if it is waiting for one */
	UFUNCTION(BlueprintCallable, Category="Encounter")
	void Signal();

	UFUNCTION(BlueprintCallable, Category="Encounter")
	void SetDefinition(UEncounterDefinition* InDefinition) { Definition = InDefinition; }

	/** Spawn points for the next StartEncounter. Empty means every AEnemySpawnPoint in the level. */
	void SetSpawnPoints(const TArray<TObjectPtr<AEnemySpawnPoint>>& InSpawnPoints) { SpawnPoints = InSpawnPoints; }

	UFUNCTION(BlueprintPure, Category="Encounter")
	const UEncounterDefinition* GetDefinition() const { return Definition; }

	UFUNCTION(BlueprintPure, Category="Encounter")
	int32 GetCurrentWave() const { return CurrentWaveIndex + 1; }

	UFUNCTION(BlueprintPure, Category="Encounter")
	int32 GetWaveCount() const;

	UFUNCTION(BlueprintPure, Category="Encounter")
	FText GetCurrentWaveName() const;

	UFUNCTION(BlueprintPure, Category="Encounter")
	int32 GetAliveCount() const { return Alive.Num(); }

	UFUNCTION(BlueprintPure, Category="Encounter")
	int32 GetKills() const { return Kills; }

	UFUNCTION(BlueprintPure, Category="Encounter")
	int32 GetTotalEnemies() const;

	UFUNCTION(BlueprintPure, Category="Encounter")
	bool IsRunning() const { return bRunning; }

	UFUNCTION(BlueprintPure, Category="Encounter")
	bool IsFinished() const { return bFinished; }

	/** Alive enemies, for aiming aids and debug */
	TArray<ACyberEnemy*> GetAliveEnemies() const;

protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void HandleEnemyDied(ACyberEnemy* Enemy, AController* Killer);

	void ResolveSpawnPoints();
	void StartWave(int32 WaveIndex);
	void SpawnNext();
	void ArmNextWave();
	void TryAdvanceOnKills();
	void CheckFinished();
	void BroadcastCounts();
	ACyberEnemy* SpawnEnemy(UEnemyDefinition* EnemyDefinition);
	const FEncounterWave* GetWave(int32 WaveIndex) const;
};
