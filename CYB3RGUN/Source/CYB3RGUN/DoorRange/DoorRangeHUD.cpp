// CYB3RGUN THEGAME. HUD for the door range.

#include "DoorRangeHUD.h"
#include "DoorRangeGameMode.h"
#include "CyberText.h"
#include "CyberMenuStyle.h"
#include "HoloHUD.h"
#include "StyleHUDWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/World.h"
#include "Styling/CoreStyle.h"

#define LOCTEXT_NAMESPACE "DoorRangeHUD"

// a named namespace with unique names, unity builds merge this file with other HUDs that have their own helpers
namespace DoorRangeHudLook
{
	FLinearColor Neutral() { return FCyberMenuStyle::TextColor(); }
	FLinearColor Good() { return FCyberMenuStyle::BrandColor(); }
	FLinearColor Bad() { return FCyberMenuStyle::DangerColor(); }
	FLinearColor Warn() { return FCyberMenuStyle::CounterColor(); }

	void Restyle(UTextBlock* Line, const FLinearColor& Color)
	{
		if (Line)
		{
			Line->SetColorAndOpacity(FSlateColor(Color));
		}
	}
}

TSharedRef<SWidget> UDoorRangeHUD::RebuildWidget()
{
	// A Blueprint child normally supplies the tree. Without one, build a plain layout so the HUD still shows.
	if (WidgetTree && !WidgetTree->RootWidget && !HasAnyFlags(RF_ClassDefaultObject))
	{
		BuildFallbackLayout();
	}

	return Super::RebuildWidget();
}

void UDoorRangeHUD::BuildFallbackLayout()
{
	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("FallbackCanvas"));
	WidgetTree->RootWidget = Canvas;

	auto MakeText = [this](const FName& Name, int32 Size, const FLinearColor& Color) -> UTextBlock*
	{
		UTextBlock* Block = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Bold", Size);
		Block->SetFont(Font);
		Block->SetColorAndOpacity(FSlateColor(Color));
		Block->SetShadowOffset(FVector2D(2.0f, 2.0f));
		Block->SetShadowColorAndOpacity(FCyberMenuStyle::ShadowColor());
		return Block;
	};

	UVerticalBox* TopLeft = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("FallbackTopLeft"));
	if (!ScoreText)
	{
		ScoreText = MakeText(TEXT("ScoreText"), 36, DoorRangeHudLook::Neutral());
		TopLeft->AddChildToVerticalBox(ScoreText);
	}
	if (!WaveText)
	{
		WaveText = MakeText(TEXT("WaveText"), 24, DoorRangeHudLook::Neutral());
		TopLeft->AddChildToVerticalBox(WaveText);
	}
	if (!HostilesText)
	{
		HostilesText = MakeText(TEXT("HostilesText"), 24, DoorRangeHudLook::Warn());
		TopLeft->AddChildToVerticalBox(HostilesText);
	}
	if (UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(TopLeft))
	{
		PanelSlot->SetAnchors(FAnchors(0.0f, 0.0f));
		PanelSlot->SetPosition(FVector2D(40.0f, 120.0f));
		PanelSlot->SetAutoSize(true);
	}

	if (!EventText)
	{
		EventText = MakeText(TEXT("EventText"), 40, DoorRangeHudLook::Neutral());
		EventText->SetJustification(ETextJustify::Center);
		if (UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(EventText))
		{
			PanelSlot->SetAnchors(FAnchors(0.5f, 0.3f));
			PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			PanelSlot->SetAutoSize(true);
		}
	}

	if (!SummaryPanel)
	{
		UBorder* Border = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SummaryPanel"));
		Border->SetBrushColor(FCyberMenuStyle::PanelColor());
		Border->SetPadding(FMargin(32.0f, 24.0f));
		SummaryText = MakeText(TEXT("SummaryText"), 28, DoorRangeHudLook::Neutral());
		SummaryText->SetJustification(ETextJustify::Center);
		Border->SetContent(SummaryText);
		SummaryPanel = Border;
		if (UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(Border))
		{
			PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f));
			PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			PanelSlot->SetAutoSize(true);
		}
	}
}

