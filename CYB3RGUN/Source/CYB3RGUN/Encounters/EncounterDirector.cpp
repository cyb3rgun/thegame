// CYB3RGUN THEGAME. Runs an encounter definition.

#include "EncounterDirector.h"
#include "CyberEnemy.h"
#include "EnemyDefinition.h"
#include "EnemySpawnPoint.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogEncounter, Log, All);

AEncounterDirector::AEncounterDirector()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AEncounterDirector::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoStart)
	{
		StartEncounter();
	}
}

void AEncounterDirector::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(SpawnTimer);
	GetWorldTimerManager().ClearTimer(WaveTimer);
	Super::EndPlay(EndPlayReason);
}

int32 AEncounterDirector::GetWaveCount() const
{
	return Definition ? Definition->GetWaveCount() : 0;
}

int32 AEncounterDirector::GetTotalEnemies() const
{
	return Definition ? Definition->GetTotalEnemyCount() : 0;
}

FText AEncounterDirector::GetCurrentWaveName() const
{
	const FEncounterWave* Wave = GetWave(CurrentWaveIndex);
	return Wave ? Wave->Name : FText::GetEmpty();
}

const FEncounterWave* AEncounterDirector::GetWave(int32 WaveIndex) const
{
	return (Definition && Definition->Waves.IsValidIndex(WaveIndex)) ? &Definition->Waves[WaveIndex] : nullptr;
}

TArray<ACyberEnemy*> AEncounterDirector::GetAliveEnemies() const
{
	TArray<ACyberEnemy*> Result;
	for (const TObjectPtr<ACyberEnemy>& Enemy : Alive)
	{
		if (Enemy && !Enemy->IsDead())
		{
			Result.Add(Enemy);
		}
	}
	return Result;
}

void AEncounterDirector::ResolveSpawnPoints()
{
	ResolvedSpawnPoints.Reset();

	for (AEnemySpawnPoint* Point : SpawnPoints)
	{
		if (Point)
		{
			ResolvedSpawnPoints.Add(Point);
		}
	}

	if (ResolvedSpawnPoints.IsEmpty())
	{
		for (TActorIterator<AEnemySpawnPoint> It(GetWorld()); It; ++It)
		{
			ResolvedSpawnPoints.Add(*It);
		}
	}

	ResolvedSpawnPoints.Sort([](const AEnemySpawnPoint& A, const AEnemySpawnPoint& B)
	{
		return A.GetName() < B.GetName();
	});
}

void AEncounterDirector::StartEncounter()
{
	if (!Definition || Definition->Waves.IsEmpty())
	{
		UE_LOG(LogEncounter, Warning, TEXT("%s has no encounter definition with waves"), *GetName());
		return;
	}

	ResolveSpawnPoints();
	if (ResolvedSpawnPoints.IsEmpty())
	{
		UE_LOG(LogEncounter, Warning, TEXT("%s found no spawn points"), *GetName());
		return;
	}

	GetWorldTimerManager().ClearTimer(SpawnTimer);
	GetWorldTimerManager().ClearTimer(WaveTimer);

	// a restart removes whatever the previous run left standing
	for (const TObjectPtr<ACyberEnemy>& Enemy : Alive)
	{
		if (Enemy)
		{
			Enemy->OnEnemyDied.RemoveDynamic(this, &AEncounterDirector::HandleEnemyDied);
			Enemy->Destroy();
		}
	}

	Kills = 0;
	TotalSpawned = 0;
	Alive.Reset();
	SpawnQueue.Reset();
	bRunning = true;
	bFinished = false;
	bNextWaveArmed = false;
	StartTime = GetWorld()->GetTimeSeconds();

	UE_LOG(LogEncounter, Log, TEXT("Encounter %s starts: %d waves, %d enemies, %d spawn points"),
		*GetNameSafe(Definition), Definition->GetWaveCount(), Definition->GetTotalEnemyCount(), ResolvedSpawnPoints.Num());

	BroadcastCounts();
	StartWave(0);
}

