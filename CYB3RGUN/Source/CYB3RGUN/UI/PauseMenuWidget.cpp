// CYB3RGUN THEGAME. The pause menu.

#include "PauseMenuWidget.h"
#include "CyberMenuStyle.h"
#include "BrandFrame.h"
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

	UBrandFrame* Panel = FCyberMenuStyle::MakeFrame(WidgetTree, TEXT("Panel"), FMargin(56.0f, 44.0f));
	Veil->SetContent(Panel);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("PauseColumn"));
	Panel->SetContent(Column);

	UTextBlock* Title = FCyberMenuStyle::MakeText(WidgetTree, TEXT("Title"), LOCTEXT("Title", "PAUSED"), EBrandText::Heading, 34, FCyberMenuStyle::BrandColor());
	FCyberMenuStyle::AddToColumn(Column, Title, FMargin(0.0f, 0.0f, 0.0f, 6.0f));
	FCyberMenuStyle::AddToColumn(Column, FCyberMenuStyle::MakeRule(WidgetTree, 420.0f, false), FMargin(0.0f, 0.0f, 0.0f, 28.0f));
	AddLoadout(Column);

	UButton* ResumeButton = AddMenuButton(TEXT("Resume"), LOCTEXT("Resume", "Resume"), 20);
	UButton* SettingsButton = AddMenuButton(TEXT("Settings"), LOCTEXT("Settings", "Settings"), 20);
	UButton* MainMenuButton = AddMenuButton(TEXT("MainMenu"), LOCTEXT("MainMenu", "Main Menu"), 20);
	for (UButton* Button : { ResumeButton, SettingsButton, MainMenuButton })
	{
		FCyberMenuStyle::AddToColumn(Column, FCyberMenuStyle::WrapWidth(WidgetTree, Button, 430.0f), FMargin(0.0f, 0.0f, 0.0f, 12.0f));
	}
	ResumeButton->OnClicked.AddUniqueDynamic(this, &UPauseMenuWidget::HandleResume);
	SettingsButton->OnClicked.AddUniqueDynamic(this, &UPauseMenuWidget::HandleSettings);
	MainMenuButton->OnClicked.AddUniqueDynamic(this, &UPauseMenuWidget::HandleMainMenu);

	UTextBlock* Hint = FCyberMenuStyle::MakeText(WidgetTree, TEXT("Hint"), LOCTEXT("Hint", "Esc or B to resume"), EBrandText::Label, 12, FCyberMenuStyle::FaintTextColor());
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

	UTextBlock* Heading = FCyberMenuStyle::MakeText(WidgetTree, TEXT("LoadoutHeading"), LOCTEXT("Loadout", "LOADOUT"), EBrandText::Label, 13, FCyberMenuStyle::FaintTextColor());
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
		UTextBlock* Name = FCyberMenuStyle::MakeText(WidgetTree, NAME_None, Weapon.WeaponName, EBrandText::Title, 24, Color);
		if (UHorizontalBoxSlot* NameSlot = Row->AddChildToHorizontalBox(Name))
		{
			NameSlot->SetVerticalAlignment(VAlign_Center);
		}

		// what kind of weapon it is, quieter beside the name (D-053)
		if (!Weapon.Subtitle.IsEmpty())
		{
			UTextBlock* Subtitle = FCyberMenuStyle::MakeText(WidgetTree, NAME_None, Weapon.Subtitle, EBrandText::Body, 16, FCyberMenuStyle::DimTextColor());
			if (UHorizontalBoxSlot* SubtitleSlot = Row->AddChildToHorizontalBox(Subtitle))
			{
				SubtitleSlot->SetVerticalAlignment(VAlign_Center);
				SubtitleSlot->SetPadding(FMargin(14.0f, 0.0f, 0.0f, 0.0f));
			}
		}
		FCyberMenuStyle::AddToColumn(Column, Row, FMargin(0.0f, 0.0f, 0.0f, 6.0f));
	}

	// the loadout and the buttons are two groups
	UTextBlock* Spacer = FCyberMenuStyle::MakeText(WidgetTree, NAME_None, FText::GetEmpty(), EBrandText::Body, 8, FCyberMenuStyle::DimTextColor());
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
