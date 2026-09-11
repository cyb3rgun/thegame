// CYB3RGUN THEGAME. The mark that powers on: the arc turns, the ring breathes, and the same arc spins while a level loads.

#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "Styling/SlateBrush.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SLeafWidget.h"
#include "CyberLogoWidget.generated.h"

/** How the mark moves */
enum class ECyberLogoMode : uint8
{
	/** The arc turns slowly and the ring breathes */
	Idle,
	/** The arc spins: the loading indicator */
	Loading
};

/**
 *  The CYB3RGUN mark drawn in Slate from its two alpha masks, tinted in the brand colour (D-047, D-048). When it appears it
 *  boots like a device powering on: the arc spins in and settles while the light flickers up, and the ring pops into
 *  place. Then the arc turns slowly and the ring breathes. In Loading mode the arc spins. It runs on the wall clock, so it
 *  keeps moving on the loading screen while the game thread is busy.
 */
class CYB3RGUN_API SCyberLogo : public SLeafWidget
{
public:

	SLATE_BEGIN_ARGS(SCyberLogo)
		: _Mode(ECyberLogoMode::Idle)
		, _Boot(true)
		, _MarkSize(180.0f)
	{}
		SLATE_ARGUMENT(ECyberLogoMode, Mode)
		SLATE_ARGUMENT(bool, Boot)
		SLATE_ARGUMENT(float, MarkSize)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void SetMode(ECyberLogoMode InMode) { Mode = InMode; }

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;

private:

	EActiveTimerReturnType Animate(double InCurrentTime, float InDeltaTime);

	FSlateBrush ArcBrush;
	FSlateBrush RingBrush;
	FLinearColor Tint = FLinearColor::White;
	ECyberLogoMode Mode = ECyberLogoMode::Idle;
	bool bBoot = true;
	float MarkSize = 180.0f;
	double StartTime = 0.0;

	// the turning arc keeps its own angle and speed, so a change of mode never makes it jump
	mutable double LastPaintTime = 0.0;
	mutable float ArcAngle = 0.0f;
	mutable float ArcSpeed = 0.0f;
};

/** What the movie player shows while a level loads: a dark screen and the spinning arc of the mark (D-048) */
class CYB3RGUN_API SCyberLoadingScreen : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SCyberLoadingScreen) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:

	FSlateBrush Backdrop;
};

/** The CYB3RGUN mark as a UMG widget, for the main menu */
UCLASS()
class CYB3RGUN_API UCyberLogoWidget : public UWidget
{
	GENERATED_BODY()

public:

	/** Boots up when it first appears */
	UPROPERTY(EditAnywhere, Category="Logo")
	bool bBootOnShow = true;

	/** Edge length of the mark, in slate units */
	UPROPERTY(EditAnywhere, Category="Logo", meta = (ClampMin = 16.0))
	float MarkSize = 180.0f;

	/** Spins the arc while something loads */
	void SetLoading(bool bInLoading);

	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

protected:

	virtual TSharedRef<SWidget> RebuildWidget() override;

	TSharedPtr<SCyberLogo> Logo;
	bool bLoading = false;
};
