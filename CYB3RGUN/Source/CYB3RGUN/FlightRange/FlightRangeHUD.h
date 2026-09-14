// CYB3RGUN THEGAME. HUD for the flight range: countdown, score, hits and misses, the points of each hit and the summary.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FlightRangeGameMode.h"
#include "FlightRangeHUD.generated.h"

class UTextBlock;
class UWidget;

/**
 *  Builds its own layout and binds itself to the flight range game mode. The logo crosshair and the style HUD sit beside
 *  it as in every scenario.
 */
UCLASS()
class CYB3RGUN_API UFlightRangeHUD : public UUserWidget
{
	GENERATED_BODY()

protected:

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TimeText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ScoreText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> HitsText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> EventText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ReadyText;

	UPROPERTY(Transient)
	TObjectPtr<UWidget> SummaryPanel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> SummaryText;

	/** The projection material of this HUD's text (D-051) */
	UPROPERTY(Transient)
	TObjectPtr<class UMaterialInstanceDynamic> HoloText;

	UPROPERTY(Transient)
	TObjectPtr<AFlightRangeGameMode> GameMode;

	/** Seconds the points of a hit stay fully visible before they fade */
	UPROPERTY(EditAnywhere, Category="Flight Range", meta = (ClampMin = 0.1, Units = "s"))
	float EventHoldSeconds = 0.6f;

	UPROPERTY(EditAnywhere, Category="Flight Range", meta = (ClampMin = 0.1, Units = "s"))
	float EventFadeSeconds = 0.5f;

	float EventAge = 0.0f;
	bool bEventVisible = false;
	int32 ShownHits = -1;
	int32 ShownMisses = -1;

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION()
	void HandleTimeChanged(int32 SecondsLeft);

	UFUNCTION()
	void HandleScoreChanged(int32 Score, int32 Delta);

	UFUNCTION()
	void HandleTargetScored(int32 Points, FVector Location);

	UFUNCTION()
	void HandleStateChanged(EFlightRoundState State);

	UFUNCTION()
	void HandleRoundFinished(const FFlightRangeStats& Stats);

	void BuildLayout();
	void BindGameMode();
	void UnbindGameMode();
};