void UDoorRangeHUD::NativeConstruct()
{
	Super::NativeConstruct();

	// the Blueprint layout bakes its own colours, the brand style decides them
	DoorRangeHudLook::Restyle(ScoreText, DoorRangeHudLook::Neutral());
	DoorRangeHudLook::Restyle(WaveText, DoorRangeHudLook::Neutral());
	DoorRangeHudLook::Restyle(HostilesText, DoorRangeHudLook::Warn());
	DoorRangeHudLook::Restyle(SummaryText, DoorRangeHudLook::Neutral());
	if (UBorder* Panel = Cast<UBorder>(SummaryPanel))
	{
		Panel->SetBrushColor(FCyberMenuStyle::PanelColor());
	}
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

	RefreshAll();
}

void UDoorRangeHUD::NativeDestruct()
{
	UnbindGameMode();
	Super::NativeDestruct();
}

void UDoorRangeHUD::RefreshAll()
{
	BindGameMode();

	if (!GameMode)
	{
		SetTextSafe(ScoreText, LOCTEXT("NoRange", "No door range"));
		return;
	}

	HandleScoreChanged(GameMode->GetScore(), 0);
	HandleWaveChanged(GameMode->GetCurrentWave(), GameMode->GetWaveCount());
	HandleHostilesChanged(GameMode->GetHostilesRemaining(), GameMode->GetHostilesTotalThisWave());

	if (GameMode->IsRangeComplete())
	{
		HandleRangeFinished(GameMode->GetStats());
	}
}

void UDoorRangeHUD::BindGameMode()
{
	ADoorRangeGameMode* Found = GetWorld() ? Cast<ADoorRangeGameMode>(GetWorld()->GetAuthGameMode()) : nullptr;
	if (Found == GameMode)
	{
		return;
	}

	UnbindGameMode();
	GameMode = Found;

	if (GameMode)
	{
		GameMode->OnScoreChanged.AddUniqueDynamic(this, &UDoorRangeHUD::HandleScoreChanged);
		GameMode->OnWaveChanged.AddUniqueDynamic(this, &UDoorRangeHUD::HandleWaveChanged);
		GameMode->OnHostilesRemainingChanged.AddUniqueDynamic(this, &UDoorRangeHUD::HandleHostilesChanged);
		GameMode->OnRangeEvent.AddUniqueDynamic(this, &UDoorRangeHUD::HandleRangeEvent);
		GameMode->OnRangeFinished.AddUniqueDynamic(this, &UDoorRangeHUD::HandleRangeFinished);
	}
}

void UDoorRangeHUD::UnbindGameMode()
{
	if (GameMode)
	{
		GameMode->OnScoreChanged.RemoveDynamic(this, &UDoorRangeHUD::HandleScoreChanged);
		GameMode->OnWaveChanged.RemoveDynamic(this, &UDoorRangeHUD::HandleWaveChanged);
		GameMode->OnHostilesRemainingChanged.RemoveDynamic(this, &UDoorRangeHUD::HandleHostilesChanged);
		GameMode->OnRangeEvent.RemoveDynamic(this, &UDoorRangeHUD::HandleRangeEvent);
		GameMode->OnRangeFinished.RemoveDynamic(this, &UDoorRangeHUD::HandleRangeFinished);
		GameMode = nullptr;
	}
}

void UDoorRangeHUD::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	FHoloHUD::Update(this, HoloText);

	if (!bEventVisible || !EventText)
	{
		return;
	}

	EventAge += InDeltaTime;
	if (EventAge <= EventHoldSeconds)
	{
		EventText->SetRenderOpacity(1.0f);
	}
	else
	{
		const float Fade = 1.0f - FMath::Clamp((EventAge - EventHoldSeconds) / EventFadeSeconds, 0.0f, 1.0f);
		EventText->SetRenderOpacity(Fade);
		if (Fade <= 0.0f)
		{
			bEventVisible = false;
		}
	}
}

void UDoorRangeHUD::HandleScoreChanged(int32 Score, int32 Delta)
{
	SetTextSafe(ScoreText, FText::Format(LOCTEXT("ScoreFormat", "SCORE {0}"), FCyberText::Int(Score)));
}

void UDoorRangeHUD::HandleWaveChanged(int32 Wave, int32 WaveCount)
{
	SetTextSafe(WaveText, FText::Format(LOCTEXT("WaveFormat", "WAVE {0} / {1}"), FCyberText::Int(Wave), FCyberText::Int(WaveCount)));
}

