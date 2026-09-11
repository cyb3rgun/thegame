// CYB3RGUN THEGAME. The pause menu: Resume, Settings and Return to Main Menu.

#pragma once

#include "CoreMinimal.h"
#include "CyberMenuScreen.h"
#include "PauseMenuWidget.generated.h"

/**
 *  Opened by Escape in a gameplay level, which pauses while it shows. Escape or Resume closes it and the level
 *  runs on. Built in code, WBP_PauseMenu derives from it.
 */
UCLASS()
class CYB3RGUN_API UPauseMenuWidget : public UCyberMenuScreen
{
	GENERATED_BODY()

protected:

	virtual void BuildLayout() override;

	UFUNCTION()
	void HandleResume();

	UFUNCTION()
	void HandleSettings();

	UFUNCTION()
	void HandleMainMenu();
};
