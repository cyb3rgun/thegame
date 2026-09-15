// CYB3RGUN THEGAME. HUD for encounters.

#include "EncounterHUD.h"
#include "EncounterDirector.h"
#include "CyberEnemy.h"
#include "CyberText.h"
#include "CyberMenuStyle.h"
#include "HoloHUD.h"
#include "EnemyDefinition.h"
#include "StyleHUDWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "BrandFrame.h"

#define LOCTEXT_NAMESPACE "EncounterHUD"

// a named namespace with unique names, unity builds merge this file with the door range HUD and its own helpers
namespace EncounterHudLook
{
	FLinearColor Neutral() { return FCyberMenuStyle::TextColor(); }
	FLinearColor Good() { return FCyberMenuStyle::BrandColor(); }
	FLinearColor Warn() { return FCyberMenuStyle::CounterColor(); }

	void Restyle(UTextBlock* Line, const FLinearColor& Color)
	{
		if (Line)
		{
			Line->SetColorAndOpacity(FSlateColor(Color));
		}
	}
}

TSharedRef<SWidget> UEncounterHUD::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget && !HasAnyFlags(RF_ClassDefaultObject))
	{
		BuildFallbackLayout();
	}
	return Super::RebuildWidget();
}

void UEncounterHUD::BuildFallbackLayout()
{
	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("FallbackCanvas"));
	WidgetTree->RootWidget = Canvas;

	auto MakeText = [this](const FName& Name, int32 Size, const FLinearColor& Color) -> UTextBlock*
	{
		// the fallback lines take their roles in NativeConstruct, like the lines of a Blueprint layout
		UTextBlock* Block = FCyberMenuStyle::MakeText(WidgetTree, Name, FText::GetEmpty(), EBrandText::Readout, Size, Color);
		Block->SetShadowOffset(FVector2D(2.0f, 2.0f));
		Block->SetShadowColorAndOpacity(FCyberMenuStyle::ShadowColor());
		return Block;
	};

	UVerticalBox* TopLeft = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("FallbackTopLeft"));
	if (!WaveText)
	{
		WaveText = MakeText(TEXT("WaveText"), 36, EncounterHudLook::Neutral());
		TopLeft->AddChildToVerticalBox(WaveText);
	}
	if (!AliveText)
	{
		AliveText = MakeText(TEXT("AliveText"), 24, EncounterHudLook::Warn());
		TopLeft->AddChildToVerticalBox(AliveText);
	}
	if (!KillsText)
	{
		KillsText = MakeText(TEXT("KillsText"), 24, EncounterHudLook::Neutral());
		TopLeft->AddChildToVerticalBox(KillsText);
	}
	if (UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(TopLeft))
	{
		PanelSlot->SetAnchors(FAnchors(0.0f, 0.0f));
		PanelSlot->SetPosition(FVector2D(40.0f, 120.0f));
		PanelSlot->SetAutoSize(true);
	}

	if (!EventText)
	{
		EventText = MakeText(TEXT("EventText"), 40, EncounterHudLook::Neutral());
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
		UBorder* Border = FCyberMenuStyle::MakeFrame(WidgetTree, TEXT("SummaryPanel"), FMargin(40.0f, 30.0f));
		SummaryText = MakeText(TEXT("SummaryText"), 28, EncounterHudLook::Neutral());
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

void UEncounterHUD::NativeConstruct()
{
	Super::NativeConstruct();

	// the Blueprint layout bakes its own fonts and colours, the brand style decides them (D-082)
	FCyberMenuStyle::ApplyRole(WaveText, EBrandText::Value, 40);
	FCyberMenuStyle::ApplyRole(AliveText, EBrandText::Readout, 18);
	FCyberMenuStyle::ApplyRole(KillsText, EBrandText::Readout, 18);
	FCyberMenuStyle::ApplyRole(EventText, EBrandText::Title, 40);
	FCyberMenuStyle::ApplyRole(SummaryText, EBrandText::Readout, 18);
	EncounterHudLook::Restyle(WaveText, EncounterHudLook::Neutral());
	EncounterHudLook::Restyle(AliveText, EncounterHudLook::Warn());
	EncounterHudLook::Restyle(KillsText, EncounterHudLook::Neutral());
	EncounterHudLook::Restyle(SummaryText, EncounterHudLook::Neutral());
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

void UEncounterHUD::NativeDestruct()
{
	UnbindDirector();
	Super::NativeDestruct();
}

void UEncounterHUD::BindDirector(AEncounterDirector* InDirector)
{
	if (InDirector == Director)
	{
		RefreshAll();
		return;
	}

	UnbindDirector();
	Director = InDirector;

	if (Director)
	{
		Director->OnWaveStarted.AddUniqueDynamic(this, &UEncounterHUD::HandleWaveStarted);
		Director->OnCountsChanged.AddUniqueDynamic(this, &UEncounterHUD::HandleCountsChanged);
		Director->OnEnemyKilled.AddUniqueDynamic(this, &UEncounterHUD::HandleEnemyKilled);
		Director->OnEncounterFinished.AddUniqueDynamic(this, &UEncounterHUD::HandleFinished);
	}

	RefreshAll();
}

void UEncounterHUD::UnbindDirector()
{
	if (Director)
	{
		Director->OnWaveStarted.RemoveDynamic(this, &UEncounterHUD::HandleWaveStarted);
		Director->OnCountsChanged.RemoveDynamic(this, &UEncounterHUD::HandleCountsChanged);
		Director->OnEnemyKilled.RemoveDynamic(this, &UEncounterHUD::HandleEnemyKilled);
		Director->OnEncounterFinished.RemoveDynamic(this, &UEncounterHUD::HandleFinished);
		Director = nullptr;
	}
}

void UEncounterHUD::RefreshAll()
{
	if (!Director)
	{
		SetTextSafe(WaveText, LOCTEXT("NoDirector", "NO ENCOUNTER"));
		SetTextSafe(AliveText, FText::GetEmpty());
		SetTextSafe(KillsText, FText::GetEmpty());
		return;
	}

	HandleWaveStarted(Director->GetCurrentWave(), Director->GetWaveCount(), Director->GetCurrentWaveName());
	HandleCountsChanged(Director->GetAliveCount(), Director->GetKills(), Director->GetTotalEnemies());
}

void UEncounterHUD::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
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

void UEncounterHUD::HandleWaveStarted(int32 Wave, int32 WaveCount, FText WaveName)
{
	if (Wave <= 0)
	{
		SetTextSafe(WaveText, LOCTEXT("WaveWaiting", "WAVE  -"));
		return;
	}

	SetTextSafe(WaveText, FText::Format(LOCTEXT("WaveFormat", "WAVE {0} / {1}  {2}"), FCyberText::Int(Wave), FCyberText::Int(WaveCount), WaveName));
	ShowEvent(FText::Format(LOCTEXT("WaveEvent", "WAVE {0}  {1}"), FCyberText::Int(Wave), WaveName), EncounterHudLook::Neutral());
	if (SummaryPanel)
	{
		SummaryPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UEncounterHUD::HandleCountsChanged(int32 Alive, int32 Kills, int32 Total)
{
	SetTextSafe(AliveText, FText::Format(LOCTEXT("AliveFormat", "ALIVE {0}"), FCyberText::Int(Alive)));
	SetTextSafe(KillsText, FText::Format(LOCTEXT("KillsFormat", "KILLS {0} / {1}"), FCyberText::Int(Kills), FCyberText::Int(Total)));
}

void UEncounterHUD::HandleEnemyKilled(ACyberEnemy* Enemy, int32 Kills)
{
	const FText Name = (Enemy && Enemy->GetDefinition()) ? Enemy->GetDefinition()->DisplayName : LOCTEXT("Enemy", "ENEMY");
	ShowEvent(FText::Format(LOCTEXT("KillEvent", "{0} DOWN"), Name.ToUpper()), EncounterHudLook::Good());
}

void UEncounterHUD::HandleFinished(int32 Kills, float Seconds)
{
	const FText EncounterSummary = FText::Format(LOCTEXT("SummaryFormat", "ENCOUNTER COMPLETE\n\nKills {0}\nTime {1} s"), FCyberText::Int(Kills), FCyberText::Int(FMath::RoundToInt(Seconds)));
	const FText Summary = FText::Format(LOCTEXT("SummaryWithStyle", "{0}\n\n{1}"), EncounterSummary, UStyleHUDWidget::FormatRunSummary(GetOwningPlayer()));
	UStyleHUDWidget::LogSummary(Summary);
	ShowSummary(Summary);
}

void UEncounterHUD::ShowSummary(const FText& Text)
{
	SetTextSafe(SummaryText, Text);
	if (SummaryPanel)
	{
		SummaryPanel->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UEncounterHUD::HideSummary()
{
	if (SummaryPanel)
	{
		SummaryPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UEncounterHUD::ShowEvent(const FText& Text, const FLinearColor& Color)
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

void UEncounterHUD::SetTextSafe(UTextBlock* Block, const FText& Text)
{
	if (Block)
	{
		Block->SetText(Text);
	}
}

#undef LOCTEXT_NAMESPACE