void UDoorRangeHUD::HandleHostilesChanged(int32 Remaining, int32 Total)
{
	SetTextSafe(HostilesText, FText::Format(LOCTEXT("HostilesFormat", "HOSTILES LEFT {0} / {1}"), FCyberText::Int(Remaining), FCyberText::Int(Total)));
}

void UDoorRangeHUD::HandleRangeEvent(EDoorRangeEvent Event, int32 Delta, ADoorSlot* SourceSlot)
{
	switch (Event)
	{
	case EDoorRangeEvent::HostileHit:
		ShowEvent(FText::Format(LOCTEXT("HostileHit", "HOSTILE DOWN  +{0}"), FCyberText::Int(Delta)), DoorRangeHudLook::Good());
		break;
	case EDoorRangeEvent::FriendlyHit:
		ShowEvent(FText::Format(LOCTEXT("FriendlyHit", "FRIENDLY HIT  {0}"), FCyberText::Int(Delta)), DoorRangeHudLook::Bad());
		break;
	case EDoorRangeEvent::HostileEscaped:
		ShowEvent(FText::Format(LOCTEXT("HostileEscaped", "HOSTILE ESCAPED  {0}"), FCyberText::Int(Delta)), DoorRangeHudLook::Warn());
		break;
	case EDoorRangeEvent::HostageRescued:
		ShowEvent(FText::Format(LOCTEXT("HostageRescued", "HOSTAGE FREED  +{0}"), FCyberText::Int(Delta)), DoorRangeHudLook::Good());
		break;
	case EDoorRangeEvent::HostageHit:
		ShowEvent(FText::Format(LOCTEXT("HostageHit", "HOSTAGE HIT  {0}"), FCyberText::Int(Delta)), DoorRangeHudLook::Bad());
		break;
	case EDoorRangeEvent::HostileDisarmed:
		ShowEvent(FText::Format(LOCTEXT("HostileDisarmed", "HOSTILE DISARMED  +{0}"), FCyberText::Int(Delta)), DoorRangeHudLook::Good());
		break;
	case EDoorRangeEvent::WaveStarted:
		ShowEvent(FText::Format(LOCTEXT("WaveStarted", "WAVE {0}"), FCyberText::Int(Delta)), DoorRangeHudLook::Neutral());
		if (SummaryPanel)
		{
			SummaryPanel->SetVisibility(ESlateVisibility::Collapsed);
		}
		break;
	default:
		break;
	}
}

void UDoorRangeHUD::HandleRangeFinished(const FDoorRangeStats& Stats)
{
	const FText RangeSummary = FText::Format(
		LOCTEXT("SummaryFormat", "RANGE COMPLETE\n\nFINAL SCORE {0}\n\nHostiles hit {1} / {2}\nDisarms {7}\nHostiles escaped {3}\nFriendlies hit {4}\nHostages freed {5}, hit {6}"),
		FCyberText::Int(Stats.FinalScore), FCyberText::Int(Stats.HostilesHit), FCyberText::Int(Stats.HostilesTotal),
		FCyberText::Int(Stats.HostilesEscaped), FCyberText::Int(Stats.FriendliesHit), FCyberText::Int(Stats.HostagesRescued), FCyberText::Int(Stats.HostagesHit),
		FCyberText::Int(Stats.Disarms));

	const FText Summary = FText::Format(LOCTEXT("SummaryWithStyle", "{0}\n\n{1}"), RangeSummary, UStyleHUDWidget::FormatRunSummary(GetOwningPlayer()));
	UStyleHUDWidget::LogSummary(Summary);
	SetTextSafe(SummaryText, Summary);
	if (SummaryPanel)
	{
		SummaryPanel->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UDoorRangeHUD::ShowEvent(const FText& Text, const FLinearColor& Color)
{
	if (!EventText)
	{
		return;
	}

	EventText->SetText(Text);
	EventText->SetColorAndOpacity(FSlateColor(Color));
	EventText->SetRenderOpacity(1.0f);
	EventAge = 0.0f;
	bEventVisible = true;
}

void UDoorRangeHUD::SetTextSafe(UTextBlock* Block, const FText& Text)
{
	if (Block)
	{
		Block->SetText(Text);
	}
}

#undef LOCTEXT_NAMESPACE
