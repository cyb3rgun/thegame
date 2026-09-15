// CYB3RGUN THEGAME. HUD for the flight range: countdown, score, hits and misses, the points of each hit and the summary.

#include "FlightRangeHUD.h"
#include "CyberMenuStyle.h"
#include "CyberText.h"
#include "HoloHUD.h"
#include "StyleHUDWidget.h"
#include "Blueprint/WidgetTree.h"
#include "BrandFrame.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
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

	auto Shadow = [](UTextBlock* Block)
	{
		Block->SetShadowOffset(FVector2D(2.0f, 2.0f));
		Block->SetShadowColorAndOpacity(FCyberMenuStyle::ShadowColor());
	};

	// the countdown in the middle at the top, the one number the whole mode is about: a small label over large numerals (D-082)
	UTextBlock* TimeValue = nullptr;
	UVerticalBox* Time = FCyberMenuStyle::MakeReadout(WidgetTree, TEXT("Time"), LOCTEXT("TimeLabel", "TIME"), 64, FCyberMenuStyle::TextColor(), TimeValue, HAlign_Center);
	TimeText = TimeValue;
	FlightRangeHudLook::Place(Canvas, Time, FAnchors(0.5f, 0.0f), FVector2D(0.5f, 0.0f), FVector2D(0.0f, 28.0f));

	UVerticalBox* TopLeft = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("TopLeft"));
	UTextBlock* ScoreValue = nullptr;
	TopLeft->AddChildToVerticalBox(FCyberMenuStyle::MakeReadout(WidgetTree, TEXT("Score"), LOCTEXT("ScoreLabel", "SCORE"), 48, FCyberMenuStyle::TextColor(), ScoreValue));
	ScoreText = ScoreValue;

	UHorizontalBox* Counts = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Counts"));
	UTextBlock* HitsValue = nullptr;
	UTextBlock* MissesValue = nullptr;
	if (UHorizontalBoxSlot* HitsSlot = Counts->AddChildToHorizontalBox(FCyberMenuStyle::MakeReadout(WidgetTree, TEXT("Hits"), LOCTEXT("HitsLabel", "HITS"), 28, FCyberMenuStyle::BrandColor(), HitsValue)))
	{
		HitsSlot->SetPadding(FMargin(0.0f, 0.0f, 28.0f, 0.0f));
	}
	Counts->AddChildToHorizontalBox(FCyberMenuStyle::MakeReadout(WidgetTree, TEXT("Misses"), LOCTEXT("MissesLabel", "MISSES"), 28, FCyberMenuStyle::DimTextColor(), MissesValue));
	HitsText = HitsValue;
	MissesText = MissesValue;
	if (UVerticalBoxSlot* CountsSlot = TopLeft->AddChildToVerticalBox(Counts))
	{
		CountsSlot->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 0.0f));
	}
	FlightRangeHudLook::Place(Canvas, TopLeft, FAnchors(0.0f, 0.0f), FVector2D(0.0f, 0.0f), FVector2D(40.0f, 100.0f));

	EventText = FCyberMenuStyle::MakeText(WidgetTree, TEXT("EventText"), FText::GetEmpty(), EBrandText::Value, 44, FCyberMenuStyle::BrandColor());
	EventText->SetJustification(ETextJustify::Center);
	Shadow(EventText);
	FlightRangeHudLook::Place(Canvas, EventText, FAnchors(0.5f, 0.36f), FVector2D(0.5f, 0.5f), FVector2D::ZeroVector);

	ReadyText = FCyberMenuStyle::MakeText(WidgetTree, TEXT("ReadyText"), FText::GetEmpty(), EBrandText::Title, 40, FCyberMenuStyle::TextColor());
	ReadyText->SetJustification(ETextJustify::Center);
	Shadow(ReadyText);
	FlightRangeHudLook::Place(Canvas, ReadyText, FAnchors(0.5f, 0.3f), FVector2D(0.5f, 0.5f), FVector2D::ZeroVector);

	// the summary is a plate with corner brackets: heading, the score as the big number, the counts as readout lines
	UBrandFrame* Plate = FCyberMenuStyle::MakeFrame(WidgetTree, TEXT("SummaryPanel"), FMargin(48.0f, 34.0f));
	UVerticalBox* SummaryColumn = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SummaryColumn"));
	Plate->SetContent(SummaryColumn);
	UTextBlock* Heading = FCyberMenuStyle::MakeText(WidgetTree, TEXT("SummaryHeading"), LOCTEXT("TimeUp", "TIME UP"), EBrandText::Heading, 26, FCyberMenuStyle::BrandColor());
	FCyberMenuStyle::AddToColumn(SummaryColumn, Heading, FMargin(0.0f, 0.0f, 0.0f, 6.0f), HAlign_Center);
	FCyberMenuStyle::AddToColumn(SummaryColumn, FCyberMenuStyle::MakeRule(WidgetTree, 320.0f, false), FMargin(0.0f, 0.0f, 0.0f, 14.0f), HAlign_Center);
	UTextBlock* FinalValue = nullptr;
	FCyberMenuStyle::AddToColumn(SummaryColumn, FCyberMenuStyle::MakeReadout(WidgetTree, TEXT("Final"), LOCTEXT("FinalLabel", "FINAL SCORE"), 72, FCyberMenuStyle::TextColor(), FinalValue, HAlign_Center),
		FMargin(0.0f, 0.0f, 0.0f, 16.0f), HAlign_Center);
	SummaryScoreText = FinalValue;
	SummaryText = FCyberMenuStyle::MakeText(WidgetTree, TEXT("SummaryText"), FText::GetEmpty(), EBrandText::Readout, 17, FCyberMenuStyle::TextColor());
	SummaryText->SetJustification(ETextJustify::Center);
	FCyberMenuStyle::AddToColumn(SummaryColumn, SummaryText, FMargin(0.0f), HAlign_Center);
	SummaryPanel = Plate;
	FlightRangeHudLook::Place(Canvas, Plate, FAnchors(0.5f, 0.5f), FVector2D(0.5f, 0.5f), FVector2D::ZeroVector);
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
	if (HitsText && MissesText && (Stats.TargetsHit != ShownHits || Stats.Misses != ShownMisses))
	{
		ShownHits = Stats.TargetsHit;
		ShownMisses = Stats.Misses;
		HitsText->SetText(FCyberText::Int(ShownHits));
		MissesText->SetText(FCyberText::Int(ShownMisses));
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
		TimeText->SetText(FCyberText::Int(SecondsLeft));
		const bool bLast = GameMode && GameMode->GetRoundState() == EFlightRoundState::Running && SecondsLeft <= 10;
		TimeText->SetColorAndOpacity(FSlateColor(bLast ? FCyberMenuStyle::DangerColor() : FCyberMenuStyle::TextColor()));
	}
}

