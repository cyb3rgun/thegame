// CYB3RGUN THEGAME. The crosshair is the logo: the ring aims, the arc counts rounds, the thin arc holds Overclock (D-048).

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "StyleScoringComponent.h"
#include "LogoCrosshairWidget.generated.h"

class APlayerController;
class UImage;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class URailAimComponent;

/**
 *  Full screen, hit test invisible overlay that draws the CYB3RGUN mark wherever the aim component puts the crosshair
 *  (D-048). The ring is the reticle: it tightens and turns from cyan to red while a hostile draws, stays tight while it
 *  can shoot, pulses on a hit, flashes on a headshot and collapses in red on a hit on a hostage or bystander. The outer
 *  arc is the gauge, one segment per round, or the reload closing it in cover. The thin arc outside is the Overclock
 *  charge. One material, M_UI_LogoCrosshair, draws all of it in the colours of MPC_Brand. The door range, the zombie test
 *  and the rail use this widget.
 */
UCLASS()
class CYB3RGUN_API ULogoCrosshairWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	/**
	 *  Creates the crosshair for a local player and puts it on screen, between the scenario HUD and the style HUD.
	 *  PlayerIndex picks the reticle's colour, so several players on one screen can tell theirs apart (D-096).
	 */
	static ULogoCrosshairWidget* CreateFor(APlayerController* Player, int32 InPlayerIndex = 0);

	/** The colour of a player's reticle. Four are prepared; further indices wrap around. */
	static FLinearColor GetPlayerColour(int32 InPlayerIndex);

	/** How many colours are prepared */
	static int32 GetPlayerColourCount();

	/** The player this reticle belongs to, counted from zero */
	int32 GetPlayerIndex() const { return PlayerIndex; }

	/** Follows this aim component. Without one the widget finds the owning pawn's own. */
	UFUNCTION(BlueprintCallable, Category="Crosshair")
	void BindAim(URailAimComponent* InAim);

protected:

	/** The material that draws the mark */
	UPROPERTY(EditAnywhere, Category="Crosshair")
	TSoftObjectPtr<UMaterialInterface> Material = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/CYB3RGUN/UI/Materials/M_UI_LogoCrosshair.M_UI_LogoCrosshair")));

	/** Edge length of the mark on screen, in slate units */
	UPROPERTY(EditAnywhere, Category="Crosshair", meta = (ClampMin = 32.0))
	float MarkSize = 132.0f;

	/** Seconds a hit pulse takes to fade */
	UPROPERTY(EditAnywhere, Category="Crosshair", meta = (ClampMin = 0.02, Units = "s"))
	float PulseSeconds = 0.18f;

	/** Seconds the headshot flash takes to fade, short and sharp */
	UPROPERTY(EditAnywhere, Category="Crosshair", meta = (ClampMin = 0.02, Units = "s"))
	float FlashSeconds = 0.12f;

	/** The red collapse after a hit on a hostage or bystander is held this long, then released */
	UPROPERTY(EditAnywhere, Category="Crosshair", meta = (ClampMin = 0.0, Units = "s"))
	float CollapseHoldSeconds = 0.3f;

	UPROPERTY(EditAnywhere, Category="Crosshair", meta = (ClampMin = 0.02, Units = "s"))
	float CollapseFadeSeconds = 0.5f;

	/** How fast the ring follows the threat level, per second, fast enough to be tight when a draw completes */
	UPROPERTY(EditAnywhere, Category="Crosshair", meta = (ClampMin = 1.0))
	float TelegraphFollowRate = 20.0f;

	/** At or below this share of the magazine the gauge turns toward red */
	UPROPERTY(EditAnywhere, Category="Crosshair", meta = (ClampMin = 0.0, ClampMax = 1.0))
	float LowAmmoShare = 0.25f;

	/** Magazines larger than this fill the gauge as one bar instead of one segment per round */
	UPROPERTY(EditAnywhere, Category="Crosshair", meta = (ClampMin = 1))
	int32 MaxSegments = 24;

	/** Opacity of the mark while firing is blocked, in cover */
	UPROPERTY(EditAnywhere, Category="Crosshair", meta = (ClampMin = 0.0, ClampMax = 1.0))
	float BlockedOpacity = 0.7f;

	UPROPERTY(Transient)
	TObjectPtr<UImage> Mark;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> MarkMaterial;

	/** Which player this reticle belongs to; it decides the colour (D-096) */
	UPROPERTY(EditAnywhere, Category="Crosshair", meta = (ClampMin = 0))
	int32 PlayerIndex = 0;

	TWeakObjectPtr<URailAimComponent> Aim;
	TWeakObjectPtr<UStyleScoringComponent> BoundStyle;

	/** Wall clock of the last pulse, flash and collapse, so a hit stop never freezes the feedback */
	double PulseAt = -1000.0;
	double FlashAt = -1000.0;
	double CollapseAt = -1000.0;
	double LastUpdateAt = 0.0;

	/** The ring's threat level as drawn, following the strongest reported threat */
	float Telegraph = 0.0f;

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION()
	void HandleStyleEvent(EStyleEvent Event, int32 Points, AActor* Target);

	void BindSources();
	void UpdateMark(const FGeometry& MyGeometry);
};
