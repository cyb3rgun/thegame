// CYB3RGUN THEGAME. The shared look of the front end and the HUDs.

#include "CyberMenuStyle.h"
#include "BrandFrame.h"
#include "BrandStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Font.h"
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
FLinearColor FCyberMenuStyle::NearWhiteColor() { return UBrandStyle::Get().GetNearWhite(); }
FLinearColor FCyberMenuStyle::FaintTextColor() { return UBrandStyle::Get().FaintText; }
FLinearColor FCyberMenuStyle::GroundColor() { return UBrandStyle::Get().Ground; }
FLinearColor FCyberMenuStyle::HairlineColor() { return UBrandStyle::Get().GetHairline(); }

FLinearColor FCyberMenuStyle::ButtonActiveColor()
{
	const UBrandStyle& Style = UBrandStyle::Get();
	return Style.GetBrand().CopyWithNewOpacity(Style.ButtonActiveOpacity);
}

FSlateFontInfo FCyberMenuStyle::MakeFont(EBrandText Role, int32 Size)
{
	const UBrandStyle& Brand = UBrandStyle::Get();
	const FBrandTextStyle& Style = Brand.GetTextStyle(Role);
	FSlateFontInfo Font = Style.Font ? FSlateFontInfo(Style.Font.Get(), Size, Style.Typeface) : FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), Size);
	Font.LetterSpacing = Style.LetterSpacing;
	if (Role == EBrandText::Wordmark)
	{
		// the website fills the word with a gradient and gives Michroma weight with a thin light stroke
		Font.FontMaterial = Brand.WordmarkMaterial;
		Font.OutlineSettings.OutlineSize = 1;
		Font.OutlineSettings.OutlineColor = Brand.GetNearWhite().CopyWithNewOpacity(0.9f);
	}
	return Font;
}

UTextBlock* FCyberMenuStyle::MakeText(UWidgetTree* Tree, const FName& Name, const FText& Text, EBrandText Role, int32 Size, const FLinearColor& Color)
{
	UTextBlock* Block = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
	ApplyRole(Block, Role, Size);
	Block->SetColorAndOpacity(FSlateColor(Color));
	Block->SetText(Text);
	return Block;
}

void FCyberMenuStyle::ApplyRole(UTextBlock* Block, EBrandText Role, int32 Size)
{
	if (!Block)
	{
		return;
	}
	Block->SetFont(MakeFont(Role, Size));
	Block->SetTextTransformPolicy(UBrandStyle::Get().GetTextStyle(Role).bUppercase ? ETextTransformPolicy::ToUpper : ETextTransformPolicy::None);
}

UVerticalBox* FCyberMenuStyle::MakeReadout(UWidgetTree* Tree, const FName& Name, const FText& Label, int32 ValueSize, const FLinearColor& ValueColor, UTextBlock*& OutValue,
	EHorizontalAlignment Alignment)
{
	UVerticalBox* Column = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), Name);
	const ETextJustify::Type Justify = Alignment == HAlign_Right ? ETextJustify::Right : (Alignment == HAlign_Center ? ETextJustify::Center : ETextJustify::Left);

	// the label is micro type, never under the website's 11 px floor, and shadowed like the value so it holds over a bright sky
	UTextBlock* LabelBlock = MakeText(Tree, NAME_None, Label, EBrandText::Label, FMath::Max(12, ValueSize / 3), DimTextColor());
	LabelBlock->SetJustification(Justify);
	LabelBlock->SetShadowOffset(FVector2D(1.0f, 1.0f));
	LabelBlock->SetShadowColorAndOpacity(ShadowColor());
	AddToColumn(Column, LabelBlock, FMargin(0.0f), Alignment);

	OutValue = MakeText(Tree, Name.IsNone() ? FName(NAME_None) : FName(*(Name.ToString() + TEXT("Value"))), FText::GetEmpty(), EBrandText::Value, ValueSize, ValueColor);
	OutValue->SetJustification(Justify);
	OutValue->SetShadowOffset(FVector2D(2.0f, 2.0f));
	OutValue->SetShadowColorAndOpacity(ShadowColor());
	AddToColumn(Column, OutValue, FMargin(0.0f, -FMath::RoundToFloat(ValueSize * 0.1f), 0.0f, 0.0f), Alignment);
	return Column;
}

