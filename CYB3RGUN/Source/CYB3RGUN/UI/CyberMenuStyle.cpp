// CYB3RGUN THEGAME. The shared look of the front end.

#include "CyberMenuStyle.h"
#include "BrandStyle.h"
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

FLinearColor FCyberMenuStyle::BrandColor() { return UBrandStyle::Get().GetBrand(); }
FLinearColor FCyberMenuStyle::CounterColor() { return UBrandStyle::Get().GetCounter(); }
FLinearColor FCyberMenuStyle::DangerColor() { return UBrandStyle::Get().GetDanger(); }
FLinearColor FCyberMenuStyle::TextColor() { return UBrandStyle::Get().Text; }
FLinearColor FCyberMenuStyle::DimTextColor() { return UBrandStyle::Get().DimText; }
FLinearColor FCyberMenuStyle::PanelColor() { return UBrandStyle::Get().Panel; }
FLinearColor FCyberMenuStyle::PanelSolidColor() { return UBrandStyle::Get().PanelSolid; }
FLinearColor FCyberMenuStyle::VeilColor() { return UBrandStyle::Get().Veil; }
FLinearColor FCyberMenuStyle::ButtonIdleColor() { return UBrandStyle::Get().ButtonIdle; }
FLinearColor FCyberMenuStyle::TrackColor() { return UBrandStyle::Get().Track; }
FLinearColor FCyberMenuStyle::ShadowColor() { return UBrandStyle::Get().Shadow; }
FLinearColor FCyberMenuStyle::PlaceholderColor() { return UBrandStyle::Get().Placeholder; }

FLinearColor FCyberMenuStyle::ButtonActiveColor()
{
	const UBrandStyle& Style = UBrandStyle::Get();
	return Style.GetBrand().CopyWithNewOpacity(Style.ButtonActiveOpacity);
}

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
	Button->SetBackgroundColor(ButtonIdleColor());
	return Button;
}

UButton* FCyberMenuStyle::MakeButton(UWidgetTree* Tree, const FName& Name, const FText& Label, int32 Size)
{
	UButton* Button = MakePlainButton(Tree, Name);
	const FName LabelName = Name.IsNone() ? FName(NAME_None) : FName(*(Name.ToString() + TEXT("Label")));
	UTextBlock* LabelText = MakeText(Tree, LabelName, Label, Size, TextColor());
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
	const FLinearColor Wanted = Button->HasKeyboardFocus() ? ButtonActiveColor() : ButtonIdleColor();
	if (!Button->GetBackgroundColor().Equals(Wanted))
	{
		Button->SetBackgroundColor(Wanted);
	}
}
