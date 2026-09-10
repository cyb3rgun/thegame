// CYB3RGUN THEGAME. One option row of the settings menu.

#include "SettingsMenuRow.h"
#include "SettingsMenuWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Styling/CoreStyle.h"

namespace
{
	const FLinearColor RowLabelColor(0.85f, 0.85f, 0.85f, 1.0f);
	const FLinearColor RowValueColor(1.0f, 1.0f, 1.0f, 1.0f);
	const FLinearColor RowExperimentalColor(1.0f, 0.55f, 0.15f, 1.0f);
}

TSharedRef<SWidget> USettingsMenuRow::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget && !HasAnyFlags(RF_ClassDefaultObject))
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Row"));
		WidgetTree->RootWidget = Row;

		auto MakeText = [this](const FName& Name, int32 Size, const FLinearColor& Color)
		{
			UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
			Text->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", Size));
			Text->SetColorAndOpacity(FSlateColor(Color));
			return Text;
		};

		auto MakeButton = [this, &MakeText](const FName& Name, const TCHAR* Glyph)
		{
			UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
			UTextBlock* Text = MakeText(*(Name.ToString() + TEXT("Text")), 18, RowValueColor);
			Text->SetText(FText::FromString(Glyph));
			Button->AddChild(Text);
			return Button;
		};

		USizeBox* LabelBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("LabelBox"));
		LabelBox->SetWidthOverride(520.0f);
		LabelText = MakeText(TEXT("Label"), 18, RowLabelColor);
		LabelBox->AddChild(LabelText);
		if (UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(LabelBox))
		{
			LabelSlot->SetVerticalAlignment(VAlign_Center);
		}

		PrevButton = MakeButton(TEXT("Prev"), TEXT("<"));
		Row->AddChildToHorizontalBox(PrevButton);

		USizeBox* ValueBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ValueBox"));
		ValueBox->SetWidthOverride(360.0f); // wide enough for "On, needs virtual shadows"
		ValueText = MakeText(TEXT("Value"), 18, RowValueColor);
		ValueText->SetJustification(ETextJustify::Center);
		ValueBox->AddChild(ValueText);
		if (UHorizontalBoxSlot* ValueSlot = Row->AddChildToHorizontalBox(ValueBox))
		{
			ValueSlot->SetVerticalAlignment(VAlign_Center);
		}

		NextButton = MakeButton(TEXT("Next"), TEXT(">"));
		Row->AddChildToHorizontalBox(NextButton);

		PrevButton->OnClicked.AddUniqueDynamic(this, &USettingsMenuRow::HandlePrev);
		NextButton->OnClicked.AddUniqueDynamic(this, &USettingsMenuRow::HandleNext);
	}
	return Super::RebuildWidget();
}

void USettingsMenuRow::Setup(USettingsMenuWidget* InMenu, ECyberSettingOption InOption)
{
	Menu = InMenu;
	Option = InOption;
}

void USettingsMenuRow::Refresh(const FText& Label, const FText& Value, bool bExperimental)
{
	if (LabelText)
	{
		LabelText->SetText(Label);
		LabelText->SetColorAndOpacity(FSlateColor(bExperimental ? RowExperimentalColor : RowLabelColor));
	}
	if (ValueText)
	{
		ValueText->SetText(Value);
	}
}

void USettingsMenuRow::HandlePrev()
{
	if (USettingsMenuWidget* Owner = Menu.Get())
	{
		Owner->StepOption(Option, -1);
	}
}

void USettingsMenuRow::HandleNext()
{
	if (USettingsMenuWidget* Owner = Menu.Get())
	{
		Owner->StepOption(Option, 1);
	}
}
