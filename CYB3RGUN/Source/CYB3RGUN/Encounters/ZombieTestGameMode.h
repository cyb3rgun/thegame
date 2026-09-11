// CYB3RGUN THEGAME. Game mode for the zombie test arena.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "StyleScenario.h"
#include "ZombieTestGameMode.generated.h"

class AEncounterDirector;
class AShooterWeapon;
class UEncounterHUD;
class UEncounterDefinition;
class UStyleSettings;

/**
 *  Hands the player the pistol, gives the pawn a navigation invoker so the nav mesh grows around it,
 *  creates the encounter HUD and starts the level's encounter director.
 */
UCLASS()
class CYB3RGUN_API AZombieTestGameMode : public AGameModeBase, public IStyleScenario
{
	GENERATED_BODY()

protected:

	/** Weapon handed to the player at spawn */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Zombie Test")
	TSubclassOf<AShooterWeapon> StartingWeaponClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Zombie Test")
	TSubclassOf<UEncounterHUD> EncounterHUDClass;

	/** Used when the level has no director of its own */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Zombie Test")
	TObjectPtr<UEncounterDefinition> FallbackEncounter;

	/** Seconds after the player spawns before the encounter starts, gives the nav mesh time to build */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Zombie Test", meta = (ClampMin = 0.0, Units = "s"))
	float EncounterStartDelay = 2.0f;

	/** Nav mesh generation radius around the player */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Zombie Test", meta = (ClampMin = 500.0, Units = "cm"))
	float NavInvokerRadius = 4000.0f;

	/** Style values of this scenario, empty uses the project default */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Zombie Test")
	TObjectPtr<UStyleSettings> StyleSettings;

	UPROPERTY(Transient)
	TObjectPtr<AEncounterDirector> Director;

	UPROPERTY(Transient)
	TObjectPtr<UEncounterHUD> HUD;

	FTimerHandle StartTimer;

public:

	UFUNCTION(BlueprintPure, Category="Zombie Test")
	AEncounterDirector* GetDirector() const { return Director; }

	//~ Begin IStyleScenario
	virtual const UStyleSettings* GetStyleSettings() const override { return StyleSettings; }
	//~ End IStyleScenario

protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

	void FindOrSpawnDirector();
	void SetupPlayer();
	void StartEncounter();
};
