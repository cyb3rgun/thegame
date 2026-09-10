// CYB3RGUN THEGAME. Opens the settings menu from any level with one key.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SettingsMenuSubsystem.generated.h"

class USettingsMenuWidget;
class FSettingsMenuInputProcessor;

/**
 *  F10 or the gamepad menu button toggles the settings menu in every level, whatever controller the level uses:
 *  the key is read by a Slate input pre processor, not by a level's input mapping. While the menu is open
 *  the game pauses and the mouse cursor shows.
 */
UCLASS(Config=Game)
class CYB3RGUN_API USettingsMenuSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

protected:

	/** Widget to open. Falls back to the code built menu. */
	UPROPERTY(Config)
	TSoftClassPtr<USettingsMenuWidget> MenuClass;

	UPROPERTY(Transient)
	TObjectPtr<USettingsMenuWidget> Menu;

	TSharedPtr<FSettingsMenuInputProcessor> InputProcessor;
	bool bPausedByMenu = false;

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category="Settings")
	void OpenMenu();

	UFUNCTION(BlueprintCallable, Category="Settings")
	void CloseMenu();

	UFUNCTION(BlueprintCallable, Category="Settings")
	void ToggleMenu();

	UFUNCTION(BlueprintPure, Category="Settings")
	bool IsMenuOpen() const { return Menu != nullptr; }

	USettingsMenuWidget* GetMenu() const { return Menu; }

	/** True while a game world of this instance is running, so editor key presses are left alone */
	bool IsPlaying() const;
};
