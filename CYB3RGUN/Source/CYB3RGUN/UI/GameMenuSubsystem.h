// CYB3RGUN THEGAME. The front end menu stack and the keys that walk it.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameMenuSubsystem.generated.h"

class APlayerController;
class UCyberMenuScreen;
class UPlayableLevelDefinition;
class FGameMenuInputProcessor;
struct FKeyEvent;

/**
 *  Keeps the open menu screens as a stack: the main menu at the bottom in the menu level with the level
 *  selection on top of it. Escape and the gamepad back button leave the top screen but never the root main
 *  menu, so Escape never quits the game (D-036). The settings menu (F10) opens over any screen and returns to
 *  it. Keys are read by a Slate input pre processor, like the settings menu.
 */
UCLASS(Config=Game)
class CYB3RGUN_API UGameMenuSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

protected:

	/** Screens to open, each falls back to its code built class */
	UPROPERTY(Config)
	TSoftClassPtr<UCyberMenuScreen> MainMenuClass;

	UPROPERTY(Config)
	TSoftClassPtr<UCyberMenuScreen> LevelSelectClass;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UCyberMenuScreen>> Stack;

	TSharedPtr<FGameMenuInputProcessor> InputProcessor;
	FDelegateHandle PreLoadMapHandle;
	bool bPausedByMenu = false;

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Clears the stack and opens the main menu as its root, the menu level's player controller calls it */
	void ShowMainMenu();

	void OpenLevelSelect();

	/** Opens the settings menu over the current screen */
	UFUNCTION(BlueprintCallable, Category="Menu")
	void OpenSettings();

	/** Leaves the top screen, the root main menu stays. False when there was nothing to leave. */
	UFUNCTION(BlueprintCallable, Category="Menu")
	bool Back();

	void StartLevel(const UPlayableLevelDefinition* Level);

	/** The only way out of the game, the main menu's Quit button */
	void QuitGame();

	bool HasScreens() const { return GetTopScreen() != nullptr; }
	UCyberMenuScreen* GetTopScreen() const;

	/** Hide the top screen while the settings menu covers it, and bring it back with the focus afterwards */
	void SuspendTop();
	void ResumeTop();

	/** False in the menu level, where nothing pauses */
	bool IsGameplayWorld() const;

	/** True while a game world of this instance is running, so editor key presses are left alone */
	bool IsPlaying() const;

	/** Called by the input pre processor, true when the key was used */
	bool HandleKey(const FKeyEvent& KeyEvent);

protected:

	UCyberMenuScreen* PushScreen(const TSoftClassPtr<UCyberMenuScreen>& ScreenClass, UClass* FallbackClass);
	void ClearStack();
	void ApplyMenuInput(UCyberMenuScreen* Screen);
	void ApplyGameInput();
	bool IsViewportFocused() const;
	APlayerController* GetPlayerController() const;
	void HandlePreLoadMap(const FString& MapName);
};
