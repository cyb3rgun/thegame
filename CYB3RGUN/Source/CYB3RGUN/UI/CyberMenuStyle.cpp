// CYB3RGUN THEGAME. The shared look of the front end.

#include "CyberMenuStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateColorBrush.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateTypes.h"

const FLinearColor FCyberMenuStyle::AccentColor(1.0f, 0.42f, 0.14f, 1.0f);
const FLinearColor FCyberMenuStyle::TextColor(0.93f, 0.94f, 0.96f, 1.0f);
const FLinearColor FCyberMenuStyle::DimTextColor(0.56f, 0.6f, 0.66f, 1.0f);
const FLinearColor FCyberMenuStyle::PanelColor(0.008f, 0.01f, 0.016f, 0.8f);
const FLinearColor FCyberMenuStyle::VeilColor(0.0f, 0.0f, 0.0f, 0.6f);
const FLinearColor FCyberMenuStyle::ButtonIdleColor(0.06f, 0.065f, 0.08f, 0.7f);
const FLinearColor FCyberMenuStyle::ButtonActiveColor(0.85f, 0.28f, 0.08f, 0.95f);

FSlateFontInfo FCyberMenuStyle::MakeFont(int32 Size, const FName& Typeface, int32 LetterSpacing)
{
	FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle(Typeface, Size);
	Font.LetterSpacing = LetterSpacing;
	return Font;
}

UTextBlock* FCyberMenuStyle::MakeText(UWidgetTree* Tree, const FName& Name, const FText& Text, int32 Size, const FLinearColor& Color, const FName& Typeface)
{
	UTextBlock* Block = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
	Block->SetFont(MakeFont(Size, Typeface));
	Block->SetColorAndOpacity(FSlateColor(Color));
	Block->SetText(Text);
	return Block;
}

UButton* FCyberMenuStyle::MakePlainButton(UWidgetTree* Tree, const FName& Name)
{
	// one white brush for every state, tinted through the button's background colour
	static const FButtonStyle Style = []()
	{
		const FSlateColorBrush White(FLinearColor::White);
		FButtonStyle Result;
		Result.SetNormal(White);
		Result.SetHovered(White);
		Result.SetPressed(White);
		Result.SetNormalPadding(FMargin(20.0f, 10.0f));
		Result.SetPressedPadding(FMargin(20.0f, 11.0f, 20.0f, 9.0f));
		return Result;
	}();

	UButton* Button = Tree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
	Button->SetStyle(Style);
	Button->SetBackgroundColor(ButtonIdleColor);
	return Button;
}

UButton* FCyberMenuStyle::MakeButton(UWidgetTree* Tree, const FName& Name, const FText& Label, int32 Size)
{
	UButton* Button = MakePlainButton(Tree, Name);
	const FName LabelName = Name.IsNone() ? FName(NAME_None) : FName(*(Name.ToString() + TEXT("Label")));
	UTextBlock* LabelText = MakeText(Tree, LabelName, Label, Size, TextColor);
	if (UButtonSlot* LabelSlot = Cast<UButtonSlot>(Button->AddChild(LabelText)))
	{
		LabelSlot->SetHorizontalAlignment(HAlign_Left);
		LabelSlot->SetVerticalAlignment(VAlign_Center);
	}
	return Button;
}

USizeBox* FCyberMenuStyle::WrapWidth(UWidgetTree* Tree, UWidget* Content, float Width)
{
	USizeBox* Box = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	Box->SetWidthOverride(Width);
	Box->AddChild(Content);
	return Box;
}

void FCyberMenuStyle::AddToColumn(UVerticalBox* Column, UWidget* Widget, const FMargin& Padding, EHorizontalAlignment Alignment)
{
	if (UVerticalBoxSlot* ColumnSlot = Column->AddChildToVerticalBox(Widget))
	{
		ColumnSlot->SetPadding(Padding);
		ColumnSlot->SetHorizontalAlignment(Alignment);
	}
}

void FCyberMenuStyle::UpdateHighlight(UButton* Button)
{
	if (!Button)
	{
		return;
	}
	const FLinearColor Wanted = Button->HasKeyboardFocus() ? ButtonActiveColor : ButtonIdleColor;
	if (!Button->GetBackgroundColor().Equals(Wanted))
	{
		Button->SetBackgroundColor(Wanted);
	}
}
