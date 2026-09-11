// CYB3RGUN THEGAME. The style HUD: meter, rank, combo, ammunition and Overclock, the same in every scenario.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "StyleScoringComponent.h"
#include "StyleHUDWidget.generated.h"

class APlayerController;
class UProgressBar;
class UTextBlock;

/**
 *  Reads the local player's style record, the weapon in hand through IWeaponStatusSource and Overclock from the
 *  combat feel subsystem, and shows them on two small panels: rank, meter, style points, combo, the last style
 *  event and the Overclock state at the top right; the weapon, its reload and the empty cue at the bottom left.
 *  The Overclock charge itself is the thin arc of the logo crosshair.
 *  Every scenario creates it for its local player, and every end of run summary adds its style lines.
 */
UCLASS()
class CYB3RGUN_API UStyleHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	/** Creates the style HUD for a local player and puts it on screen */
	static UStyleHUDWidget* CreateFor(APlayerController* Player);

	/** The style lines of an end of run summary: points, best rank, accuracy, best combo, pairs, headshots, rescues, penalties */
	static FText FormatRunSummary(APlayerController* Player);

	/** Writes a summary to the log on one line, for automated runs */
	static void LogSummary(const FText& Summary);

protected:

	/** Seconds the last style event line stays up before it fades */
	UPROPERTY(EditAnywhere, Category="Style HUD", meta = (ClampMin = 0.1, Units = "s"))
	float EventHoldSeconds = 1.0f;

	/** Seconds the empty cue stays after a trigger pull on an empty magazine */
	UPROPERTY(EditAnywhere, Category="Style HUD", meta = (ClampMin = 0.1, Units = "s"))
	float EmptyCueSeconds = 1.5f;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> RankText;

	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> MeterBar;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StyleText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ComboText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> EventText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> OverclockText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> WeaponText;

	/** The maker's mark beside the weapon name (D-056) */
	UPROPERTY(Transient)
	TObjectPtr<class UImage> MakerMarkImage;

	/** The mark the image shows now, so the brush is only rebuilt on a change of weapon */
	TWeakObjectPtr<class UTexture2D> ShownMark;

	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> ReloadBar;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> CueText;

	/** The projection material of this HUD's text (D-051) */
	UPROPERTY(Transient)
	TObjectPtr<class UMaterialInstanceDynamic> HoloText;

	TWeakObjectPtr<UStyleScoringComponent> BoundStyle;

	/** The last style events, one shot's events share the line, and when it was set on the wall clock */
	FText EventLine;
	double EventShownAt = -1000.0;

	/** Wall clock of the last clean event, the rank label pops for a moment */
	double RankPopAt = -1000.0;

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION()
	void HandleStyleEvent(EStyleEvent Event, int32 Points, AActor* Target);

	void BuildLayout();
	void BindStyle();
	void UpdateStyle(double WallNow);
	void UpdateWeapon(double RealTime);
	void UpdateOverclock(double WallNow);
};
