// CYB3RGUN THEGAME. The style HUD: meter, rank, combo, ammunition and Overclock, the same in every scenario.

#include "StyleHUDWidget.h"
#include "CombatFeelSubsystem.h"
#include "CyberMenuStyle.h"
#include "CyberText.h"
#include "StyleSettings.h"
#include "WeaponStatus.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformTime.h"

#define LOCTEXT_NAMESPACE "StyleHUD"

DEFINE_LOG_CATEGORY_STATIC(LogStyleHUD, Log, All);

// a named namespace with unique names, unity builds merge this file with other HUDs that have their own helpers
namespace StyleHudLook
{
	UTextBlock* MakeLine(UWidgetTree* Tree, const FName& Name, int32 Size, const FLinearColor& Color, ETextJustify::Type Justify)
	{
		UTextBlock* Block = FCyberMenuStyle::MakeText(Tree, Name, FText::GetEmpty(), Size, Color);
		Block->SetJustification(Justify);
		Block->SetShadowOffset(FVector2D(2.0f, 2.0f));
		Block->SetShadowColorAndOpacity(FCyberMenuStyle::ShadowColor());
		return Block;
	}

	UProgressBar* MakeBar(UWidgetTree* Tree, const FName& Name, const FLinearColor& Fill)
	{
		UProgressBar* Bar = Tree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), Name);
		FProgressBarStyle Style = Bar->GetWidgetStyle();
		Style.BackgroundImage.TintColor = FSlateColor(FCyberMenuStyle::TrackColor());
		Bar->SetWidgetStyle(Style);
		Bar->SetFillColorAndOpacity(Fill);
		Bar->SetPercent(0.0f);
		return Bar;
	}

	void AddLine(UVerticalBox* Column, UWidget* Widget, EHorizontalAlignment Align, float Bottom)
	{
		if (UVerticalBoxSlot* LineSlot = Column->AddChildToVerticalBox(Widget))
		{
			LineSlot->SetHorizontalAlignment(Align);
			LineSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, Bottom));
		}
	}

	void AddBar(UWidgetTree* Tree, UVerticalBox* Column, UProgressBar* Bar, float Width, float Height, EHorizontalAlignment Align, float Bottom)
	{
		USizeBox* Box = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		Box->SetWidthOverride(Width);
		Box->SetHeightOverride(Height);
		Box->SetContent(Bar);
		AddLine(Column, Box, Align, Bottom);
	}
}

UStyleHUDWidget* UStyleHUDWidget::CreateFor(APlayerController* Player)
{
	if (!Player || !Player->IsLocalController())
	{
		return nullptr;
	}

	UStyleHUDWidget* Widget = CreateWidget<UStyleHUDWidget>(Player, UStyleHUDWidget::StaticClass());
	if (Widget)
	{
		Widget->AddToViewport(3);
		UE_LOG(LogStyleHUD, Log, TEXT("Style HUD created for %s"), *GetNameSafe(Player));
	}
	return Widget;
}

FText UStyleHUDWidget::FormatRunSummary(APlayerController* Player)
{
	const UStyleScoringComponent* Style = UStyleScoringComponent::Get(Player);
	if (!Style)
	{
		return FText::GetEmpty();
	}

	const FStyleRunStats& Stats = Style->GetStats();
	return FText::Format(LOCTEXT("RunSummary", "STYLE {0}   BEST RANK {1}\nAccuracy {2}%   Best combo {3}\nControlled pairs {4}   Headshots {5}\nRescues {6}   Penalties {7}"),
		FCyberText::Int(Stats.StylePoints), UStyleSettings::Get(Player)->GetRankLabel(Stats.PeakMeter), FCyberText::Int(FMath::RoundToInt(Stats.GetAccuracy() * 100.0f)),
		FCyberText::Int(Stats.BestCombo), FCyberText::Int(Stats.ControlledPairs), FCyberText::Int(Stats.Headshots), FCyberText::Int(Stats.Rescues), FCyberText::Int(Stats.Penalties));
}

void UStyleHUDWidget::LogSummary(const FText& Summary)
{
	FString Line = Summary.ToString();
	Line.ReplaceInline(TEXT("\n\n"), TEXT(" | "));
	Line.ReplaceInline(TEXT("\n"), TEXT(" | "));
	UE_LOG(LogStyleHUD, Log, TEXT("Run summary: %s"), *Line);
}

