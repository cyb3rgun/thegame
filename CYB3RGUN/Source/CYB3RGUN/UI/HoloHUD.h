// CYB3RGUN THEGAME. The HUD is projected by the weapon: fine scanlines, a faint flicker and a glitch when the player is hit (D-051).

#pragma once

#include "CoreMinimal.h"

class UMaterialInstanceDynamic;
class UObject;
class UUserWidget;
class UWidgetTree;

/**
 *  The holographic treatment of the HUD (D-051). The weapon projects the HUD, so its text is drawn through the projection
 *  material M_UI_HoloText: fine scanlines and a faint flicker, legibility first. When the player takes damage the
 *  projection glitches for DamageGlitchSeconds: bands of the text drop out and the whole HUD jumps a few pixels in steps.
 *  The logo crosshair carries the same treatment in its own material, with the chromatic offset toward its edges.
 */
struct CYB3RGUN_API FHoloHUD
{
	/** One instance of the projection material for a HUD widget, null when the brand style names none */
	static UMaterialInstanceDynamic* CreateTextMaterial(UObject* Outer);

	/** Draws every text block of the tree through the projection material */
	static void ApplyToText(UWidgetTree* Tree, UMaterialInstanceDynamic* Material);

	/** Feeds the damage glitch into the material and makes the widget jump while it lasts */
	static void Update(UUserWidget* Widget, UMaterialInstanceDynamic* Material);
};
