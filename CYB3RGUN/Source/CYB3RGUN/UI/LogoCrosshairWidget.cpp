// CYB3RGUN THEGAME. The crosshair is the logo: the ring aims, the arc counts rounds, the thin arc holds Overclock (D-048).

#include "LogoCrosshairWidget.h"
#include "CombatFeelSubsystem.h"
#include "RailAimComponent.h"
#include "ThreatSubsystem.h"
#include "WeaponStatus.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformTime.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

DEFINE_LOG_CATEGORY_STATIC(LogLogoCrosshair, Log, All);

// the scalar parameters of M_UI_LogoCrosshair, a named namespace so unity builds keep the names apart
namespace LogoCrosshairLook
{
	struct FParams
	{
		FName Fill = TEXT("Fill");
		FName Segments = TEXT("Segments");
		FName Reloading = TEXT("Reloading");
		FName LowAmmo = TEXT("LowAmmo");
		FName Charge = TEXT("OC");
		FName ChargeActive = TEXT("OCActive");
		FName Telegraph = TEXT("Telegraph");
		FName Pulse = TEXT("Pulse");
		FName Flash = TEXT("Flash");
		FName Collapse = TEXT("Collapse");
		FName Opacity = TEXT("Opacity");
		FName Glitch = TEXT("Glitch");
	};

	const FParams& Params()
	{
		static const FParams Names;
		return Names;
	}
}

ULogoCrosshairWidget* ULogoCrosshairWidget::CreateFor(APlayerController* Player)
{
	if (!Player || !Player->IsLocalController())
	{
		return nullptr;
	}

	ULogoCrosshairWidget* Widget = CreateWidget<ULogoCrosshairWidget>(Player, ULogoCrosshairWidget::StaticClass());
	if (Widget)
	{
		Widget->AddToViewport(2);
		UE_LOG(LogLogoCrosshair, Log, TEXT("Logo crosshair created for %s"), *GetNameSafe(Player));
	}
	return Widget;
}

void ULogoCrosshairWidget::BindAim(URailAimComponent* InAim)
{
	Aim = InAim;
}

TSharedRef<SWidget> ULogoCrosshairWidget::RebuildWidget()
{
	// an empty full screen canvas with one image: the mark, placed at the aim point every frame
	if (WidgetTree && !WidgetTree->RootWidget && !HasAnyFlags(RF_ClassDefaultObject))
	{
		UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CrosshairCanvas"));
		WidgetTree->RootWidget = Canvas;

		Mark = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("LogoMark"));
		if (UMaterialInterface* Base = Material.LoadSynchronous())
		{
			MarkMaterial = UMaterialInstanceDynamic::Create(Base, this);
			Mark->SetBrushFromMaterial(MarkMaterial);
		}
		else
		{
			UE_LOG(LogLogoCrosshair, Warning, TEXT("Logo crosshair material %s is missing"), *Material.ToString());
		}

		if (UCanvasPanelSlot* MarkSlot = Canvas->AddChildToCanvas(Mark))
		{
			MarkSlot->SetAnchors(FAnchors(0.0f, 0.0f));
			MarkSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			MarkSlot->SetAutoSize(false);
			MarkSlot->SetSize(FVector2D(MarkSize, MarkSize));
		}
	}
	return Super::RebuildWidget();
}

void ULogoCrosshairWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::HitTestInvisible);
	BindSources();
}

void ULogoCrosshairWidget::NativeDestruct()
{
	if (UStyleScoringComponent* Style = BoundStyle.Get())
	{
		Style->OnStyleEvent.RemoveDynamic(this, &ULogoCrosshairWidget::HandleStyleEvent);
	}
	BoundStyle = nullptr;
	Super::NativeDestruct();
}

void ULogoCrosshairWidget::BindSources()
{
	// the pawn can change, a rider respawns or a level restarts: follow the aim of whoever is possessed now
	APawn* Pawn = GetOwningPlayerPawn();
	if (Pawn && (!Aim.IsValid() || Aim->GetOwner() != Pawn))
	{
		if (URailAimComponent* PawnAim = Pawn->FindComponentByClass<URailAimComponent>())
		{
			Aim = PawnAim;
		}
	}

	if (!BoundStyle.IsValid())
	{
		if (UStyleScoringComponent* Style = UStyleScoringComponent::Get(GetOwningPlayer()))
		{
			Style->OnStyleEvent.AddUniqueDynamic(this, &ULogoCrosshairWidget::HandleStyleEvent);
			BoundStyle = Style;
		}
	}
}

void ULogoCrosshairWidget::HandleStyleEvent(EStyleEvent Event, int32 Points, AActor* Target)
{
	const double Now = FPlatformTime::Seconds();
	switch (Event)
	{
	case EStyleEvent::Headshot:
		FlashAt = Now;
		PulseAt = Now;
		break;
	case EStyleEvent::Penalty:
		CollapseAt = Now;
		break;
	case EStyleEvent::Miss:
		break;
	default:
		// hits, kills, disarms, zone bonuses, controlled pairs and rescues all pulse the ring
		PulseAt = Now;
		break;
	}
}

void ULogoCrosshairWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	BindSources();
	UpdateMark(MyGeometry);
}

