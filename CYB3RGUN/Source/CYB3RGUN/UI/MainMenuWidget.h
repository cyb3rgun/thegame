// CYB3RGUN THEGAME. The title screen: CYB3RGUN, the tagline, Play, Settings and Quit.

#pragma once

#include "CoreMinimal.h"
#include "CyberMenuScreen.h"
#include "MainMenuWidget.generated.h"

class UCyberLogoWidget;

/**
 *  Root of the menu stack in the menu level (D-036). Play opens the level selection, Settings the settings menu,
 *  Quit is the only way out of the game. Escape does nothing here. Built in code, WBP_MainMenu derives from it.
 */
UCLASS()
class CYB3RGUN_API UMainMenuWidget : public UCyberMenuScreen
{
	GENERATED_BODY()

public:

	virtual bool CanGoBack() const override { return false; }

protected:

	/** Width of the dark band on the left that carries the title and the buttons */
	UPROPERTY(EditAnywhere, Category="Menu", meta = (ClampMin = 200.0))
	float BandWidth = 760.0f;

	/** Size of the wordmark in the band */
	UPROPERTY(EditAnywhere, Category="Menu", meta = (ClampMin = 12))
	int32 WordmarkSize = 54;

	/** The mark above the name: boots up with the menu, spins while the background level streams in */
	UPROPERTY(Transient)
	TObjectPtr<UCyberLogoWidget> LogoMark;

	virtual void BuildLayout() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION()
	void HandlePlay();

	UFUNCTION()
	void HandleSettings();

	UFUNCTION()
	void HandleQuit();
};
