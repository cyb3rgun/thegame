// CYB3RGUN THEGAME. The pause menu.

#include "PauseMenuWidget.h"
#include "CyberMenuStyle.h"
#include "GameMenuSubsystem.h"
#include "WeaponStatus.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Engine/Texture2D.h"
#include "GameFramework/Pawn.h"

#define LOCTEXT_NAMESPACE "PauseMenu"

void UPauseMenuWidget::BuildLayout()
{
	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("PauseCanvas"));
	WidgetTree->RootWidget = Canvas;

	// the frozen level stays visible behind a veil, the panel sits in the middle
	UBorder* Veil = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Veil"));
	Veil->SetBrushColor(FCyberMenuStyle::VeilColor());
	Veil->SetHorizontalAlignment(HAlign_Center);
	Veil->SetVerticalAlignment(VAlign_Center);
	if (UCanvasPanelSlot* VeilSlot = Canvas->AddChildToCanvas(Veil))
	{
		VeilSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		VeilSlot->SetOffsets(FMargin(0.0f));
	}

	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Panel"));
	Panel->SetBrushColor(FCyberMenuStyle::PanelColor());
	Panel->SetPadding(FMargin(56.0f, 44.0f));
	Veil->SetContent(Panel);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("PauseColumn"));
	Panel->SetContent(Column);

	UTextBlock* Title = FCyberMenuStyle::MakeText(WidgetTree, TEXT("Title"), LOCTEXT("Title", "PAUSED"), 48, FCyberMenuStyle::TextColor());
	Title->SetFont(FCyberMenuStyle::MakeFont(48, TEXT("Bold"), 200));
	FCyberMenuStyle::AddToColumn(Column, Title, FMargin(0.0f, 0.0f, 0.0f, 32.0f));
	AddLoadout(Column);

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

	UTextBlock* Hint = FCyberMenuStyle::MakeText(WidgetTree, TEXT("Hint"), LOCTEXT("Hint", "Esc or B to resume"), 14, FCyberMenuStyle::DimTextColor(), TEXT("Regular"));
	FCyberMenuStyle::AddToColumn(Column, Hint, FMargin(0.0f, 24.0f, 0.0f, 0.0f));
}

void UPauseMenuWidget::AddLoadout(UVerticalBox* Column)
{
	TArray<FWeaponStatus> Loadout;
	FWeaponStatus InHand;
	const IWeaponStatusSource* Source = Cast<IWeaponStatusSource>(GetOwningPlayerPawn());
	if (!Source)
	{
		return;
	}
	Source->GetLoadout(Loadout);
	const bool bHolding = Source->GetWeaponStatus(InHand);
	if (Loadout.IsEmpty())
	{
		return;
	}

	UTextBlock* Heading = FCyberMenuStyle::MakeText(WidgetTree, TEXT("LoadoutHeading"), LOCTEXT("Loadout", "LOADOUT"), 16, FCyberMenuStyle::BrandColor());
	Heading->SetFont(FCyberMenuStyle::MakeFont(16, TEXT("Bold"), 240));
	FCyberMenuStyle::AddToColumn(Column, Heading, FMargin(0.0f, 0.0f, 0.0f, 8.0f));

	for (const FWeaponStatus& Weapon : Loadout)
	{
		const bool bCurrent = bHolding && Weapon.WeaponIndex == InHand.WeaponIndex;
		const FLinearColor Color = bCurrent ? FCyberMenuStyle::TextColor() : FCyberMenuStyle::DimTextColor();
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		if (Weapon.MakerMark)
		{
			UImage* Mark = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
			Mark->SetBrushFromTexture(Weapon.MakerMark, false);
			Mark->SetDesiredSizeOverride(FVector2D(36.0f, 36.0f));
			Mark->SetColorAndOpacity(Color);
			if (UHorizontalBoxSlot* MarkSlot = Row->AddChildToHorizontalBox(Mark))
			{
				MarkSlot->SetVerticalAlignment(VAlign_Center);
				MarkSlot->SetPadding(FMargin(0.0f, 0.0f, 12.0f, 0.0f));
			}
		}
		UTextBlock* Name = FCyberMenuStyle::MakeText(WidgetTree, NAME_None, Weapon.WeaponName, 22, Color);
		if (UHorizontalBoxSlot* NameSlot = Row->AddChildToHorizontalBox(Name))
		{
			NameSlot->SetVerticalAlignment(VAlign_Center);
		}
		FCyberMenuStyle::AddToColumn(Column, Row, FMargin(0.0f, 0.0f, 0.0f, 6.0f));
	}

	// the loadout and the buttons are two groups
	UTextBlock* Spacer = FCyberMenuStyle::MakeText(WidgetTree, NAME_None, FText::GetEmpty(), 8, FCyberMenuStyle::DimTextColor());
	FCyberMenuStyle::AddToColumn(Column, Spacer, FMargin(0.0f, 0.0f, 0.0f, 20.0f));
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