UBrandFrame* FCyberMenuStyle::MakeFrame(UWidgetTree* Tree, const FName& Name, const FMargin& Padding, bool bCornerTicks)
{
	UBrandFrame* Frame = Tree->ConstructWidget<UBrandFrame>(UBrandFrame::StaticClass(), Name);
	Frame->bCornerTicks = bCornerTicks;
	Frame->SetPadding(Padding);
	return Frame;
}

UBrandRule* FCyberMenuStyle::MakeRule(UWidgetTree* Tree, float Length, bool bFadeFromStart)
{
	UBrandRule* Rule = Tree->ConstructWidget<UBrandRule>(UBrandRule::StaticClass());
	Rule->Length = Length;
	Rule->bFadeFromStart = bFadeFromStart;
	return Rule;
}

namespace CyberMenuButtons
{
	/** Hard edged, one pixel frame: the plate border without the focus, the brand colour and its faint fill with it (D-082) */
	FButtonStyle Make(bool bActive)
	{
		const UBrandStyle& Style = UBrandStyle::Get();
		const FLinearColor Fill = bActive ? Style.GetBrand().CopyWithNewOpacity(Style.ButtonActiveFill) : Style.ButtonIdle;
		const FLinearColor Frame = bActive ? Style.GetBrand() : Style.ButtonBorder;
		const FSlateRoundedBoxBrush Brush(Fill, 0.0f, Frame, 1.0f);
		FButtonStyle Result;
		Result.SetNormal(Brush);
		Result.SetHovered(Brush);
		Result.SetPressed(Brush);
		Result.SetNormalPadding(FMargin(24.0f, 12.0f));
		Result.SetPressedPadding(FMargin(24.0f, 13.0f, 24.0f, 11.0f));
		return Result;
	}

	/** True when the button already shows the focused style, read from its frame colour so the style is only set on a change */
	bool ShowsActive(const UButton* Button)
	{
		return Button->GetStyle().Normal.OutlineSettings.Color.GetSpecifiedColor().Equals(UBrandStyle::Get().GetBrand());
	}
}

UButton* FCyberMenuStyle::MakePlainButton(UWidgetTree* Tree, const FName& Name)
{
	UButton* Button = Tree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
	Button->SetStyle(CyberMenuButtons::Make(false));
	Button->SetBackgroundColor(FLinearColor::White);
	return Button;
}

UButton* FCyberMenuStyle::MakeButton(UWidgetTree* Tree, const FName& Name, const FText& Label, int32 Size)
{
	UButton* Button = MakePlainButton(Tree, Name);
	const FName LabelName = Name.IsNone() ? FName(NAME_None) : FName(*(Name.ToString() + TEXT("Label")));
	UTextBlock* LabelText = MakeText(Tree, LabelName, Label, EBrandText::Button, Size, TextColor());
	if (UButtonSlot* LabelSlot = Cast<UButtonSlot>(Button->AddChild(LabelText)))
	{
		LabelSlot->SetHorizontalAlignment(HAlign_Center);
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
	const bool bActive = Button->HasKeyboardFocus();
	if (bActive == CyberMenuButtons::ShowsActive(Button))
	{
		return;
	}

	Button->SetStyle(CyberMenuButtons::Make(bActive));
	Button->SetBackgroundColor(FLinearColor::White);

	// the label brightens to the near white with the focus
	if (UTextBlock* Label = Cast<UTextBlock>(Button->GetChildAt(0)))
	{
		Label->SetColorAndOpacity(FSlateColor(bActive ? NearWhiteColor() : TextColor()));
	}
}
