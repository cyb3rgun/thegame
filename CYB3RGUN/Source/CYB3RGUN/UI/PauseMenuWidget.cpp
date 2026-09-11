// CYB3RGUN THEGAME. The pause menu.

#include "PauseMenuWidget.h"
#include "CyberMenuStyle.h"
#include "GameMenuSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"

#define LOCTEXT_NAMESPACE "PauseMenu"

void UPauseMenuWidget::BuildLayout()
{
	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("PauseCanvas"));
	WidgetTree->RootWidget = Canvas;

	// the frozen level stays visible behind a veil, the panel sits in the middle
	UBorder* Veil = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Veil"));
	Veil->SetBrushColor(FCyberMenuStyle::VeilColor);
	Veil->SetHorizontalAlignment(HAlign_Center);
	Veil->SetVerticalAlignment(VAlign_Center);
	if (UCanvasPanelSlot* VeilSlot = Canvas->AddChildToCanvas(Veil))
	{
		VeilSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		VeilSlot->SetOffsets(FMargin(0.0f));
	}

	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Panel"));
	Panel->SetBrushColor(FCyberMenuStyle::PanelColor);
	Panel->SetPadding(FMargin(56.0f, 44.0f));
	Veil->SetContent(Panel);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("PauseColumn"));
	Panel->SetContent(Column);

	UTextBlock* Title = FCyberMenuStyle::MakeText(WidgetTree, TEXT("Title"), LOCTEXT("Title", "PAUSED"), 48, FCyberMenuStyle::TextColor);
	Title->SetFont(FCyberMenuStyle::MakeFont(48, TEXT("Bold"), 200));
	FCyberMenuStyle::AddToColumn(Column, Title, FMargin(0.0f, 0.0f, 0.0f, 32.0f));

	UButton* ResumeButton = AddMenuButton(TEXT("Resume"), LOCTEXT("Resume", "Resume"), 26);
	UButton* SettingsButton = AddMenuButton(TEXT("Settings"), LOCTEXT("Settings", "Settings"), 26);
	UButton* MainMenuButton = AddMenuButton(TEXT("MainMenu"), LOCTEXT("MainMenu", "Return to Main Menu"), 26);
	for (UButton* Button : { ResumeButton, SettingsButton, MainMenuButton })
	{
		FCyberMenuStyle::AddToColumn(Column, FCyberMenuStyle::WrapWidth(WidgetTree, Button, 420.0f), FMargin(0.0f, 0.0f, 0.0f, 12.0f));
	}
	ResumeButton->OnClicked.AddUniqueDynamic(this, &UPauseMenuWidget::HandleResume);
	SettingsButton->OnClicked.AddUniqueDynamic(this, &UPauseMenuWidget::HandleSettings);
	MainMenuButton->OnClicked.AddUniqueDynamic(this, &UPauseMenuWidget::HandleMainMenu);

	UTextBlock* Hint = FCyberMenuStyle::MakeText(WidgetTree, TEXT("Hint"), LOCTEXT("Hint", "Esc or B to resume"), 14, FCyberMenuStyle::DimTextColor, TEXT("Regular"));
	FCyberMenuStyle::AddToColumn(Column, Hint, FMargin(0.0f, 24.0f, 0.0f, 0.0f));
}

void UPauseMenuWidget::HandleResume()
{
	if (UGameMenuSubsystem* Menu = GetGameMenu())
	{
		Menu->Back();
	}
}

void UPauseMenuWidget::HandleSettings()
{
	if (UGameMenuSubsystem* Menu = GetGameMenu())
	{
		Menu->OpenSettings();
	}
}

void UPauseMenuWidget::HandleMainMenu()
{
	if (UGameMenuSubsystem* Menu = GetGameMenu())
	{
		Menu->ReturnToMainMenu();
	}
}

#undef LOCTEXT_NAMESPACE
