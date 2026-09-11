// CYB3RGUN THEGAME. Console commands that drive a rail route for automated verification.
// Every shot goes through the rider's aim component: the command only places the crosshair on screen,
// exactly like a light gun reporting a cursor position. Not compiled into shipping builds.

#include "RailAimComponent.h"
#include "RailGameMode.h"
#include "RailPawn.h"
#include "CyberEnemy.h"
#include "EncounterDirector.h"
#include "HostageTaker.h"
#include "EngineUtils.h"
#include "CollisionQueryParams.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

#if !UE_BUILD_SHIPPING

DEFINE_LOG_CATEGORY_STATIC(LogRailDebug, Log, All);

namespace RailDebug
{
	ARailGameMode* GetMode(UWorld* World)
	{
		return World ? Cast<ARailGameMode>(World->GetAuthGameMode()) : nullptr;
	}

	ARailPawn* GetRider(UWorld* World)
	{
		APlayerController* PC = World ? UGameplayStatics::GetPlayerController(World, 0) : nullptr;
		return PC ? Cast<ARailPawn>(PC->GetPawn()) : nullptr;
	}

	/** Puts the crosshair on the nearest alive enemy that is on screen and not hidden behind geometry. Returns it or null. */
	ACyberEnemy* PlaceCrosshairOnNearest(UWorld* World, FVector2D& OutScreen)
	{
		ARailGameMode* Mode = GetMode(World);
		ARailPawn* Rider = GetRider(World);
		APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
		if (!Mode || !Mode->GetDirector() || !Rider || !PC)
		{
			return nullptr;
		}

		int32 SizeX = 0;
		int32 SizeY = 0;
		PC->GetViewportSize(SizeX, SizeY);

		ACyberEnemy* Best = nullptr;
		float BestDistance = TNumericLimits<float>::Max();
		for (ACyberEnemy* Enemy : Mode->GetDirector()->GetAliveEnemies())
		{
			FVector2D Screen;
			if (!PC->ProjectWorldLocationToScreen(Enemy->GetAimPoint(), Screen, false))
			{
				continue;
			}
			if (Screen.X < 0.0 || Screen.Y < 0.0 || Screen.X > SizeX || Screen.Y > SizeY)
			{
				continue;
			}

			// the same screen space trace the shot will use must reach this enemy, otherwise a wall is in the way
			FHitResult LineCheck;
			FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(RailDebugLine), false, Rider);
			if (!PC->GetHitResultAtScreenPosition(Screen, ECC_Visibility, QueryParams, LineCheck) || LineCheck.GetActor() != Enemy)
			{
				continue;
			}
			const float Distance = FVector::Dist(Rider->GetActorLocation(), Enemy->GetActorLocation());
			if (Distance < BestDistance)
			{
				BestDistance = Distance;
				Best = Enemy;
				OutScreen = Screen;
			}
		}

		if (Best)
		{
			Rider->GetAim()->SetCrosshairScreenPosition(OutScreen);
		}
		return Best;
	}

	/** One shot through the real fire input path. Logs what the crosshair was put on. */
	void AimAndFire(UWorld* World)
	{
		ARailPawn* Rider = GetRider(World);
		if (!Rider)
		{
			return;
		}

		FVector2D Screen = FVector2D::ZeroVector;
		ACyberEnemy* Target = PlaceCrosshairOnNearest(World, Screen);
		if (!Target)
		{
			return;
		}

		const bool bWasBlocked = Rider->GetAim()->IsFireBlocked();
		Rider->DoFire();
		UE_LOG(LogRailDebug, Log, TEXT("Rail.Fire: %s at screen %.0f %.0f, distance %.0f cm%s"), *Target->GetName(), Screen.X, Screen.Y,
			FVector::Dist(Rider->GetActorLocation(), Target->GetActorLocation()), bWasBlocked ? TEXT(", blocked by cover") : TEXT(""));
	}

	void FireBurst(TWeakObjectPtr<UWorld> WeakWorld, int32 ShotsLeft)
	{
		UWorld* World = WeakWorld.Get();
		if (!World || ShotsLeft <= 0)
		{
			return;
		}

		AimAndFire(World);

		if (ShotsLeft > 1)
		{
			FTimerHandle Handle;
			World->GetTimerManager().SetTimer(Handle, [WeakWorld, ShotsLeft]()
			{
				FireBurst(WeakWorld, ShotsLeft - 1);
			}, 0.3f, false);
		}
	}

	FTimerHandle AutoFireTimer;
}

static FAutoConsoleCommandWithWorldAndArgs GRailFireCommand(
	TEXT("Rail.Fire"),
	TEXT("Places the rider's crosshair on the nearest on screen enemy and pulls the trigger. Optional argument: shots in a burst."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		const int32 Shots = Args.Num() > 0 ? FMath::Clamp(FCString::Atoi(*Args[0]), 1, 60) : 1;
		RailDebug::FireBurst(World, Shots);
	}));

static FAutoConsoleCommandWithWorldAndArgs GRailAutoFireCommand(
	TEXT("Rail.AutoFire"),
	TEXT("1 starts firing at the nearest on screen enemy every 0.3 s for the whole ride, 0 stops."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (!World)
		{
			return;
		}
		const bool bOn = Args.Num() == 0 || FCString::Atoi(*Args[0]) != 0;
		World->GetTimerManager().ClearTimer(RailDebug::AutoFireTimer);
		if (bOn)
		{
			TWeakObjectPtr<UWorld> WeakWorld(World);
			World->GetTimerManager().SetTimer(RailDebug::AutoFireTimer, [WeakWorld]()
			{
				if (UWorld* Inner = WeakWorld.Get())
				{
					RailDebug::AimAndFire(Inner);
				}
			}, 0.3f, true);
		}
		UE_LOG(LogRailDebug, Log, TEXT("Rail.AutoFire %s"), bOn ? TEXT("on") : TEXT("off"));
	}));