TSharedRef<SWidget> UStyleHUDWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget && !HasAnyFlags(RF_ClassDefaultObject))
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

void UStyleHUDWidget::BuildLayout()
{
	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("StyleCanvas"));
	WidgetTree->RootWidget = Canvas;

	// top right, over the sky: rank, meter, style points, combo, the last style event, Overclock
	UVerticalBox* StyleColumn = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("StyleColumn"));
	RankText = StyleHudLook::MakeLine(WidgetTree, TEXT("RankText"), 34, FCyberMenuStyle::TextColor(), ETextJustify::Right);
	MeterBar = StyleHudLook::MakeBar(WidgetTree, TEXT("MeterBar"), FCyberMenuStyle::BrandColor());
	StyleText = StyleHudLook::MakeLine(WidgetTree, TEXT("StyleText"), 20, FCyberMenuStyle::TextColor(), ETextJustify::Right);
	ComboText = StyleHudLook::MakeLine(WidgetTree, TEXT("ComboText"), 26, FCyberMenuStyle::BrandColor(), ETextJustify::Right);
	EventText = StyleHudLook::MakeLine(WidgetTree, TEXT("EventText"), 18, FCyberMenuStyle::TextColor(), ETextJustify::Right);
	OverclockText = StyleHudLook::MakeLine(WidgetTree, TEXT("OverclockText"), 18, FCyberMenuStyle::CounterColor(), ETextJustify::Right);
	OverclockBar = StyleHudLook::MakeBar(WidgetTree, TEXT("OverclockBar"), FCyberMenuStyle::CounterColor());
	StyleHudLook::AddLine(StyleColumn, RankText, HAlign_Right, 2.0f);
	StyleHudLook::AddBar(WidgetTree, StyleColumn, MeterBar, 300.0f, 10.0f, HAlign_Right, 6.0f);
	StyleHudLook::AddLine(StyleColumn, StyleText, HAlign_Right, 2.0f);
	StyleHudLook::AddLine(StyleColumn, ComboText, HAlign_Right, 2.0f);
	StyleHudLook::AddLine(StyleColumn, EventText, HAlign_Right, 18.0f);
	StyleHudLook::AddLine(StyleColumn, OverclockText, HAlign_Right, 2.0f);
	StyleHudLook::AddBar(WidgetTree, StyleColumn, OverclockBar, 300.0f, 8.0f, HAlign_Right, 0.0f);
	if (UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(StyleColumn))
	{
		PanelSlot->SetAnchors(FAnchors(1.0f, 0.0f));
		PanelSlot->SetAlignment(FVector2D(1.0f, 0.0f));
		PanelSlot->SetPosition(FVector2D(-40.0f, 40.0f));
		PanelSlot->SetAutoSize(true);
	}

	// bottom left, above the round counter of the shooter HUD: the weapon, its reload and the empty cue
	UVerticalBox* WeaponColumn = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("WeaponColumn"));
	WeaponText = StyleHudLook::MakeLine(WidgetTree, TEXT("WeaponText"), 28, FCyberMenuStyle::TextColor(), ETextJustify::Left);
	ReloadBar = StyleHudLook::MakeBar(WidgetTree, TEXT("ReloadBar"), FCyberMenuStyle::BrandColor());
	CueText = StyleHudLook::MakeLine(WidgetTree, TEXT("CueText"), 22, FCyberMenuStyle::DangerColor(), ETextJustify::Left);
	StyleHudLook::AddLine(WeaponColumn, WeaponText, HAlign_Left, 4.0f);
	StyleHudLook::AddBar(WidgetTree, WeaponColumn, ReloadBar, 260.0f, 6.0f, HAlign_Left, 4.0f);
	StyleHudLook::AddLine(WeaponColumn, CueText, HAlign_Left, 0.0f);
	if (UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(WeaponColumn))
	{
		PanelSlot->SetAnchors(FAnchors(0.0f, 1.0f));
		PanelSlot->SetAlignment(FVector2D(0.0f, 1.0f));
		PanelSlot->SetPosition(FVector2D(50.0f, -110.0f));
		PanelSlot->SetAutoSize(true);
	}
}

void UStyleHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetVisibility(ESlateVisibility::HitTestInvisible);
	if (ReloadBar)
	{
		ReloadBar->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (CueText)
	{
		CueText->SetVisibility(ESlateVisibility::Collapsed);
	}
	BindStyle();
}

void UStyleHUDWidget::NativeDestruct()
{
	if (UStyleScoringComponent* Style = BoundStyle.Get())
	{
		Style->OnStyleEvent.RemoveDynamic(this, &UStyleHUDWidget::HandleStyleEvent);
	}
	BoundStyle = nullptr;
	Super::NativeDestruct();
}

void UStyleHUDWidget::BindStyle()
{
	if (BoundStyle.IsValid())
	{
		return;
	}
	if (UStyleScoringComponent* Style = UStyleScoringComponent::Get(GetOwningPlayer()))
	{
		Style->OnStyleEvent.AddUniqueDynamic(this, &UStyleHUDWidget::HandleStyleEvent);
		BoundStyle = Style;
	}
}

void UStyleHUDWidget::HandleStyleEvent(EStyleEvent Event, int32 Points, AActor* Target)
{
	FText Label;
	FLinearColor Color = FCyberMenuStyle::TextColor();
	switch (Event)
	{
	case EStyleEvent::Hit:
		Label = LOCTEXT("EventHit", "HIT");
		break;
	case EStyleEvent::Kill:
		Label = LOCTEXT("EventKill", "KILL");
		break;
	case EStyleEvent::Headshot:
		Label = LOCTEXT("EventHeadshot", "HEADSHOT");
		break;
	case EStyleEvent::ControlledPair:
		Label = LOCTEXT("EventPair", "CONTROLLED PAIR");
		break;
	case EStyleEvent::Rescue:
		Label = LOCTEXT("EventRescue", "RESCUE");
		break;
	case EStyleEvent::Miss:
		Label = LOCTEXT("EventMiss", "MISS");
		Color = FCyberMenuStyle::DimTextColor();
		break;
	case EStyleEvent::Penalty:
		Label = LOCTEXT("EventPenalty", "PENALTY");
		Color = FCyberMenuStyle::DangerColor();
		break;
	default:
		return;
	}

	const FText Part = Points != 0 ? FText::Format(LOCTEXT("EventPoints", "{0} {1}"), Label, FCyberText::Signed(Points)) : Label;

	// the events of one shot arrive in the same frame and share the line
	const double Now = FPlatformTime::Seconds();
	EventLine = (Now - EventShownAt < 0.05) ? FText::Format(LOCTEXT("EventJoin", "{0}   {1}"), EventLine, Part) : Part;
	EventShownAt = Now;
	if (EventText)
	{
		EventText->SetText(EventLine);
		EventText->SetColorAndOpacity(FSlateColor(Color));
	}
	if (Event != EStyleEvent::Miss && Event != EStyleEvent::Penalty)
	{
		RankPopAt = Now;
	}
}

void UStyleHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	BindStyle();
	const double WallNow = FPlatformTime::Seconds();
	UpdateStyle(WallNow);
	UpdateWeapon(GetWorld() ? GetWorld()->GetRealTimeSeconds() : 0.0);
	UpdateOverclock(WallNow);
}

void UStyleHUDWidget::UpdateStyle(double WallNow)
{
	const UStyleScoringComponent* Style = BoundStyle.Get();
	if (!Style || !RankText)
	{
		return;
	}

	// the rank brightens from dim to the accent as the meter fills, and pops for a moment on every clean action
	const float Fraction = Style->GetMeterFraction();
	MeterBar->SetPercent(Fraction);
	RankText->SetText(Style->GetRankLabel());
	RankText->SetColorAndOpacity(FSlateColor(FMath::Lerp(FCyberMenuStyle::DimTextColor(), FCyberMenuStyle::BrandColor(), Fraction)));
	const float Pop = FMath::Clamp(1.0f - static_cast<float>(WallNow - RankPopAt) / 0.25f, 0.0f, 1.0f);
	RankText->SetRenderScale(FVector2D(1.0f + 0.15f * Pop));

	StyleText->SetText(FText::Format(LOCTEXT("StylePoints", "STYLE {0}"), FCyberText::Int(Style->GetStats().StylePoints)));
	ComboText->SetText(FText::Format(LOCTEXT("Combo", "x{0}  COMBO {1}"), FCyberText::Fixed(Style->GetMultiplier(), 1), FCyberText::Int(Style->GetCombo())));

	const float EventAge = static_cast<float>(WallNow - EventShownAt);
	EventText->SetRenderOpacity(FMath::Clamp(1.0f - (EventAge - EventHoldSeconds) / 0.5f, 0.0f, 1.0f));
}

