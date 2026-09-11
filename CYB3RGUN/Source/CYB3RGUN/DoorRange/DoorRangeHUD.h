// CYB3RGUN THEGAME. HUD for the door range.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DoorTypes.h"
#include "DoorRangeHUD.generated.h"

class UTextBlock;
class UWidget;
class ADoorRangeGameMode;
class ADoorSlot;

/**
 *  Shows score, wave, hostiles remaining, a fading event line and the end of range summary.
 *  Binds itself to the door range game mode delegates when constructed.
 *  Layout lives in WBP_DoorRangeHUD; the named widgets below are optional, so the HUD
 *  still works and builds a plain fallback layout when a widget is missing.
 */
UCLASS()
class CYB3RGUN_API UDoorRangeHUD : public UUserWidget
{
	GENERATED_BODY()

protected:

	UPROPERTY(BlueprintReadOnly, Category="Door Range", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ScoreText;

	UPROPERTY(BlueprintReadOnly, Category="Door Range", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> WaveText;

	UPROPERTY(BlueprintReadOnly, Category="Door Range", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HostilesText;

	UPROPERTY(BlueprintReadOnly, Category="Door Range", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> EventText;

	UPROPERTY(BlueprintReadOnly, Category="Door Range", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> SummaryPanel;

	UPROPERTY(BlueprintReadOnly, Category="Door Range", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SummaryText;

	/** The projection material of this HUD's text (D-051) */
	UPROPERTY(Transient)
	TObjectPtr<class UMaterialInstanceDynamic> HoloText;

	/** Seconds an event line stays fully visible before it fades */
	UPROPERTY(EditAnywhere, Category="Door Range", meta = (ClampMin = 0.1, Units = "s"))
	float EventHoldSeconds = 1.2f;

	/** Seconds the event line takes to fade out */
	UPROPERTY(EditAnywhere, Category="Door Range", meta = (ClampMin = 0.1, Units = "s"))
	float EventFadeSeconds = 0.8f;

	UPROPERTY(Transient)
	TObjectPtr<ADoorRangeGameMode> GameMode;

	float EventAge = 0.0f;
	bool bEventVisible = false;

public:

	/** Rebinds to the game mode and refreshes every field */
	UFUNCTION(BlueprintCallable, Category="Door Range")
	void RefreshAll();

protected:

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual TSharedRef<SWidget> RebuildWidget() override;

	UFUNCTION()
	void HandleScoreChanged(int32 Score, int32 Delta);

	UFUNCTION()
	void HandleWaveChanged(int32 Wave, int32 WaveCount);

	UFUNCTION()
	void HandleHostilesChanged(int32 Remaining, int32 Total);

	UFUNCTION()
	void HandleRangeEvent(EDoorRangeEvent Event, int32 Delta, ADoorSlot* SourceSlot);

	UFUNCTION()
	void HandleRangeFinished(const FDoorRangeStats& Stats);

	void BindGameMode();
	void UnbindGameMode();
	void ShowEvent(const FText& Text, const FLinearColor& Color);
	void BuildFallbackLayout();
	static void SetTextSafe(UTextBlock* Block, const FText& Text);
};