static FAutoConsoleCommandWithWorldAndArgs GRailCoverCommand(
	TEXT("Rail.Cover"),
	TEXT("1 enters cover, 0 leaves it, no argument toggles."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (ARailPawn* Rider = RailDebug::GetRider(World))
		{
			const bool bCover = Args.Num() > 0 ? FCString::Atoi(*Args[0]) != 0 : !Rider->IsInCover();
			Rider->SetInCover(bCover);
		}
	}));

static FAutoConsoleCommandWithWorld GRailStatusCommand(
	TEXT("Rail.Status"),
	TEXT("Logs distance, speed, hold, cover, health and encounter state of the ride."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		ARailPawn* Rider = RailDebug::GetRider(World);
		ARailGameMode* Mode = RailDebug::GetMode(World);
		if (!Rider)
		{
			UE_LOG(LogRailDebug, Log, TEXT("Rail.Status: no rider"));
			return;
		}

		const AEncounterDirector* Director = Mode ? Mode->GetDirector() : nullptr;
		UE_LOG(LogRailDebug, Log, TEXT("Rail.Status: %s at %.0f cm, ride %.0f cm, speed %.0f, moving %d, held beat %d, cover %d, health %.0f, finished %d, beats cleared %d, kills %d, alive %d, shots %d, hits %d"),
			*GetNameSafe(Rider->GetTrack()), Rider->GetDistanceAlongSpline(), Rider->GetRideDistance(), Rider->GetSpeed(), Rider->IsMoving() ? 1 : 0,
			Rider->GetHeldBeatIndex(), Rider->IsInCover() ? 1 : 0, Rider->GetHealth(), Rider->IsFinished() ? 1 : 0,
			Mode ? Mode->GetBeatsCleared() : 0, Mode ? Mode->GetTotalKills() : 0, Director ? Director->GetAliveCount() : 0,
			Rider->GetAim()->GetShotsFired(), Rider->GetAim()->GetShotsHit());

		FWeaponStatus Weapon;
		if (Rider->GetWeaponStatus(Weapon))
		{
			UE_LOG(LogRailDebug, Log, TEXT("Rail.Status: weapon %s %d/%d, reloading %d at %.0f%%, switching %d, weapon %d of %d"), *Weapon.WeaponName.ToString(), Weapon.Rounds, Weapon.MagazineSize,
				Weapon.bReloading ? 1 : 0, Weapon.ReloadProgress * 100.0f, Weapon.bSwitching ? 1 : 0, Weapon.WeaponIndex + 1, Weapon.WeaponCount);
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GRailHostageCommand(
	TEXT("Rail.Hostage"),
	TEXT("Places the crosshair on the showing part of the active hostage taker and pulls the trigger. Arguments: Hostage aims at the hostage instead, Aim only places the crosshair."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		ARailPawn* Rider = RailDebug::GetRider(World);
		APlayerController* PC = World ? UGameplayStatics::GetPlayerController(World, 0) : nullptr;
		if (!Rider || !PC)
		{
			return;
		}

		AHostageTaker* Taker = nullptr;
		for (TActorIterator<AHostageTaker> It(World); It; ++It)
		{
			if (It->IsActive())
			{
				Taker = *It;
				break;
			}
		}
		if (!Taker)
		{
			UE_LOG(LogRailDebug, Log, TEXT("Rail.Hostage: no active hostage taker"));
			return;
		}

		const bool bHostage = Args.ContainsByPredicate([](const FString& Arg) { return Arg.Equals(TEXT("Hostage"), ESearchCase::IgnoreCase); });
		const bool bAimOnly = Args.ContainsByPredicate([](const FString& Arg) { return Arg.Equals(TEXT("Aim"), ESearchCase::IgnoreCase); });
		FVector2D Screen;
		if (!PC->ProjectWorldLocationToScreen(bHostage ? Taker->GetHostageAimPoint() : Taker->GetTakerAimPoint(), Screen, false))
		{
			UE_LOG(LogRailDebug, Log, TEXT("Rail.Hostage: %s is off screen"), *Taker->GetName());
			return;
		}

		Rider->GetAim()->SetCrosshairScreenPosition(Screen);
		if (!bAimOnly)
		{
			Rider->DoFire();
		}
		UE_LOG(LogRailDebug, Log, TEXT("Rail.Hostage: %s the %s of %s at screen %.0f %.0f"), bAimOnly ? TEXT("aimed at") : TEXT("fired at"),
			bHostage ? TEXT("hostage") : TEXT("taker"), *Taker->GetName(), Screen.X, Screen.Y);
	}));

static FAutoConsoleCommandWithWorld GRailPauseCommand(
	TEXT("Rail.Pause"),
	TEXT("Pauses the ride."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		if (ARailPawn* Rider = RailDebug::GetRider(World))
		{
			Rider->Pause();
		}
	}));

static FAutoConsoleCommandWithWorld GRailResumeCommand(
	TEXT("Rail.Resume"),
	TEXT("Resumes a paused ride."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		if (ARailPawn* Rider = RailDebug::GetRider(World))
		{
			Rider->Resume();
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GRailSpeedCommand(
	TEXT("Rail.Speed"),
	TEXT("Sets the ride speed in cm/s."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		ARailPawn* Rider = RailDebug::GetRider(World);
		if (Rider && Args.Num() > 0)
		{
			Rider->SetSpeed(FCString::Atof(*Args[0]));
		}
	}));

#endif
