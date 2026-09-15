// CYB3RGUN THEGAME. The website's framing language for UMG: hard edged plates with a one pixel frame, corner brackets and thin rules (D-082).

#include "BrandFrame.h"
#include "BrandStyle.h"
#include "Components/BorderSlot.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

namespace BrandFrameDraw
{
	void Line(FSlateWindowElementList& Out, int32 Layer, const FGeometry& Geometry, const FVector2D& From, const FVector2D& To, const FLinearColor& Color)
	{
		TArray<FVector2D> Points;
		Points.Add(From);
		Points.Add(To);
		FSlateDrawElement::MakeLines(Out, Layer, Geometry.ToPaintGeometry(), Points, ESlateDrawEffect::None, Color, false, 1.0f);
	}
}

void SBrandFrame::SetFrame(const FLinearColor& InFrameColor, const FLinearColor& InTickColor, float InTickSize, bool bInTicks)
{
	FrameColor = InFrameColor;
	TickColor = InTickColor;
	TickSize = InTickSize;
	bTicks = bInTicks;
	Invalidate(EInvalidateWidgetReason::Paint);
}

int32 SBrandFrame::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
	int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	int32 Layer = SBorder::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled) + 1;

	const FVector2D Size = AllottedGeometry.GetLocalSize();
	const float Right = FMath::Max(Size.X - 0.5f, 0.5f);
	const float Bottom = FMath::Max(Size.Y - 0.5f, 0.5f);
	if (FrameColor.A > 0.0f)
	{
		TArray<FVector2D> Frame = { FVector2D(0.5f, 0.5f), FVector2D(Right, 0.5f), FVector2D(Right, Bottom), FVector2D(0.5f, Bottom), FVector2D(0.5f, 0.5f) };
		FSlateDrawElement::MakeLines(OutDrawElements, Layer, AllottedGeometry.ToPaintGeometry(), Frame, ESlateDrawEffect::None, FrameColor, false, 1.0f);
	}

	// the brackets sit on the frame at the top left and the bottom right, never on all four corners
	if (bTicks && TickColor.A > 0.0f)
	{
		++Layer;
		const float Arm = FMath::Min(TickSize, FMath::Min(Size.X, Size.Y) * 0.5f);
		BrandFrameDraw::Line(OutDrawElements, Layer, AllottedGeometry, FVector2D(0.5f, 0.5f), FVector2D(Arm, 0.5f), TickColor);
		BrandFrameDraw::Line(OutDrawElements, Layer, AllottedGeometry, FVector2D(0.5f, 0.5f), FVector2D(0.5f, Arm), TickColor);
		BrandFrameDraw::Line(OutDrawElements, Layer, AllottedGeometry, FVector2D(Right, Bottom), FVector2D(Right - Arm, Bottom), TickColor);
		BrandFrameDraw::Line(OutDrawElements, Layer, AllottedGeometry, FVector2D(Right, Bottom), FVector2D(Right, Bottom - Arm), TickColor);
	}
	return Layer;
}

UBrandFrame::UBrandFrame()
{
	// the style asset is read when the widget is built, never while the class default is constructed
	SetPadding(FMargin(24.0f, 18.0f));
}

void UBrandFrame::SetHighlighted(bool bInHighlighted)
{
	if (bHighlighted != bInHighlighted)
	{
		bHighlighted = bInHighlighted;
		SynchronizeProperties();
	}
}

TSharedRef<SWidget> UBrandFrame::RebuildWidget()
{
	MyBorder = SNew(SBrandFrame);
	if (GetChildrenCount() > 0)
	{
		Cast<UBorderSlot>(GetContentSlot())->BuildSlot(MyBorder.ToSharedRef());
	}
	return MyBorder.ToSharedRef();
}

void UBrandFrame::SynchronizeProperties()
{
	if (bPanelFill)
	{
		SetBrushColor(UBrandStyle::Get().Panel);
	}
	Super::SynchronizeProperties();

	if (MyBorder.IsValid())
	{
		const UBrandStyle& Style = UBrandStyle::Get();
		const FLinearColor Frame = bHighlighted ? Style.GetBrand() : Style.PlateBorder;
		StaticCastSharedPtr<SBrandFrame>(MyBorder)->SetFrame(Frame, Style.GetCornerTick(), Style.CornerTickSize, bCornerTicks);
	}
}

void SBrandRule::Construct(const FArguments& InArgs)
{
	Length = InArgs._Length;
	bFadeFromStart = InArgs._FadeFromStart;
	Color = InArgs._Color;
}

int32 SBrandRule::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
	int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const FVector2D Size = AllottedGeometry.GetLocalSize();
	TArray<FSlateGradientStop> Stops;
	Stops.Add(FSlateGradientStop(FVector2D(0.0f, 0.0f), bFadeFromStart ? Color.CopyWithNewOpacity(0.0f) : Color));
	Stops.Add(FSlateGradientStop(FVector2D(Size.X, 0.0f), bFadeFromStart ? Color : Color.CopyWithNewOpacity(0.0f)));
	const float Top = FMath::Max((Size.Y - 1.0f) * 0.5f, 0.0f);
	FSlateDrawElement::MakeGradient(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(FVector2f(Size.X, 1.0f), FSlateLayoutTransform(FVector2f(0.0f, Top))), Stops, Orient_Vertical);
	return LayerId;
}

TSharedRef<SWidget> UBrandRule::RebuildWidget()
{
	MyRule = SNew(SBrandRule)
		.Length(Length)
		.FadeFromStart(bFadeFromStart)
		.Color(UBrandStyle::Get().GetBrand().CopyWithNewOpacity(Opacity));
	return MyRule.ToSharedRef();
}

void UBrandRule::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	MyRule.Reset();
}
