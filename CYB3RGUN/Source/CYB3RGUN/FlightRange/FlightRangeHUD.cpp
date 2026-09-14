// CYB3RGUN THEGAME. HUD for the flight range: countdown, score, hits and misses, the points of each hit and the summary.

#include "FlightRangeHUD.h"
#include "CyberMenuStyle.h"
#include "CyberText.h"
#include "HoloHUD.h"
#include "StyleHUDWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Engine/World.h"

#define LOCTEXT_NAMESPACE "FlightRangeHUD"

// a named namespace with unique names, unity builds merge this file with the other HUDs
namespace FlightRangeHudLook
{
	void Place(UCanvasPanel* Canvas, UWidget* Widget, const FAnchors& Anchors, const FVector2D& Alignment, const FVector2D& Position)
	{
		if (UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(Widget))
		{
			PanelSlot->SetAnchors(Anchors);
			PanelSlot->SetAlignment(Alignment);
			PanelSlot->SetPosition(Position);
			PanelSlot->SetAutoSize(true);
		}
	}
}

TSharedRef<SWidget> UFlightRangeHUD::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget && !HasAnyFlags(RF_ClassDefaultObject))
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

void UFlightRangeHUD::BuildLayout()
{
	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Canvas"));
	WidgetTree->RootWidget = Canvas;

	auto MakeText = [this](const FName& Name, int32 Size, const FLinearColor& Color)
	{
		UTextBlock* Block = FCyberMenuStyle::MakeText(WidgetTree, Name, FText::GetEmpty(), Size, Color);
		Block->SetShadowOffset(FVector2D(2.0f, 2.0f));
		Block->SetShadowColorAndOpacity(FCyberMenuStyle::ShadowColor());
		return Block;
	};

	// the countdown in the middle at the top, the one number the whole mode is about
	TimeText = MakeText(TEXT("TimeText"), 52, FCyberMenuStyle::TextColor());
	TimeText->SetJustification(ETextJustify::Center);
	FlightRangeHudLook::Place(Canvas, TimeText, FAnchors(0.5f, 0.0f), FVector2D(0.5f, 0.0f), FVector2D(0.0f, 36.0f));

	UVerticalBox* TopLeft = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("TopLeft"));
	ScoreText = MakeText(TEXT("ScoreText"), 36, FCyberMenuStyle::TextColor());
	HitsText = MakeText(TEXT("HitsText"), 24, FCyberMenuStyle::TextColor());
	TopLeft->AddChildToVerticalBox(ScoreText);
	TopLeft->AddChildToVerticalBox(HitsText);
	FlightRangeHudLook::Place(Canvas, TopLeft, FAnchors(0.0f, 0.0f), FVector2D(0.0f, 0.0f), FVector2D(40.0f, 120.0f));

	EventText = MakeText(TEXT("EventText"), 40, FCyberMenuStyle::BrandColor());
	EventText->SetJustification(ETextJustify::Center);
	FlightRangeHudLook::Place(Canvas, EventText, FAnchors(0.5f, 0.36f), FVector2D(0.5f, 0.5f), FVector2D::ZeroVector);

	ReadyText = MakeText(TEXT("ReadyText"), 44, FCyberMenuStyle::CounterColor());
	ReadyText->SetJustification(ETextJustify::Center);
	FlightRangeHudLook::Place(Canvas, ReadyText, FAnchors(0.5f, 0.3f), FVector2D(0.5f, 0.5f), FVector2D::ZeroVector);

	UBorder* Border = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SummaryPanel"));
	Border->SetBrushColor(FCyberMenuStyle::PanelColor());
	Border->SetPadding(FMargin(32.0f, 24.0f));
	SummaryText = MakeText(TEXT("SummaryText"), 28, FCyberMenuStyle::TextColor());
	SummaryText->SetJustification(ETextJustify::Center);
	Border->SetContent(SummaryText);
	SummaryPanel = Border;
	FlightRangeHudLook::Place(Canvas, Border, FAnchors(0.5f, 0.5f), FVector2D(0.5f, 0.5f), FVector2D::ZeroVector);
}

void UFlightRangeHUD::NativeConstruct()
{
	Super::NativeConstruct();

	HoloText = FHoloHUD::CreateTextMaterial(this);
	FHoloHUD::ApplyToText(WidgetTree, HoloText);
	if (EventText)
	{
		EventText->SetRenderOpacity(0.0f);
	}
	if (SummaryPanel)
	{
		SummaryPanel->SetVisibility(ESlateVisibility::Collapsed);
	}

	BindGameMode();
	if (GameMode)
	{
		HandleTimeChanged(GameMode->GetSecondsLeft());
		HandleScoreChanged(GameMode->GetScore(), 0);
		HandleStateChanged(GameMode->GetRoundState());
	}
}

void UFlightRangeHUD::NativeDestruct()
{
	UnbindGameMode();
	Super::NativeDestruct();
}

void UFlightRangeHUD::BindGameMode()
{
	AFlightRangeGameMode* Found = GetWorld() ? Cast<AFlightRangeGameMode>(GetWorld()->GetAuthGameMode()) : nullptr;
	if (Found == GameMode)
	{
		return;
	}
	UnbindGameMode();
	GameMode = Found;
	if (GameMode)
	{
		GameMode->OnTimeChanged.AddUniqueDynamic(this, &UFlightRangeHUD::HandleTimeChanged);
		GameMode->OnScoreChanged.AddUniqueDynamic(this, &UFlightRangeHUD::HandleScoreChanged);
		GameMode->OnTargetScored.AddUniqueDynamic(this, &UFlightRangeHUD::HandleTargetScored);
		GameMode->OnRoundStateChanged.AddUniqueDynamic(this, &UFlightRangeHUD::HandleStateChanged);
		GameMode->OnRoundFinished.AddUniqueDynamic(this, &UFlightRangeHUD::HandleRoundFinished);
	}
}

