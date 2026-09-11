// CYB3RGUN THEGAME. The level selection.

#include "LevelSelectWidget.h"
#include "CyberMenuStyle.h"
#include "GameMenuSubsystem.h"
#include "LevelCardWidget.h"
#include "PlayableLevelDefinition.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/VerticalBox.h"

#define LOCTEXT_NAMESPACE "LevelSelect"

void ULevelSelectWidget::BuildLayout()
{
	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("SelectCanvas"));
	WidgetTree->RootWidget = Canvas;

	// the night range stays visible behind a veil, the cards sit in the middle
	UBorder* Veil = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Veil"));
	Veil->SetBrushColor(FCyberMenuStyle::VeilColor());
	Veil->SetHorizontalAlignment(HAlign_Center);
	Veil->SetVerticalAlignment(VAlign_Center);
	if (UCanvasPanelSlot* VeilSlot = Canvas->AddChildToCanvas(Veil))
	{
		VeilSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		VeilSlot->SetOffsets(FMargin(0.0f));
	}

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SelectColumn"));
	Veil->SetContent(Column);

	UTextBlock* Title = FCyberMenuStyle::MakeText(WidgetTree, TEXT("Title"), LOCTEXT("Title", "SELECT LEVEL"), 44, FCyberMenuStyle::TextColor());
	Title->SetFont(FCyberMenuStyle::MakeFont(44, TEXT("Bold"), 160));
	FCyberMenuStyle::AddToColumn(Column, Title, FMargin(8.0f, 0.0f, 0.0f, 24.0f));

	const TArray<UPlayableLevelDefinition*> Levels = UPlayableLevelDefinition::LoadAll();
	for (const EPlayableLevelKind Kind : { EPlayableLevelKind::Scenario, EPlayableLevelKind::TestMap })
	{
		const TArray<UPlayableLevelDefinition*> OfKind = Levels.FilterByPredicate([Kind](const UPlayableLevelDefinition* Level) { return Level->Kind == Kind; });
		if (OfKind.Num() == 0)
		{
			continue;
		}

		const FText Heading = Kind == EPlayableLevelKind::Scenario ? LOCTEXT("Scenarios", "SCENARIOS") : LOCTEXT("TestMaps", "TEST MAPS");
		UTextBlock* HeadingText = FCyberMenuStyle::MakeText(WidgetTree, NAME_None, Heading, 16, FCyberMenuStyle::BrandColor());
		HeadingText->SetFont(FCyberMenuStyle::MakeFont(16, TEXT("Bold"), 200));
		FCyberMenuStyle::AddToColumn(Column, HeadingText, FMargin(8.0f, 8.0f, 0.0f, 4.0f));

		UUniformGridPanel* Grid = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass());
		Grid->SetSlotPadding(FMargin(8.0f));
		for (int32 Index = 0; Index < OfKind.Num(); ++Index)
		{
			ULevelCardWidget* Card = CreateWidget<ULevelCardWidget>(this, ULevelCardWidget::StaticClass());
			Card->Setup(OfKind[Index]);
			Grid->AddChildToUniformGrid(Card, Index / CardsPerRow, Index % CardsPerRow);
			RegisterButton(Card->GetButton());
			Cards.Add(Card);
		}
		FCyberMenuStyle::AddToColumn(Column, Grid, FMargin(0.0f, 0.0f, 0.0f, 12.0f));
	}

	if (Cards.Num() == 0)
	{
		UTextBlock* Empty = FCyberMenuStyle::MakeText(WidgetTree, TEXT("Empty"), LOCTEXT("Empty", "No playable levels found. Level definitions live in Core/Levels."), 18, FCyberMenuStyle::DimTextColor(), TEXT("Regular"));
		FCyberMenuStyle::AddToColumn(Column, Empty, FMargin(8.0f, 0.0f, 0.0f, 12.0f));
	}

	UButton* BackButton = AddMenuButton(TEXT("Back"), LOCTEXT("Back", "Back"), 22);
	BackButton->OnClicked.AddUniqueDynamic(this, &ULevelSelectWidget::HandleBack);
	FCyberMenuStyle::AddToColumn(Column, FCyberMenuStyle::WrapWidth(WidgetTree, BackButton, 220.0f), FMargin(8.0f, 16.0f, 0.0f, 0.0f));

	UTextBlock* Hint = FCyberMenuStyle::MakeText(WidgetTree, TEXT("Hint"), LOCTEXT("Hint", "Enter or A to start, Esc or B to go back"), 14, FCyberMenuStyle::DimTextColor(), TEXT("Regular"));
	FCyberMenuStyle::AddToColumn(Column, Hint, FMargin(8.0f, 18.0f, 0.0f, 0.0f));
}

void ULevelSelectWidget::HandleBack()
{
	if (UGameMenuSubsystem* Menu = GetGameMenu())
	{
		Menu->Back();
	}
}

#undef LOCTEXT_NAMESPACE
