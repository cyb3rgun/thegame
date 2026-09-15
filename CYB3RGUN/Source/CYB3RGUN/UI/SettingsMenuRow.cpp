// CYB3RGUN THEGAME. One option row of the settings menu.

#include "SettingsMenuRow.h"
#include "CyberMenuStyle.h"
#include "SettingsMenuWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"

TSharedRef<SWidget> USettingsMenuRow::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget && !HasAnyFlags(RF_ClassDefaultObject))
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Row"));
		WidgetTree->RootWidget = Row;

		auto MakeText = [this](const FName& Name, EBrandText Role, int32 Size, const FLinearColor& Color)
		{
			return FCyberMenuStyle::MakeText(WidgetTree, Name, FText::GetEmpty(), Role, Size, Color);
		};

		auto MakeButton = [this](const FName& Name, const TCHAR* Glyph)
		{
			return FCyberMenuStyle::MakeButton(WidgetTree, Name, FText::FromString(Glyph), 16);
		};

		USizeBox* LabelBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("LabelBox"));
		LabelBox->SetWidthOverride(520.0f);
		LabelText = MakeText(TEXT("Label"), EBrandText::Body, 18, FCyberMenuStyle::TextColor());
		LabelBox->AddChild(LabelText);
		if (UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(LabelBox))
		{
			LabelSlot->SetVerticalAlignment(VAlign_Center);
		}

		PrevButton = MakeButton(TEXT("Prev"), TEXT("<"));
		Row->AddChildToHorizontalBox(PrevButton);

		USizeBox* ValueBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ValueBox"));
		ValueBox->SetWidthOverride(360.0f); // wide enough for "On, needs virtual shadows"
		ValueText = MakeText(TEXT("Value"), EBrandText::Title, 20, FCyberMenuStyle::TextColor());
		ValueText->SetJustification(ETextJustify::Center);
		ValueBox->AddChild(ValueText);
		if (UHorizontalBoxSlot* ValueSlot = Row->AddChildToHorizontalBox(ValueBox))
		{
			ValueSlot->SetVerticalAlignment(VAlign_Center);
		}

		NextButton = MakeButton(TEXT("Next"), TEXT(">"));
		Row->AddChildToHorizontalBox(NextButton);

		// measured cost of the shown value, right aligned so the figures line up
		USizeBox* CostBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("CostBox"));
		CostBox->SetWidthOverride(150.0f);
		CostText = MakeText(TEXT("Cost"), EBrandText::Readout, 13, FCyberMenuStyle::DimTextColor());
		CostText->SetJustification(ETextJustify::Right);
		CostBox->AddChild(CostText);
		if (UHorizontalBoxSlot* CostSlot = Row->AddChildToHorizontalBox(CostBox))
		{
			CostSlot->SetVerticalAlignment(VAlign_Center);
		}

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

void USettingsMenuRow::Refresh(const FText& Label, const FText& Value, bool bExperimental, const FText& Cost)
{
	if (CostText)
	{
		CostText->SetText(Cost);
	}
	if (LabelText)
	{
		LabelText->SetText(Label);
		LabelText->SetColorAndOpacity(FSlateColor(bExperimental ? FCyberMenuStyle::CounterColor() : FCyberMenuStyle::TextColor()));
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
