// CYB3RGUN THEGAME. The shared look of the front end and the HUDs: colours, text roles, framing and the menu button.

#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"
#include "Layout/Margin.h"
#include "Types/SlateEnums.h"
#include "BrandStyle.h"

class UBrandFrame;
class UBrandRule;
class UButton;
class USizeBox;
class UTextBlock;
class UVerticalBox;
class UWidget;
class UWidgetTree;

/** What every screen and HUD shares, so menus, HUDs and the website read as one product (D-081, D-082) */
struct CYB3RGUN_API FCyberMenuStyle
{
	/** The colours come from the brand style (UBrandStyle): brand, counter, danger and near white from MPC_Brand, the neutrals
	 *  from the style asset. Nothing here types a colour (D-047). */
	static FLinearColor BrandColor();
	static FLinearColor CounterColor();
	static FLinearColor DangerColor();
	static FLinearColor TextColor();
	static FLinearColor DimTextColor();
	static FLinearColor PanelColor();
	static FLinearColor PanelSolidColor();
	static FLinearColor VeilColor();
	static FLinearColor ButtonIdleColor();
	static FLinearColor ButtonActiveColor();
	static FLinearColor TrackColor();
	static FLinearColor ShadowColor();
	static FLinearColor PlaceholderColor();
	static FLinearColor NearWhiteColor();
	static FLinearColor FaintTextColor();
	static FLinearColor GroundColor();
	static FLinearColor HairlineColor();

	/** The font of a text role at a size: its face, typeface and tracking from the brand style. There is no other way to get a
	 *  font, so no engine default font reaches player facing UI. */
	static FSlateFontInfo MakeFont(EBrandText Role, int32 Size);

	/** A text block set in a role: font, tracking and capitals as the website sets that role */
	static UTextBlock* MakeText(UWidgetTree* Tree, const FName& Name, const FText& Text, EBrandText Role, int32 Size, const FLinearColor& Color);

	/** Sets an existing text block, for example one laid out in a Blueprint, in a role */
	static void ApplyRole(UTextBlock* Block, EBrandText Role, int32 Size);

	/** A small capital label over a large value, the website's readout. Returns the column; the value block comes back in OutValue. */
	static UVerticalBox* MakeReadout(UWidgetTree* Tree, const FName& Name, const FText& Label, int32 ValueSize, const FLinearColor& ValueColor, UTextBlock*& OutValue,
		EHorizontalAlignment Alignment = HAlign_Left);

	/** A hard edged plate with a one pixel frame and, unless turned off, corner brackets */
	static UBrandFrame* MakeFrame(UWidgetTree* Tree, const FName& Name, const FMargin& Padding, bool bCornerTicks = true);

	/** A thin rule fading in towards its bright end */
	static UBrandRule* MakeRule(UWidgetTree* Tree, float Length, bool bFadeFromStart = true);

	/** Hard edged button without content, framed; UpdateHighlight frames it in the brand colour while it holds the focus */
	static UButton* MakePlainButton(UWidgetTree* Tree, const FName& Name);

	/** Hard edged button with a centred label in the button role */
	static UButton* MakeButton(UWidgetTree* Tree, const FName& Name, const FText& Label, int32 Size);

	static USizeBox* WrapWidth(UWidgetTree* Tree, UWidget* Content, float Width);

	static void AddToColumn(UVerticalBox* Column, UWidget* Widget, const FMargin& Padding, EHorizontalAlignment Alignment = HAlign_Left);

	/** Lights a button up while it holds the focus, the one cursor that mouse, keyboard and gamepad share */
	static void UpdateHighlight(UButton* Button);
};
