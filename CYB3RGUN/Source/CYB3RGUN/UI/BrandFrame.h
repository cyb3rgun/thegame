// CYB3RGUN THEGAME. The website's framing language for UMG: hard edged plates with a one pixel frame, corner brackets and thin rules (D-082).

#pragma once

#include "CoreMinimal.h"
#include "Components/Border.h"
#include "Components/Widget.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SLeafWidget.h"
#include "BrandFrame.generated.h"

/** A border that also draws a one pixel frame and corner brackets on top of its content */
class CYB3RGUN_API SBrandFrame : public SBorder
{
public:

	void SetFrame(const FLinearColor& InFrameColor, const FLinearColor& InTickColor, float InTickSize, bool bInTicks);

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:

	FLinearColor FrameColor = FLinearColor::Transparent;
	FLinearColor TickColor = FLinearColor::Transparent;
	float TickSize = 12.0f;
	bool bTicks = true;
};

/**
 *  A plate: the panel fill of the brand style, a one pixel frame and corner brackets at the top left and bottom right, hard
 *  edged as everything on the website. Colours come from the brand style unless an override is set.
 */
UCLASS()
class CYB3RGUN_API UBrandFrame : public UBorder
{
	GENERATED_BODY()

public:

	UBrandFrame();

	/** Draw the corner brackets; a plain frame without them for rows and small boxes */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Frame")
	bool bCornerTicks = true;

	/** Fill with the panel colour of the brand style; off keeps the brush colour set on the widget */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Frame")
	bool bPanelFill = true;

	/** Frame in the brand colour instead of the plate border, for the element that holds the focus */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Frame")
	bool bHighlighted = false;

	/** Changes the highlight and repaints */
	void SetHighlighted(bool bInHighlighted);

protected:

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void SynchronizeProperties() override;
};

/** A hairline that fades in from one end, as the rules beside the website's wordmark */
class CYB3RGUN_API SBrandRule : public SLeafWidget
{
public:

	SLATE_BEGIN_ARGS(SBrandRule) : _Length(64.0f), _FadeFromStart(true) {}
		SLATE_ARGUMENT(float, Length)
		SLATE_ARGUMENT(bool, FadeFromStart)
		SLATE_ARGUMENT(FLinearColor, Color)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override { return FVector2D(Length, 1.0f); }

private:

	float Length = 64.0f;
	bool bFadeFromStart = true;
	FLinearColor Color = FLinearColor::White;
};

/** A thin rule for UMG: transparent at one end, the brand cyan at the other; it stretches when its slot fills */
UCLASS()
class CYB3RGUN_API UBrandRule : public UWidget
{
	GENERATED_BODY()

public:

	/** Desired length; a filling slot makes it longer */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Rule")
	float Length = 64.0f;

	/** Transparent at the start and bright at the end, as the rule left of the wordmark; off mirrors it */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Rule")
	bool bFadeFromStart = true;

	/** Opacity of the brand colour at the bright end */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Rule", meta = (ClampMin = 0.0, ClampMax = 1.0))
	float Opacity = 0.75f;

protected:

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

	TSharedPtr<SBrandRule> MyRule;
};
