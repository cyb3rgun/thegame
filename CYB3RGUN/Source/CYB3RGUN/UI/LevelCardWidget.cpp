// CYB3RGUN THEGAME. One level on the level selection screen.

#include "LevelCardWidget.h"
#include "CyberMenuStyle.h"
#include "GameMenuSubsystem.h"
#include "PlayableLevelDefinition.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"

#define LOCTEXT_NAMESPACE "LevelCard"

void ULevelCardWidget::Setup(UPlayableLevelDefinition* InLevel)
{
	Level = InLevel;
	if (!WidgetTree || WidgetTree->RootWidget || !Level)
	{
		return;
	}

	USizeBox* Card = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Card"));
	Card->SetWidthOverride(CardWidth);
	WidgetTree->RootWidget = Card;

	Button = FCyberMenuStyle::MakePlainButton(WidgetTree, TEXT("CardButton"));
	Card->AddChild(Button);
	Button->OnClicked.AddUniqueDynamic(this, &ULevelCardWidget::HandleClicked);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("CardColumn"));
	if (UButtonSlot* ColumnSlot = Cast<UButtonSlot>(Button->AddChild(Column)))
	{
		ColumnSlot->SetHorizontalAlignment(HAlign_Fill);
		ColumnSlot->SetVerticalAlignment(VAlign_Top);
	}

	// the preview keeps a 16:9 frame, without a picture the frame stays dark
	const float PreviewWidth = CardWidth - 40.0f;
	USizeBox* PreviewBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PreviewBox"));
	PreviewBox->SetWidthOverride(PreviewWidth);
	PreviewBox->SetHeightOverride(PreviewWidth * 9.0f / 16.0f);
	UImage* Preview = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Preview"));
	if (UTexture2D* Texture = Level->Preview.LoadSynchronous())
	{
		Preview->SetBrushFromTexture(Texture);
	}
	else
	{
		Preview->SetColorAndOpacity(FCyberMenuStyle::PlaceholderColor());
	}
	PreviewBox->AddChild(Preview);
	FCyberMenuStyle::AddToColumn(Column, PreviewBox, FMargin(0.0f, 0.0f, 0.0f, 10.0f), HAlign_Fill);

	const FText Kind = Level->Kind == EPlayableLevelKind::Scenario ? LOCTEXT("Scenario", "SCENARIO") : LOCTEXT("TestMap", "TEST MAP");
	UTextBlock* KindText = FCyberMenuStyle::MakeText(WidgetTree, TEXT("Kind"), Kind, EBrandText::Label, 11, FCyberMenuStyle::BrandColor());
	FCyberMenuStyle::AddToColumn(Column, KindText, FMargin(0.0f, 0.0f, 0.0f, 2.0f));

	UTextBlock* Name = FCyberMenuStyle::MakeText(WidgetTree, TEXT("Name"), Level->GetDisplayText(), EBrandText::Title, 24, FCyberMenuStyle::TextColor());
	FCyberMenuStyle::AddToColumn(Column, Name, FMargin(0.0f, 0.0f, 0.0f, 4.0f));

	UTextBlock* Description = FCyberMenuStyle::MakeText(WidgetTree, TEXT("Description"), Level->Description, EBrandText::Body, 15, FCyberMenuStyle::DimTextColor());
	Description->SetAutoWrapText(true);
	FCyberMenuStyle::AddToColumn(Column, Description, FMargin(0.0f, 0.0f, 0.0f, 4.0f), HAlign_Fill);
}

void ULevelCardWidget::HandleClicked()
{
	const UGameInstance* GameInstance = GetGameInstance();
	if (UGameMenuSubsystem* Menu = GameInstance ? GameInstance->GetSubsystem<UGameMenuSubsystem>() : nullptr)
	{
		Menu->StartLevel(Level);
	}
}

#undef LOCTEXT_NAMESPACE
