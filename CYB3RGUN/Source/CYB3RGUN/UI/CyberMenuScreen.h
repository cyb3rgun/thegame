// CYB3RGUN THEGAME. Base of the front end screens: built in code, keeps a focused button for keyboard and gamepad.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CyberMenuScreen.generated.h"

class UButton;
class UGameMenuSubsystem;

/**
 *  One screen of the menu stack: main menu, level selection or pause menu. Builds its widget tree in code like
 *  the settings menu, the WBP_ subclasses exist for styling. Every button it registers lights up while it holds
 *  the focus, and a moving mouse takes the focus to the button under it, so keyboard, mouse and gamepad share one
 *  cursor. A mouse that merely rests over a button when the screen appears leaves the focus on the first button.
 *  Arrow keys, the d-pad and the left stick move between buttons, Enter or the gamepad's bottom button presses one.
 */
UCLASS(Abstract)
class CYB3RGUN_API UCyberMenuScreen : public UUserWidget
{
	GENERATED_BODY()

public:

	/** False for the root screen, which Escape and the back button cannot leave */
	virtual bool CanGoBack() const { return true; }

	/** Gives the focus to the screen's first button, retried for a few frames while the screen shows */
	void FocusDefault();

	UWidget* GetDefaultFocus() const;

protected:

	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> MenuButtons;

	/** Button that takes the focus when the screen shows, the first one registered */
	UPROPERTY(Transient)
	TObjectPtr<UButton> DefaultFocus;

	/** Cursor position of the last tick, a changed position is what lets the mouse take the focus */
	FVector2D LastCursorPosition = FVector2D::ZeroVector;
	bool bCursorKnown = false;
	int32 FocusAttempts = 0;

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** Fills the widget tree, called once before the Slate widget is built */
	virtual void BuildLayout() {}

	/** Makes a menu button and registers it */
	UButton* AddMenuButton(const FName& Name, const FText& Label, int32 Size = 26);

	/** Registers a button for the highlight and the mouse focus */
	void RegisterButton(UButton* Button);

	UGameMenuSubsystem* GetGameMenu() const;
};
