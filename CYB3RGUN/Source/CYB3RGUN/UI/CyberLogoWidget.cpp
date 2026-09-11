// CYB3RGUN THEGAME. The mark that powers on: the arc turns, the ring breathes, and the same arc spins while a level loads.

#include "CyberLogoWidget.h"
#include "BrandStyle.h"
#include "CyberMenuStyle.h"
#include "Brushes/SlateColorBrush.h"
#include "Engine/Texture2D.h"
#include "HAL/PlatformTime.h"
#include "Rendering/DrawElements.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

// a named namespace with unique names, unity builds merge this file with others
namespace CyberLogoLook
{
	/** Turn of the arc, degrees per second, at rest and while loading */
	constexpr float IdleSpeed = 8.0f;
	constexpr float LoadingSpeed = 280.0f;

	/** Seconds the mark takes to power on */
	constexpr float BootSeconds = 1.4f;

	/** How much and how slowly the ring breathes */
	constexpr float BreathAmount = 0.035f;
	constexpr float BreathPeriod = 3.2f;

	/** The glow is the mark again, a little larger and faint */
	constexpr float GlowScale = 1.06f;
	constexpr float GlowOpacity = 0.22f;

	/** Arc and ring share this centre in the logo frame: 521 of 1024 */
	constexpr float Centre = 0.5088f;
}

void SCyberLogo::Construct(const FArguments& InArgs)
{
	const UBrandStyle& Style = UBrandStyle::Get();
	auto UseMask = [](FSlateBrush& Brush, UTexture2D* Texture)
	{
		Brush.SetResourceObject(Texture);
		Brush.ImageSize = FVector2D(1024.0, 1024.0);
		Brush.Tiling = ESlateBrushTileType::NoTile;
		// without its texture the brush would draw a white square, so it draws nothing
		Brush.DrawAs = Texture ? ESlateBrushDrawType::Image : ESlateBrushDrawType::NoDrawType;
	};
	UseMask(ArcBrush, Style.LogoArc);
	UseMask(RingBrush, Style.LogoRing);
	Tint = Style.GetBrand();

	Mode = InArgs._Mode;
	bBoot = InArgs._Boot;
	MarkSize = InArgs._MarkSize;
	StartTime = FPlatformTime::Seconds();
	ArcSpeed = Mode == ECyberLogoMode::Loading ? CyberLogoLook::LoadingSpeed : CyberLogoLook::IdleSpeed;

	RegisterActiveTimer(0.0f, FWidgetActiveTimerDelegate::CreateSP(this, &SCyberLogo::Animate));
}

EActiveTimerReturnType SCyberLogo::Animate(double InCurrentTime, float InDeltaTime)
{
	Invalidate(EInvalidateWidgetReason::Paint);
	return EActiveTimerReturnType::Continue;
}

FVector2D SCyberLogo::ComputeDesiredSize(float LayoutScaleMultiplier) const
{
	return FVector2D(MarkSize, MarkSize);
}

