// CYB3RGUN THEGAME. Game mode for the zombie test arena.

#include "ZombieTestGameMode.h"
#include "StyleHUDWidget.h"
#include "EncounterDirector.h"
#include "EncounterHUD.h"
#include "ShooterWeapon.h"
#include "ShooterWeaponHolder.h"
#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationInvokerComponent.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogZombieTest, Log, All);

void AZombieTestGameMode::BeginPlay()
{
	Super::BeginPlay();
	FindOrSpawnDirector();
}

void AZombieTestGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(StartTimer);
	Super::EndPlay(EndPlayReason);
}

void AZombieTestGameMode::FindOrSpawnDirector()
{
	for (TActorIterator<AEncounterDirector> It(GetWorld()); It; ++It)
	{
		Director = *It;
		break;
	}

	if (!Director && FallbackEncounter)
	{
		Director = GetWorld()->SpawnActor<AEncounterDirector>();
		if (Director)
		{
			Director->SetDefinition(FallbackEncounter);
		}
	}

	UE_LOG(LogZombieTest, Log, TEXT("Zombie test ready, director %s, encounter %s"), *GetNameSafe(Director), Director ? *GetNameSafe(Director->GetDefinition()) : TEXT("none"));

	// the local player may have logged in before BeginPlay, so bind a HUD that already exists
	if (HUD)
	{
		HUD->BindDirector(Director);
	}
}

void AZombieTestGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);

	if (!HUD && EncounterHUDClass && NewPlayer && NewPlayer->IsLocalController())
	{
		HUD = CreateWidget<UEncounterHUD>(NewPlayer, EncounterHUDClass);
		if (HUD)
		{
			HUD->AddToViewport(1);
			HUD->BindDirector(Director);
		}
	}

	// the style HUD is the same in every scenario
	if (!StyleHUD && NewPlayer && NewPlayer->IsLocalController())
	{
		StyleHUD = UStyleHUDWidget::CreateFor(NewPlayer);
	}

	GetWorldTimerManager().SetTimerForNextTick(this, &AZombieTestGameMode::SetupPlayer);
	GetWorldTimerManager().SetTimer(StartTimer, this, &AZombieTestGameMode::StartEncounter, FMath::Max(EncounterStartDelay, 0.01f), false);
}

void AZombieTestGameMode::SetupPlayer()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	if (!Pawn)
	{
		UE_LOG(LogZombieTest, Warning, TEXT("No player pawn to set up"));
		return;
	}

	// the nav mesh grows around the player, no level wide bake needed
	UNavigationInvokerComponent* Invoker = NewObject<UNavigationInvokerComponent>(Pawn, TEXT("NavInvoker"));
	Invoker->SetGenerationRadii(NavInvokerRadius, NavInvokerRadius + 500.0f);
	Invoker->RegisterComponent();

	if (IShooterWeaponHolder* Holder = Cast<IShooterWeaponHolder>(Pawn))
	{
		// the extra weapons first, so the starting weapon ends up in hand
		for (const TSubclassOf<AShooterWeapon>& Extra : AdditionalWeaponClasses)
		{
			if (Extra)
			{
				Holder->AddWeaponClass(Extra);
				UE_LOG(LogZombieTest, Log, TEXT("Granted weapon %s"), *GetNameSafe(Extra));
			}
		}
		if (StartingWeaponClass)
		{
			Holder->AddWeaponClass(StartingWeaponClass);
			UE_LOG(LogZombieTest, Log, TEXT("Granted starting weapon %s"), *GetNameSafe(StartingWeaponClass));
		}
	}
	else
	{
		UE_LOG(LogZombieTest, Warning, TEXT("Player pawn does not implement ShooterWeaponHolder, no weapon granted"));
	}
}

void AZombieTestGameMode::StartEncounter()
{
	if (Director)
	{
		Director->StartEncounter();
	}
	else
	{
		UE_LOG(LogZombieTest, Warning, TEXT("No encounter director to start"));
	}
}