void UFlightRangeHUD::UnbindGameMode()
{
	if (GameMode)
	{
		GameMode->OnTimeChanged.RemoveDynamic(this, &UFlightRangeHUD::HandleTimeChanged);
		GameMode->OnScoreChanged.RemoveDynamic(this, &UFlightRangeHUD::HandleScoreChanged);
		GameMode->OnTargetScored.RemoveDynamic(this, &UFlightRangeHUD::HandleTargetScored);
		GameMode->OnRoundStateChanged.RemoveDynamic(this, &UFlightRangeHUD::HandleStateChanged);
		GameMode->OnRoundFinished.RemoveDynamic(this, &UFlightRangeHUD::HandleRoundFinished);
		GameMode = nullptr;
	}
}

void UFlightRangeHUD::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	FHoloHUD::Update(this, HoloText);

	if (!GameMode)
	{
		BindGameMode();
		return;
	}

	// hits and misses change with every shot, the style record resolves misses on its own time
	const FFlightRangeStats Stats = GameMode->GetStats();
	if (HitsText && (Stats.TargetsHit != ShownHits || Stats.Misses != ShownMisses))
	{
		ShownHits = Stats.TargetsHit;
		ShownMisses = Stats.Misses;
		HitsText->SetText(FText::Format(LOCTEXT("HitsFormat", "HITS {0}   MISSES {1}"), FCyberText::Int(ShownHits), FCyberText::Int(ShownMisses)));
	}

	if (ReadyText && GameMode->GetRoundState() == EFlightRoundState::GetReady)
	{
		ReadyText->SetText(FText::Format(LOCTEXT("ReadyFormat", "GET READY  {0}"), FCyberText::Int(FMath::CeilToInt(GameMode->GetReadySecondsLeft()))));
	}

	if (bEventVisible && EventText)
	{
		EventAge += InDeltaTime;
		const float Fade = EventAge <= EventHoldSeconds ? 1.0f : 1.0f - FMath::Clamp((EventAge - EventHoldSeconds) / EventFadeSeconds, 0.0f, 1.0f);
		EventText->SetRenderOpacity(Fade);
		bEventVisible = Fade > 0.0f;
	}
}

void UFlightRangeHUD::HandleTimeChanged(int32 SecondsLeft)
{
	if (TimeText)
	{
		TimeText->SetText(FText::Format(LOCTEXT("TimeFormat", "TIME {0}"), FCyberText::Int(SecondsLeft)));
		const bool bLast = GameMode && GameMode->GetRoundState() == EFlightRoundState::Running && SecondsLeft <= 10;
		TimeText->SetColorAndOpacity(FSlateColor(bLast ? FCyberMenuStyle::DangerColor() : FCyberMenuStyle::TextColor()));
	}
}

void UFlightRangeHUD::HandleScoreChanged(int32 Score, int32 Delta)
{
	if (ScoreText)
	{
		ScoreText->SetText(FText::Format(LOCTEXT("ScoreFormat", "SCORE {0}"), FCyberText::Int(Score)));
	}
}

void UFlightRangeHUD::HandleTargetScored(int32 Points, FVector Location)
{
	if (!EventText)
	{
		return;
	}
	EventText->SetText(FText::Format(LOCTEXT("Hit", "+{0}"), FCyberText::Int(Points)));
	EventText->SetRenderOpacity(1.0f);
	EventAge = 0.0f;
	bEventVisible = true;
}

void UFlightRangeHUD::HandleStateChanged(EFlightRoundState State)
{
	if (ReadyText)
	{
		ReadyText->SetVisibility(State == EFlightRoundState::GetReady ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (SummaryPanel && State != EFlightRoundState::Finished)
	{
		SummaryPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UFlightRangeHUD::HandleRoundFinished(const FFlightRangeStats& Stats)
{
	const FText RoundSummary = FText::Format(
		LOCTEXT("SummaryFormat", "TIME UP\n\nFINAL SCORE {0}\n\nHits {1} of {2} launched\nMisses {3}, accuracy {4} %\nBest hit {5}\nEscaped {6}"),
		FCyberText::Int(Stats.FinalScore), FCyberText::Int(Stats.TargetsHit), FCyberText::Int(Stats.TargetsLaunched),
		FCyberText::Int(Stats.Misses), FCyberText::Int(FMath::RoundToInt(Stats.Accuracy * 100.0f)), FCyberText::Int(Stats.BestHit), FCyberText::Int(Stats.TargetsEscaped));

	const FText Summary = FText::Format(LOCTEXT("SummaryWithStyle", "{0}\n\n{1}"), RoundSummary, UStyleHUDWidget::FormatRunSummary(GetOwningPlayer()));
	UStyleHUDWidget::LogSummary(Summary);
	if (SummaryText)
	{
		SummaryText->SetText(Summary);
	}
	if (SummaryPanel)
	{
		SummaryPanel->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	if (EventText)
	{
		EventText->SetRenderOpacity(0.0f);
		bEventVisible = false;
	}
}

#undef LOCTEXT_NAMESPACE