void AEncounterDirector::StartWave(int32 WaveIndex)
{
	const FEncounterWave* Wave = GetWave(WaveIndex);
	if (!Wave)
	{
		return;
	}

	CurrentWaveIndex = WaveIndex;
	WaveStartTime = GetWorld()->GetTimeSeconds();
	bWaveSpawningDone = false;
	bNextWaveArmed = false;

	SpawnQueue.Reset();
	for (const FEnemySpawnEntry& Entry : Wave->Composition)
	{
		if (!Entry.Definition)
		{
			continue;
		}
		for (int32 i = 0; i < Entry.Count; ++i)
		{
			SpawnQueue.Add(Entry.Definition);
		}
	}

	if (Wave->bShuffle)
	{
		for (int32 i = SpawnQueue.Num() - 1; i > 0; --i)
		{
			SpawnQueue.Swap(i, FMath::RandRange(0, i));
		}
	}

	UE_LOG(LogEncounter, Log, TEXT("Wave %d of %d starts: %s, %d enemies, interval %.2fs, start delay %.2fs"),
		WaveIndex + 1, GetWaveCount(), *Wave->Name.ToString(), SpawnQueue.Num(), Wave->SpawnInterval, Wave->StartDelay);

	OnWaveStarted.Broadcast(WaveIndex + 1, GetWaveCount(), Wave->Name);

	if (SpawnQueue.IsEmpty())
	{
		bWaveSpawningDone = true;
		ArmNextWave();
		return;
	}

	GetWorldTimerManager().SetTimer(SpawnTimer, this, &AEncounterDirector::SpawnNext, FMath::Max(Wave->StartDelay, 0.01f), false);
}

void AEncounterDirector::SpawnNext()
{
	const FEncounterWave* Wave = GetWave(CurrentWaveIndex);
	if (!Wave || SpawnQueue.IsEmpty())
	{
		bWaveSpawningDone = true;
		ArmNextWave();
		return;
	}

	UEnemyDefinition* Next = SpawnQueue.Pop();
	SpawnEnemy(Next);

	if (SpawnQueue.IsEmpty())
	{
		bWaveSpawningDone = true;
		ArmNextWave();
	}
	else
	{
		GetWorldTimerManager().SetTimer(SpawnTimer, this, &AEncounterDirector::SpawnNext, FMath::Max(Wave->SpawnInterval, 0.05f), false);
	}
}

ACyberEnemy* AEncounterDirector::SpawnEnemy(UEnemyDefinition* EnemyDefinition)
{
	if (!EnemyDefinition || ResolvedSpawnPoints.IsEmpty())
	{
		return nullptr;
	}

	// walk the spawn points in a rotating order so a wave spreads around the arena
	AEnemySpawnPoint* Point = ResolvedSpawnPoints[SpawnPointCursor % ResolvedSpawnPoints.Num()];
	SpawnPointCursor = (SpawnPointCursor + 1 + FMath::RandRange(0, 1)) % FMath::Max(ResolvedSpawnPoints.Num(), 1);

	UClass* EnemyClass = EnemyDefinition->EnemyClass ? EnemyDefinition->EnemyClass.Get() : ACyberEnemy::StaticClass();
	FTransform SpawnTransform = Point->GetSpawnTransform();
	SpawnTransform.AddToTranslation(FVector(0.0f, 0.0f, EnemyDefinition->CapsuleHalfHeight + 5.0f));

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	ACyberEnemy* Enemy = GetWorld()->SpawnActorDeferred<ACyberEnemy>(EnemyClass, SpawnTransform, this, nullptr, Params.SpawnCollisionHandlingOverride);
	if (!Enemy)
	{
		UE_LOG(LogEncounter, Warning, TEXT("Failed to spawn %s"), *GetNameSafe(EnemyDefinition));
		return nullptr;
	}

	Enemy->Initialize(EnemyDefinition);
	Enemy->FinishSpawning(SpawnTransform);
	Enemy->OnEnemyDied.AddUniqueDynamic(this, &AEncounterDirector::HandleEnemyDied);

	Alive.Add(Enemy);
	++TotalSpawned;

	UE_LOG(LogEncounter, Verbose, TEXT("Spawned %s (%s) at %s, alive %d"), *Enemy->GetName(), *GetNameSafe(EnemyDefinition), *Point->GetName(), Alive.Num());
	BroadcastCounts();
	return Enemy;
}

