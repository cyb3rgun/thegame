// CYB3RGUN THEGAME. Crosshair drawn wherever the aim component says the crosshair is.

#include "RailCrosshairWidget.h"
#include "RailAimComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Rendering/DrawElements.h"

TSharedRef<SWidget> URailCrosshairWidget::RebuildWidget()
{
	// an empty canvas gives the overlay the full viewport to paint in
	if (WidgetTree && !WidgetTree->RootWidget && !HasAnyFlags(RF_ClassDefaultObject))
	{
		WidgetTree->RootWidget = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CrosshairCanvas"));
	}
	return Super::RebuildWidget();
}

void URailCrosshairWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void URailCrosshairWidget::NativeDestruct()
{
	UnbindAim();
	Super::NativeDestruct();
}

void URailCrosshairWidget::BindAim(URailAimComponent* InAim)
{
	UnbindAim();
	Aim = InAim;
	if (InAim)
	{
		InAim->OnShotFired.AddUniqueDynamic(this, &URailCrosshairWidget::HandleShotFired);
	}
}

void URailCrosshairWidget::UnbindAim()
{
	if (URailAimComponent* Current = Aim.Get())
	{
		Current->OnShotFired.RemoveDynamic(this, &URailCrosshairWidget::HandleShotFired);
	}
	Aim.Reset();
}

void URailCrosshairWidget::HandleShotFired(bool bHit, const FHitResult& Hit, AActor* DamagedActor)
{
	if (DamagedActor)
	{
		HitFlash = 1.0f;
	}
}

void URailCrosshairWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	HitFlash = FMath::Max(HitFlash - InDeltaTime / HitFlashSeconds, 0.0f);
}

int32 URailCrosshairWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const int32 Layer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	const URailAimComponent* Current = Aim.Get();
	if (!Current)
	{
		return Layer;
	}

	// normalised position scales with the widget, so DPI never shifts the crosshair off the trace
	const FVector2f Size = FVector2f(AllottedGeometry.GetLocalSize());
	const FVector2D Normalized = Current->GetCrosshairNormalized();
	const FVector2f Center(Size.X * Normalized.X, Size.Y * Normalized.Y);

	const bool bBlocked = Current->IsFireBlocked();
	const FLinearColor Tint = bBlocked ? BlockedColor : FMath::Lerp(Color, HitColor, HitFlash);
	const float Spread = Gap + (bBlocked ? 0.0f : HitFlash * 6.0f);

	auto Line = [&](const FVector2f& From, const FVector2f& To)
	{
		TArray<FVector2f> Points;
		Points.Add(Center + From);
		Points.Add(Center + To);
		FSlateDrawElement::MakeLines(OutDrawElements, Layer + 1, AllottedGeometry.ToPaintGeometry(), MoveTemp(Points), ESlateDrawEffect::None, Tint, true, Thickness);
	};

	Line(FVector2f(-Spread - ArmLength, 0.0f), FVector2f(-Spread, 0.0f));
	Line(FVector2f(Spread, 0.0f), FVector2f(Spread + ArmLength, 0.0f));
	Line(FVector2f(0.0f, -Spread - ArmLength), FVector2f(0.0f, -Spread));
	Line(FVector2f(0.0f, Spread), FVector2f(0.0f, Spread + ArmLength));

	// centre dot
	Line(FVector2f(-1.0f, 0.0f), FVector2f(1.0f, 0.0f));

	return Layer + 1;
}
