// CYB3RGUN THEGAME. Console commands that drive the zombie test for automated verification.
// Not compiled into shipping builds.

#include "CyberEnemy.h"
#include "EncounterDirector.h"
#include "ZombieTestGameMode.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Misc/OutputDeviceNull.h"
#include "TimerManager.h"

#if !UE_BUILD_SHIPPING

DEFINE_LOG_CATEGORY_STATIC(LogZombieDebug, Log, All);

namespace ZombieDebug
{
	AEncounterDirector* GetDirector(UWorld* World)
	{
		AZombieTestGameMode* Mode = World ? Cast<AZombieTestGameMode>(World->GetAuthGameMode()) : nullptr;
		return Mode ? Mode->GetDirector() : nullptr;
	}

	/** Turns the local player toward the nearest alive enemy. Returns it or null. */
	ACyberEnemy* AimAtNearest(UWorld* World)
	{
		AEncounterDirector* Director = GetDirector(World);
		APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
		if (!Director || !PC || !PC->GetPawn())
		{
			return nullptr;
		}

		const FVector Eye = PC->GetPawn()->GetPawnViewLocation();
		ACyberEnemy* Nearest = nullptr;
		float NearestDistance = TNumericLimits<float>::Max();
		for (ACyberEnemy* Enemy : Director->GetAliveEnemies())
		{
			const float Distance = FVector::DistSquared(Eye, Enemy->GetActorLocation());
			if (Distance < NearestDistance)
			{
				NearestDistance = Distance;
				Nearest = Enemy;
			}
		}

		if (Nearest)
		{
			PC->SetControlRotation(UKismetMathLibrary::FindLookAtRotation(Eye, Nearest->GetAimPoint()));
		}
		return Nearest;
	}

	void Fire(UWorld* World)
	{
		APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
		if (!PC || !PC->GetPawn())
		{
			return;
		}
		FOutputDeviceNull Ar;
		PC->GetPawn()->CallFunctionByNameWithArguments(TEXT("DoStartFiring"), Ar, nullptr, true);
		PC->GetPawn()->CallFunctionByNameWithArguments(TEXT("DoStopFiring"), Ar, nullptr, true);
	}
}

namespace ZombieDebug
{
	/** Aims at the nearest enemy and fires, then repeats until the burst is spent. Each shot re-aims so a burst can drop several enemies. */
	void FireBurst(TWeakObjectPtr<UWorld> WeakWorld, int32 ShotsLeft)
	{
		UWorld* World = WeakWorld.Get();
		if (!World || ShotsLeft <= 0)
		{
			return;
		}

		ACyberEnemy* Target = AimAtNearest(World);
		const APawn* Player = UGameplayStatics::GetPlayerPawn(World, 0);
		const float Distance = (Target && Player) ? FVector::Dist2D(Player->GetActorLocation(), Target->GetActorLocation()) : 0.0f;
		UE_LOG(LogZombieDebug, Log, TEXT("Zombie.Fire: target %s, distance %.0f cm, shots left %d"), Target ? *Target->GetName() : TEXT("none, firing ahead"), Distance, ShotsLeft);

		// weapons aim through the screen space path (D-019), which reads the view the player last saw,
		// so the trigger waits until the camera has caught up with the snap
		FTimerHandle FireHandle;
		World->GetTimerManager().SetTimer(FireHandle, [WeakWorld]()
		{
			if (UWorld* InnerWorld = WeakWorld.Get())
			{
				Fire(InnerWorld);
			}
		}, 0.1f, false);

		if (ShotsLeft > 1)
		{
			FTimerHandle Handle;
			World->GetTimerManager().SetTimer(Handle, [WeakWorld, ShotsLeft]()
			{
				FireBurst(WeakWorld, ShotsLeft - 1);
			}, 0.35f, false);
		}
	}
}

static FAutoConsoleCommandWithWorldAndArgs GZombieFireCommand(
	TEXT("Zombie.Fire"),
	TEXT("Aims the local player at the nearest alive enemy and fires the current weapon. Optional argument: number of shots in a burst, each re-aimed."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		const int32 Shots = Args.Num() > 0 ? FMath::Clamp(FCString::Atoi(*Args[0]), 1, 60) : 1;
		ZombieDebug::FireBurst(World, Shots);
	}));

static FAutoConsoleCommandWithWorld GZombieStatusCommand(
	TEXT("Zombie.Status"),
	TEXT("Logs wave, alive and kill counts of the running encounter."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		if (AEncounterDirector* Director = ZombieDebug::GetDirector(World))
		{
			UE_LOG(LogZombieDebug, Log, TEXT("Zombie.Status: wave %d/%d %s, alive %d, kills %d/%d, running %d, finished %d"),
				Director->GetCurrentWave(), Director->GetWaveCount(), *Director->GetCurrentWaveName().ToString(),
				Director->GetAliveCount(), Director->GetKills(), Director->GetTotalEnemies(), Director->IsRunning() ? 1 : 0, Director->IsFinished() ? 1 : 0);
		}
		else
		{
			UE_LOG(LogZombieDebug, Log, TEXT("Zombie.Status: no encounter director"));
		}
	}));

static FAutoConsoleCommandWithWorld GZombieSignalCommand(
	TEXT("Zombie.Signal"),
	TEXT("Sends the director signal that starts a signal triggered wave."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		if (AEncounterDirector* Director = ZombieDebug::GetDirector(World))
		{
			Director->Signal();
		}
	}));

static FAutoConsoleCommandWithWorld GZombieRestartCommand(
	TEXT("Zombie.Restart"),
	TEXT("Restarts the running encounter from wave one."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		if (AEncounterDirector* Director = ZombieDebug::GetDirector(World))
		{
			Director->StartEncounter();
		}
	}));

#endif