void AEncounterDirector::HandleEnemyDied(ACyberEnemy* Enemy, AController* Killer)
{
	if (!Alive.Remove(Enemy))
	{
		return;
	}

	++Kills;
	UE_LOG(LogEncounter, Log, TEXT("Kill %d: %s by %s, alive %d"), Kills, *GetNameSafe(Enemy), *GetNameSafe(Killer), Alive.Num());

	OnEnemyKilled.Broadcast(Enemy, Kills);
	BroadcastCounts();
	TryAdvanceOnKills();
	CheckFinished();
}

void AEncounterDirector::ArmNextWave()
{
	const FEncounterWave* Next = GetWave(CurrentWaveIndex + 1);
	if (!Next)
	{
		CheckFinished();
		return;
	}

	bNextWaveArmed = true;

	switch (Next->Trigger)
	{
	case EEncounterWaveTrigger::Time:
	{
		const float Elapsed = GetWorld()->GetTimeSeconds() - WaveStartTime;
		const float Remaining = FMath::Max(Next->TriggerSeconds - Elapsed, 0.01f);
		const int32 NextIndex = CurrentWaveIndex + 1;
		GetWorldTimerManager().SetTimer(WaveTimer, [this, NextIndex]()
		{
			StartWave(NextIndex);
		}, Remaining, false);
		UE_LOG(LogEncounter, Verbose, TEXT("Next wave in %.1fs"), Remaining);
		break;
	}
	case EEncounterWaveTrigger::Kills:
		TryAdvanceOnKills();
		break;
	case EEncounterWaveTrigger::Signal:
		UE_LOG(LogEncounter, Verbose, TEXT("Next wave waits for a signal"));
		break;
	}
}

void AEncounterDirector::TryAdvanceOnKills()
{
	if (!bRunning || !bWaveSpawningDone || !bNextWaveArmed)
	{
		return;
	}

	const FEncounterWave* Next = GetWave(CurrentWaveIndex + 1);
	if (Next && Next->Trigger == EEncounterWaveTrigger::Kills && Alive.Num() <= Next->TriggerAliveAtMost)
	{
		bNextWaveArmed = false;
		StartWave(CurrentWaveIndex + 1);
	}
}

void AEncounterDirector::Signal()
{
	const FEncounterWave* Next = GetWave(CurrentWaveIndex + 1);
	if (bRunning && bWaveSpawningDone && bNextWaveArmed && Next && Next->Trigger == EEncounterWaveTrigger::Signal)
	{
		bNextWaveArmed = false;
		StartWave(CurrentWaveIndex + 1);
	}
}

void AEncounterDirector::CheckFinished()
{
	if (!bRunning || bFinished)
	{
		return;
	}

	const bool bLastWave = GetWave(CurrentWaveIndex + 1) == nullptr;
	if (bLastWave && bWaveSpawningDone && Alive.IsEmpty())
	{
		bRunning = false;
		bFinished = true;
		const float Seconds = GetWorld()->GetTimeSeconds() - StartTime;
		UE_LOG(LogEncounter, Log, TEXT("Encounter complete: %d kills in %.1fs"), Kills, Seconds);
		OnEncounterFinished.Broadcast(Kills, Seconds);
	}
}

void AEncounterDirector::BroadcastCounts()
{
	OnCountsChanged.Broadcast(Alive.Num(), Kills, GetTotalEnemies());
}
