// CYB3RGUN THEGAME. The shared look of the front end: colours, fonts and the menu button.

#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"
#include "Layout/Margin.h"
#include "Types/SlateEnums.h"

class UButton;
class USizeBox;
class UTextBlock;
class UVerticalBox;
class UWidget;
class UWidgetTree;

/** What every menu screen shares, so the main menu, the level selection and the pause menu read as one front end */
struct CYB3RGUN_API FCyberMenuStyle
{
	static const FLinearColor AccentColor;
	static const FLinearColor TextColor;
	static const FLinearColor DimTextColor;
	static const FLinearColor PanelColor;
	static const FLinearColor VeilColor;
	static const FLinearColor ButtonIdleColor;
	static const FLinearColor ButtonActiveColor;

	static FSlateFontInfo MakeFont(int32 Size, const FName& Typeface = TEXT("Bold"), int32 LetterSpacing = 0);

	static UTextBlock* MakeText(UWidgetTree* Tree, const FName& Name, const FText& Text, int32 Size, const FLinearColor& Color, const FName& Typeface = TEXT("Bold"));

	/** Flat button without content, its brush takes the colour UpdateHighlight gives it */
	static UButton* MakePlainButton(UWidgetTree* Tree, const FName& Name);

	/** Flat button with a left aligned label */
	static UButton* MakeButton(UWidgetTree* Tree, const FName& Name, const FText& Label, int32 Size);

	static USizeBox* WrapWidth(UWidgetTree* Tree, UWidget* Content, float Width);

	static void AddToColumn(UVerticalBox* Column, UWidget* Widget, const FMargin& Padding, EHorizontalAlignment Alignment = HAlign_Left);

	/** Lights a button up while it holds the focus, the one cursor that mouse, keyboard and gamepad share */
	static void UpdateHighlight(UButton* Button);
};
