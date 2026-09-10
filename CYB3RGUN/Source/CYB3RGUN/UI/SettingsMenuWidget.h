// CYB3RGUN THEGAME. The graphics settings menu: preset, every option, reset, apply and a live frame rate readout.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CyberSettingsTypes.h"
#include "SettingsMenuWidget.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;
class USettingsMenuRow;

/**
 *  Edits a pending copy of the settings; nothing reaches the engine until Apply. Changing the preset
 *  resets every option to that preset's defaults on screen. The experimental rows only appear on Ultra.
 *  Built in code like the HUDs, WBP_SettingsMenu derives from it for styling.
 */
UCLASS()
class CYB3RGUN_API USettingsMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:

	UPROPERTY(Transient)
	TArray<TObjectPtr<USettingsMenuRow>> Rows;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> RowBox;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> FrameRateText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StatusText;

	UPROPERTY(Transient)
	TObjectPtr<UButton> ResetButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> ApplyButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> CloseButton;

	/** Seconds between frame rate readout updates */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Settings", meta = (ClampMin = 0.05))
	float FrameRateUpdateSeconds = 0.25f;

	FCyberSettingsState Pending;
	float FrameAccumulator = 0.0f;
	int32 FrameCount = 0;

public:

	/** Moves an option one value forward or back, wrapping around */
	void StepOption(ECyberSettingOption Option, int32 Direction);

	UFUNCTION(BlueprintCallable, Category="Settings")
	void ApplyPending();

	UFUNCTION(BlueprintCallable, Category="Settings")
	void ResetToDefaults();

	UFUNCTION(BlueprintCallable, Category="Settings")
	void CloseMenu();

	const FCyberSettingsState& GetPending() const { return Pending; }

protected:

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	void BuildLayout();
	void RefreshRows();
	void SetStatus(const FText& Text);
};
