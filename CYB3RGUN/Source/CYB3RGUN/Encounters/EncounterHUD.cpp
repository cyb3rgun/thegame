// CYB3RGUN THEGAME. HUD for encounters.

#include "EncounterHUD.h"
#include "EncounterDirector.h"
#include "CyberEnemy.h"
#include "EnemyDefinition.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Styling/CoreStyle.h"

#define LOCTEXT_NAMESPACE "EncounterHUD"

namespace
{
	const FLinearColor ColorNeutral(0.95f, 0.95f, 0.95f, 1.0f);
	const FLinearColor ColorGood(0.2f, 1.0f, 0.3f, 1.0f);
	const FLinearColor ColorWarn(1.0f, 0.7f, 0.1f, 1.0f);
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
		UTextBlock* Block = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		Block->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", Size));
		Block->SetColorAndOpacity(FSlateColor(Color));
		Block->SetShadowOffset(FVector2D(2.0f, 2.0f));
		Block->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.8f));
		return Block;
	};

	UVerticalBox* TopLeft = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("FallbackTopLeft"));
	if (!WaveText)
	{
		WaveText = MakeText(TEXT("WaveText"), 36, ColorNeutral);
		TopLeft->AddChildToVerticalBox(WaveText);
	}
	if (!AliveText)
	{
		AliveText = MakeText(TEXT("AliveText"), 24, ColorWarn);
		TopLeft->AddChildToVerticalBox(AliveText);
	}
	if (!KillsText)
	{
		KillsText = MakeText(TEXT("KillsText"), 24, ColorNeutral);
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
		EventText = MakeText(TEXT("EventText"), 40, ColorNeutral);
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
		Border->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.75f));
		Border->SetPadding(FMargin(32.0f, 24.0f));
		SummaryText = MakeText(TEXT("SummaryText"), 28, ColorNeutral);
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

	SetTextSafe(WaveText, FText::Format(LOCTEXT("WaveFormat", "WAVE {0} / {1}  {2}"), FText::AsNumber(Wave), FText::AsNumber(WaveCount), WaveName));
	ShowEvent(FText::Format(LOCTEXT("WaveEvent", "WAVE {0}  {1}"), FText::AsNumber(Wave), WaveName), ColorNeutral);
	if (SummaryPanel)
	{
		SummaryPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UEncounterHUD::HandleCountsChanged(int32 Alive, int32 Kills, int32 Total)
{
	SetTextSafe(AliveText, FText::Format(LOCTEXT("AliveFormat", "ALIVE {0}"), FText::AsNumber(Alive)));
	SetTextSafe(KillsText, FText::Format(LOCTEXT("KillsFormat", "KILLS {0} / {1}"), FText::AsNumber(Kills), FText::AsNumber(Total)));
}

void UEncounterHUD::HandleEnemyKilled(ACyberEnemy* Enemy, int32 Kills)
{
	const FText Name = (Enemy && Enemy->GetDefinition()) ? Enemy->GetDefinition()->DisplayName : LOCTEXT("Enemy", "ENEMY");
	ShowEvent(FText::Format(LOCTEXT("KillEvent", "{0} DOWN"), Name.ToUpper()), ColorGood);
}

void UEncounterHUD::HandleFinished(int32 Kills, float Seconds)
{
	ShowSummary(FText::Format(LOCTEXT("SummaryFormat", "ENCOUNTER COMPLETE\n\nKills {0}\nTime {1} s"), FText::AsNumber(Kills), FText::AsNumber(FMath::RoundToInt(Seconds))));
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