void ULogoCrosshairWidget::UpdateMark(const FGeometry& MyGeometry)
{
	const URailAimComponent* CurrentAim = Aim.Get();
	if (!Mark || !CurrentAim || !MarkMaterial)
	{
		if (Mark)
		{
			Mark->SetVisibility(ESlateVisibility::Collapsed);
		}
		return;
	}
	Mark->SetVisibility(ESlateVisibility::HitTestInvisible);

	// the normalised aim point in the widget's own space, so DPI never moves the mark off the trace
	const FVector2D Size = MyGeometry.GetLocalSize();
	const FVector2D Normalized = CurrentAim->GetCrosshairNormalized();
	if (UCanvasPanelSlot* MarkSlot = Cast<UCanvasPanelSlot>(Mark->Slot))
	{
		MarkSlot->SetPosition(FVector2D(Size.X * Normalized.X, Size.Y * Normalized.Y));
	}

	const double Now = FPlatformTime::Seconds();
	const float Delta = LastUpdateAt > 0.0 ? FMath::Min(static_cast<float>(Now - LastUpdateAt), 0.1f) : 0.0f;
	LastUpdateAt = Now;
	const LogoCrosshairLook::FParams& Param = LogoCrosshairLook::Params();

	// the gauge: one segment per round, or the reservoir's usable pressure as one arc, or the reload or refill closing it
	float Fill = 1.0f;
	float Segments = 0.0f;
	float Reloading = 0.0f;
	float LowAmmo = 0.0f;
	FWeaponStatus Status;
	const IWeaponStatusSource* Source = Cast<IWeaponStatusSource>(GetOwningPlayerPawn());
	if (Source && Source->GetWeaponStatus(Status) && (Status.MagazineSize > 0 || Status.bPressureFed))
	{
		if (Status.bReloading)
		{
			Fill = Status.ReloadProgress;
			Reloading = 1.0f;
		}
		else if (Status.bPressureFed)
		{
			// the arc spans the air a shot can still use, from the firing pressure up to a full reservoir (D-055)
			const float Usable = Status.FillPressureBar - Status.MinFirePressureBar;
			Fill = Usable > 0.0f ? FMath::Clamp((Status.PressureBar - Status.MinFirePressureBar) / Usable, 0.0f, 1.0f) : 0.0f;
			LowAmmo = Status.bEmpty || Fill <= LowAmmoShare ? 1.0f : 0.0f;
		}
		else
		{
			Fill = static_cast<float>(Status.Rounds) / static_cast<float>(Status.MagazineSize);
			Segments = Status.MagazineSize <= MaxSegments ? static_cast<float>(Status.MagazineSize) : 0.0f;
			LowAmmo = Status.Rounds <= FMath::Max(1, FMath::FloorToInt(Status.MagazineSize * LowAmmoShare)) ? 1.0f : 0.0f;
		}
	}

	// the thin arc: Overclock charge, empty where precision rules
	float Charge = 0.0f;
	float ChargeActive = 0.0f;
	if (const UCombatFeelSubsystem* Feel = UCombatFeelSubsystem::Get(this))
	{
		if (Feel->IsOverclockAllowed())
		{
			Charge = Feel->GetOverclockFraction();
			ChargeActive = Feel->IsOverclockActive() ? 1.0f : 0.0f;
		}
	}

	// the ring follows the strongest telegraph smoothly
	const UThreatSubsystem* Threats = UThreatSubsystem::Get(this);
	Telegraph = FMath::FInterpTo(Telegraph, Threats ? Threats->GetThreatLevel() : 0.0f, Delta, TelegraphFollowRate);

	const float Pulse = FMath::Clamp(1.0f - static_cast<float>(Now - PulseAt) / PulseSeconds, 0.0f, 1.0f);
	const float Flash = FMath::Clamp(1.0f - static_cast<float>(Now - FlashAt) / FlashSeconds, 0.0f, 1.0f);
	const float CollapseAge = static_cast<float>(Now - CollapseAt);
	const float Collapse = CollapseAge <= CollapseHoldSeconds ? 1.0f : FMath::Clamp(1.0f - (CollapseAge - CollapseHoldSeconds) / CollapseFadeSeconds, 0.0f, 1.0f);

	MarkMaterial->SetScalarParameterValue(Param.Fill, Fill);
	MarkMaterial->SetScalarParameterValue(Param.Segments, Segments);
	MarkMaterial->SetScalarParameterValue(Param.Reloading, Reloading);
	MarkMaterial->SetScalarParameterValue(Param.LowAmmo, LowAmmo);
	MarkMaterial->SetScalarParameterValue(Param.Charge, Charge);
	MarkMaterial->SetScalarParameterValue(Param.ChargeActive, ChargeActive);
	MarkMaterial->SetScalarParameterValue(Param.Telegraph, Telegraph);
	MarkMaterial->SetScalarParameterValue(Param.Pulse, Pulse);
	MarkMaterial->SetScalarParameterValue(Param.Flash, Flash);
	MarkMaterial->SetScalarParameterValue(Param.Collapse, Collapse);
	MarkMaterial->SetScalarParameterValue(Param.Opacity, CurrentAim->IsFireBlocked() ? BlockedOpacity : 1.0f);

	// the projection glitches for a moment when the player is hit (D-051)
	const UCombatFeelSubsystem* FeelNow = UCombatFeelSubsystem::Get(this);
	MarkMaterial->SetScalarParameterValue(Param.Glitch, FeelNow ? FeelNow->GetDamageGlitch() : 0.0f);
}
