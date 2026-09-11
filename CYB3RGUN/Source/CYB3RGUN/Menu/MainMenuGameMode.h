// CYB3RGUN THEGAME. The front end: a lit night scene behind the main menu, no player pawn.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MainMenuGameMode.generated.h"

class ULevelStreamingDynamic;
class UMaterialInterface;

/**
 *  Game mode of Lvl_MainMenu (D-036, D-038). Streams a playable level in behind the menu, so the menu shows the
 *  real night scene with its lights, fog and grade, and opens that level's doors on its own now and then so the
 *  screen is alive. Nobody plays here: there is no pawn, the menu camera rig is the view.
 */
UCLASS()
class CYB3RGUN_API AMainMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:

	AMainMenuGameMode();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual bool PlayerCanRestart_Implementation(APlayerController* Player) override;

	/** True once the streamed background level is on screen, or when there is none */
	bool IsBackgroundShown() const;

protected:

	/** Level streamed in behind the menu. Empty keeps the menu level as it is. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Menu")
	TSoftObjectPtr<UWorld> BackgroundLevel;

	/** Seconds between two background doors opening on their own, 0 keeps every door shut */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Menu|Doors", meta = (ClampMin = 0.0, Units = "s"))
	float AmbientDoorInterval = 3.0f;

	/** Share of the background openings that show a hostile, the rest show a friendly */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Menu|Doors", meta = (ClampMin = 0.0, ClampMax = 1.0))
	float AmbientHostileShare = 0.6f;

	/** Seconds an occupant stays in view with its door open */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Menu|Doors", meta = (ClampMin = 0.5, Units = "s"))
	float AmbientExposure = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Menu|Doors")
	TSoftObjectPtr<UMaterialInterface> AmbientHostileMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Menu|Doors")
	TSoftObjectPtr<UMaterialInterface> AmbientFriendlyMaterial;

	UPROPERTY(Transient)
	TObjectPtr<ULevelStreamingDynamic> Background;

	FTimerHandle AmbientTimer;

	/** Opens one closed background door with an occupant, the door closes again by itself */
	void OpenAmbientDoor();
};
