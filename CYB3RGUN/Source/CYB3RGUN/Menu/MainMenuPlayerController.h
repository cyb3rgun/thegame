// CYB3RGUN THEGAME. The player controller of the main menu: a cursor, and the menu camera as the view.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MainMenuPlayerController.generated.h"

/**
 *  Controls nothing in the world. Shows the mouse cursor and looks through the menu camera rig placed in the
 *  level, or through one it spawns at the level origin when the level has none, and opens the main menu.
 */
UCLASS()
class CYB3RGUN_API AMainMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

public:

	AMainMenuPlayerController();

protected:

	virtual void BeginPlay() override;
};