int32 SCyberLogo::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const double Now = FPlatformTime::Seconds();
	const float Age = static_cast<float>(Now - StartTime);
	const float Delta = LastPaintTime > 0.0 ? FMath::Min(static_cast<float>(Now - LastPaintTime), 0.1f) : 0.0f;
	LastPaintTime = Now;

	// the arc turns slowly and spins while something loads, the speed eases so a change never jumps
	const float TargetSpeed = Mode == ECyberLogoMode::Loading ? CyberLogoLook::LoadingSpeed : CyberLogoLook::IdleSpeed;
	ArcSpeed = FMath::FInterpTo(ArcSpeed, TargetSpeed, Delta, 3.0f);
	ArcAngle = FMath::Fmod(ArcAngle + ArcSpeed * Delta, 360.0f);

	// boot: the arc spins in and settles, the ring pops into place, the light flickers up like a tube starting
	float BootTurn = 0.0f;
	float RingScale = 1.0f;
	float Light = 1.0f;
	if (bBoot && Age < CyberLogoLook::BootSeconds)
	{
		const float T = FMath::Clamp(Age / CyberLogoLook::BootSeconds, 0.0f, 1.0f);
		BootTurn = -540.0f * FMath::Pow(1.0f - T, 3.0f);
		RingScale = T < 0.55f ? FMath::Lerp(0.2f, 1.12f, T / 0.55f) : FMath::Lerp(1.12f, 1.0f, (T - 0.55f) / 0.45f);
		const uint32 Step = static_cast<uint32>(Age * 30.0f) * 2654435761u;
		const bool bFlickerDark = T < 0.7f && ((Step >> 16) & 0xFF) < 90;
		Light = FMath::Clamp(T * 1.6f, 0.0f, 1.0f) * (bFlickerDark ? 0.25f : 1.0f);
	}

	// the ring breathes
	const float Breath = 1.0f + CyberLogoLook::BreathAmount * FMath::Sin(Age * 2.0f * UE_PI / CyberLogoLook::BreathPeriod);

	const FVector2f Size(AllottedGeometry.GetLocalSize());
	const float Side = FMath::Min(Size.X, Size.Y);
	const FVector2f Origin((Size.X - Side) * 0.5f, (Size.Y - Side) * 0.5f);
	const float Opacity = Light * InWidgetStyle.GetColorAndOpacityTint().A;
	const FLinearColor Color = Tint.CopyWithNewOpacity(Tint.A * Opacity);
	const FLinearColor Glow = Tint.CopyWithNewOpacity(Tint.A * Opacity * CyberLogoLook::GlowOpacity);
	const float Angle = FMath::DegreesToRadians(ArcAngle + BootTurn);

	// both parts scale about the shared centre of the mark, the arc also turns about it
	auto Frame = [&](float Scale, FVector2f& OutAt) -> float
	{
		const float Edge = Side * Scale;
		const float Shift = (Side - Edge) * CyberLogoLook::Centre;
		OutAt = Origin + FVector2f(Shift, Shift);
		return Edge;
	};
	auto DrawArc = [&](float Scale, const FLinearColor& InColor, int32 Layer)
	{
		FVector2f At;
		const float Edge = Frame(Scale, At);
		FSlateDrawElement::MakeRotatedBox(OutDrawElements, Layer, AllottedGeometry.ToPaintGeometry(FVector2f(Edge, Edge), FSlateLayoutTransform(1.0f, At)),
			&ArcBrush, ESlateDrawEffect::None, Angle, TOptional<FVector2f>(FVector2f(Edge * CyberLogoLook::Centre, Edge * CyberLogoLook::Centre)), FSlateDrawElement::RelativeToElement, InColor);
	};
	auto DrawRing = [&](float Scale, const FLinearColor& InColor, int32 Layer)
	{
		FVector2f At;
		const float Edge = Frame(Scale, At);
		FSlateDrawElement::MakeBox(OutDrawElements, Layer, AllottedGeometry.ToPaintGeometry(FVector2f(Edge, Edge), FSlateLayoutTransform(1.0f, At)), &RingBrush, ESlateDrawEffect::None, InColor);
	};

	DrawArc(CyberLogoLook::GlowScale, Glow, LayerId);
	DrawRing(RingScale * Breath * CyberLogoLook::GlowScale, Glow, LayerId);
	DrawArc(1.0f, Color, LayerId + 1);
	DrawRing(RingScale * Breath, Color, LayerId + 1);
	return LayerId + 1;
}

void SCyberLoadingScreen::Construct(const FArguments& InArgs)
{
	Backdrop = FSlateColorBrush(FCyberMenuStyle::PanelSolidColor().CopyWithNewOpacity(1.0f));
	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(&Backdrop)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			[
				SNew(SCyberLogo)
				.Mode(ECyberLogoMode::Loading)
				.Boot(false)
				.MarkSize(160.0f)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(0.0f, 24.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock)
				.Text(NSLOCTEXT("CyberLogo", "Loading", "LOADING"))
				.Font(FCyberMenuStyle::MakeFont(18, TEXT("Bold"), 320))
				.ColorAndOpacity(FCyberMenuStyle::TextColor())
			]
		]
	];
}

TSharedRef<SWidget> UCyberLogoWidget::RebuildWidget()
{
	Logo = SNew(SCyberLogo)
		.Mode(bLoading ? ECyberLogoMode::Loading : ECyberLogoMode::Idle)
		.Boot(bBootOnShow)
		.MarkSize(MarkSize);
	return Logo.ToSharedRef();
}

void UCyberLogoWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	Logo.Reset();
}

void UCyberLogoWidget::SetLoading(bool bInLoading)
{
	bLoading = bInLoading;
	if (Logo.IsValid())
	{
		Logo->SetMode(bLoading ? ECyberLogoMode::Loading : ECyberLogoMode::Idle);
	}
}
