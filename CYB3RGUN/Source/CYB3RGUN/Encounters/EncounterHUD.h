// CYB3RGUN THEGAME. HUD for encounters: wave, alive, kills, events and the end summary.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EncounterHUD.generated.h"

class UTextBlock;
class UWidget;
class AEncounterDirector;
class ACyberEnemy;

/**
 *  Same pattern as the door range HUD: named optional widgets supplied by WBP_EncounterHUD,
 *  a code built fallback layout when they are missing, bound to the encounter director delegates.
 */
UCLASS()
class CYB3RGUN_API UEncounterHUD : public UUserWidget
{
	GENERATED_BODY()

protected:

	UPROPERTY(BlueprintReadOnly, Category="Encounter", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> WaveText;

	UPROPERTY(BlueprintReadOnly, Category="Encounter", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> AliveText;

	UPROPERTY(BlueprintReadOnly, Category="Encounter", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> KillsText;

	UPROPERTY(BlueprintReadOnly, Category="Encounter", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> EventText;

	UPROPERTY(BlueprintReadOnly, Category="Encounter", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> SummaryPanel;

	UPROPERTY(BlueprintReadOnly, Category="Encounter", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SummaryText;

	UPROPERTY(EditAnywhere, Category="Encounter", meta = (ClampMin = 0.1, Units = "s"))
	float EventHoldSeconds = 1.2f;

	UPROPERTY(EditAnywhere, Category="Encounter", meta = (ClampMin = 0.1, Units = "s"))
	float EventFadeSeconds = 0.8f;

	UPROPERTY(Transient)
	TObjectPtr<AEncounterDirector> Director;

	float EventAge = 0.0f;
	bool bEventVisible = false;

public:

	/** Binds to a director and refreshes every field */
	UFUNCTION(BlueprintCallable, Category="Encounter")
	void BindDirector(AEncounterDirector* InDirector);

protected:

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual TSharedRef<SWidget> RebuildWidget() override;

	UFUNCTION()
	void HandleWaveStarted(int32 Wave, int32 WaveCount, FText WaveName);

	UFUNCTION()
	void HandleCountsChanged(int32 Alive, int32 Kills, int32 Total);

	UFUNCTION()
	void HandleEnemyKilled(ACyberEnemy* Enemy, int32 Kills);

	UFUNCTION()
	void HandleFinished(int32 Kills, float Seconds);

	void UnbindDirector();
	void RefreshAll();
	void ShowEvent(const FText& Text, const FLinearColor& Color);
	void BuildFallbackLayout();
	static void SetTextSafe(UTextBlock* Block, const FText& Text);
};