void UFlightRangeHUD::HandleScoreChanged(int32 Score, int32 Delta)
{
	if (ScoreText)
	{
		ScoreText->SetText(FCyberText::Int(Score));
	}
}

void UFlightRangeHUD::HandleTargetScored(int32 Points, FVector Location)
{
	if (!EventText)
	{
		return;
	}
	// a penalty shows in the danger colour with its minus
	const bool bPenalty = Points < 0;
	EventText->SetText(bPenalty ? FCyberText::Int(Points) : FText::Format(LOCTEXT("Hit", "+{0}"), FCyberText::Int(Points)));
	EventText->SetColorAndOpacity(FSlateColor(bPenalty ? FCyberMenuStyle::DangerColor() : FCyberMenuStyle::BrandColor()));
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
	if (SummaryScoreText)
	{
		SummaryScoreText->SetText(FCyberText::Int(Stats.FinalScore));
	}
	const FText RoundSummary = FText::Format(
		LOCTEXT("SummaryFormat", "HITS {0} OF {1} LAUNCHED\nMISSES {2}   ACCURACY {3} %\nBEST HIT {4}   ESCAPED {5}\nSCENE BONUS {6}   SIGN {7}"),
		FCyberText::Int(Stats.TargetsHit), FCyberText::Int(Stats.TargetsLaunched),
		FCyberText::Int(Stats.Misses), FCyberText::Int(FMath::RoundToInt(Stats.Accuracy * 100.0f)), FCyberText::Int(Stats.BestHit), FCyberText::Int(Stats.TargetsEscaped),
		FCyberText::Int(Stats.BonusPoints), FCyberText::Int(-Stats.PenaltyPoints));

	const FText Summary = FText::Format(LOCTEXT("SummaryWithStyle", "{0}\n\n{1}"), RoundSummary, UStyleHUDWidget::FormatRunSummary(GetOwningPlayer()));
	UStyleHUDWidget::LogSummary(FText::Format(LOCTEXT("SummaryLog", "FINAL SCORE {0}\n{1}"), FCyberText::Int(Stats.FinalScore), Summary));
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
