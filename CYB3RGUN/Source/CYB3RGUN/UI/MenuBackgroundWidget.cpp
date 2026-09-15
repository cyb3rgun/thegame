// CYB3RGUN THEGAME. The front end's background: rain running down glass in front of the night range, a faint cyan glow behind it.

#include "MenuBackgroundWidget.h"
#include "BrandStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"

DEFINE_LOG_CATEGORY_STATIC(LogMenuBackground, Log, All);

namespace MenuBackgroundLayers
{
	/** Under the menu screens, which start at 900 */
	constexpr int32 ZOrder = 800;
}

UMenuBackgroundWidget* UMenuBackgroundWidget::CreateFor(APlayerController* Player)
{
	if (!Player || !Player->IsLocalController())
	{
		return nullptr;
	}
	UMenuBackgroundWidget* Widget = CreateWidget<UMenuBackgroundWidget>(Player, UMenuBackgroundWidget::StaticClass());
	if (Widget)
	{
		Widget->AddToViewport(MenuBackgroundLayers::ZOrder);
	}
	return Widget;
}

TSharedRef<SWidget> UMenuBackgroundWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget && !HasAnyFlags(RF_ClassDefaultObject))
	{
		UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("BackgroundCanvas"));
		WidgetTree->RootWidget = Canvas;
		Glass = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Glass"));
		if (UCanvasPanelSlot* GlassSlot = Canvas->AddChildToCanvas(Glass))
		{
			GlassSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
			GlassSlot->SetOffsets(FMargin(0.0f));
		}
	}
	return Super::RebuildWidget();
}

void UMenuBackgroundWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::HitTestInvisible);

	const UBrandStyle& Style = UBrandStyle::Get();
	if (Glass && Style.MenuBackgroundMaterial)
	{
		GlassMaterial = UMaterialInstanceDynamic::Create(Style.MenuBackgroundMaterial, this);
		GlassMaterial->SetVectorParameterValue(TEXT("Ground"), Style.Ground);
		Glass->SetBrushFromMaterial(GlassMaterial);
	}
	else
	{
		UE_LOG(LogMenuBackground, Warning, TEXT("No menu background material in the brand style, the menu shows the scene without glass"));
	}
}

void UMenuBackgroundWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// the drops stay round at any window shape
	const FVector2D Size = MyGeometry.GetLocalSize();
	const float Aspect = Size.Y > 1.0f ? static_cast<float>(Size.X / Size.Y) : 1.0f;
	if (GlassMaterial && !FMath::IsNearlyEqual(Aspect, ShownAspect, 0.001f))
	{
		ShownAspect = Aspect;
		GlassMaterial->SetScalarParameterValue(TEXT("Aspect"), Aspect);
	}
}