void UStyleHUDWidget::UpdateWeapon(double RealTime)
{
	if (!WeaponText)
	{
		return;
	}

	FWeaponStatus Status;
	const IWeaponStatusSource* Source = Cast<IWeaponStatusSource>(GetOwningPlayerPawn());
	if (!Source || !Source->GetWeaponStatus(Status))
	{
		WeaponText->SetVisibility(ESlateVisibility::Collapsed);
		ReloadBar->SetVisibility(ESlateVisibility::Collapsed);
		CueText->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	WeaponText->SetVisibility(ESlateVisibility::HitTestInvisible);
	WeaponText->SetText(FText::Format(LOCTEXT("Weapon", "{0}  {1} / {2}"), Status.WeaponName, FCyberText::Int(Status.Rounds), FCyberText::Int(Status.MagazineSize)));
	const FLinearColor WeaponColor = Status.Rounds == 0 ? FCyberMenuStyle::DangerColor() : (Status.bSwitching ? FCyberMenuStyle::DimTextColor() : FCyberMenuStyle::TextColor());
	WeaponText->SetColorAndOpacity(FSlateColor(WeaponColor));

	ReloadBar->SetVisibility(Status.bReloading ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	ReloadBar->SetPercent(Status.ReloadProgress);

	// the empty cue: while the magazine is empty and no reload runs, and for a moment after a pull on an empty magazine
	const bool bDryPull = RealTime - Status.LastDryFireTime < EmptyCueSeconds;
	const bool bCue = !Status.bReloading && (Status.Rounds == 0 || bDryPull);
	CueText->SetVisibility(bCue ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (bCue)
	{
		CueText->SetText(FText::Format(LOCTEXT("EmptyCue", "EMPTY  {0}"), Status.ReloadHint));
		CueText->SetRenderOpacity(0.55f + 0.45f * FMath::Abs(FMath::Sin(static_cast<float>(RealTime) * 6.0f)));
	}
}

void UStyleHUDWidget::UpdateOverclock(double WallNow)
{
	if (!OverclockText)
	{
		return;
	}

	const UCombatFeelSubsystem* Feel = UCombatFeelSubsystem::Get(this);
	if (!Feel || !Feel->IsOverclockAllowed())
	{
		OverclockText->SetText(LOCTEXT("OverclockLocked", "OVERCLOCK LOCKED"));
		OverclockText->SetColorAndOpacity(FSlateColor(FCyberMenuStyle::DimTextColor()));
		OverclockText->SetRenderOpacity(1.0f);
		OverclockBar->SetPercent(0.0f);
		return;
	}

	OverclockBar->SetPercent(Feel->GetOverclockFraction());
	if (Feel->IsOverclockActive())
	{
		OverclockText->SetText(LOCTEXT("OverclockOn", "OVERCLOCK ON"));
		OverclockText->SetColorAndOpacity(FSlateColor(FCyberMenuStyle::CounterColor()));
		OverclockText->SetRenderOpacity(0.6f + 0.4f * FMath::Abs(FMath::Sin(static_cast<float>(WallNow) * 8.0f)));
	}
	else if (Feel->IsOverclockReady())
	{
		OverclockText->SetText(LOCTEXT("OverclockReady", "OVERCLOCK READY  E OR LB"));
		OverclockText->SetColorAndOpacity(FSlateColor(FCyberMenuStyle::CounterColor()));
		OverclockText->SetRenderOpacity(1.0f);
	}
	else
	{
		OverclockText->SetText(FText::Format(LOCTEXT("OverclockCharge", "OVERCLOCK {0}%"), FCyberText::Int(FMath::RoundToInt(Feel->GetOverclockFraction() * 100.0f))));
		OverclockText->SetColorAndOpacity(FSlateColor(FCyberMenuStyle::DimTextColor()));
		OverclockText->SetRenderOpacity(1.0f);
	}
}

#undef LOCTEXT_NAMESPACE
