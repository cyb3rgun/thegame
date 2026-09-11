// CYB3RGUN THEGAME. The HUD is projected by the weapon: fine scanlines, a faint flicker and a glitch when the player is hit (D-051).

#include "HoloHUD.h"
#include "BrandStyle.h"
#include "CombatFeelSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "HAL/PlatformTime.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

// a named namespace with unique names, unity builds merge this file with the other HUDs
namespace HoloHudLook
{
	/** How far the HUD jumps at the peak of a glitch, in slate units */
	constexpr float JumpX = 6.0f;
	constexpr float JumpY = 2.0f;

	/** Jumps per second: the glitch steps, it never slides */
	constexpr double JumpRate = 30.0;
}

UMaterialInstanceDynamic* FHoloHUD::CreateTextMaterial(UObject* Outer)
{
	UMaterialInterface* Base = UBrandStyle::Get().HoloTextMaterial;
	return Base ? UMaterialInstanceDynamic::Create(Base, Outer) : nullptr;
}

void FHoloHUD::ApplyToText(UWidgetTree* Tree, UMaterialInstanceDynamic* Material)
{
	if (!Tree || !Material)
	{
		return;
	}
	Tree->ForEachWidget([Material](UWidget* Widget)
	{
		if (UTextBlock* Text = Cast<UTextBlock>(Widget))
		{
			FSlateFontInfo Font = Text->GetFont();
			Font.FontMaterial = Material;
			Text->SetFont(Font);
		}
	});
}

void FHoloHUD::Update(UUserWidget* Widget, UMaterialInstanceDynamic* Material)
{
	if (!Widget)
	{
		return;
	}

	const UCombatFeelSubsystem* Feel = UCombatFeelSubsystem::Get(Widget);
	const float Glitch = Feel ? Feel->GetDamageGlitch() : 0.0f;
	if (Material)
	{
		Material->SetScalarParameterValue(TEXT("Glitch"), Glitch);
	}

	// the projection jumps in steps while it glitches and sits still otherwise
	FVector2D Jump = FVector2D::ZeroVector;
	if (Glitch > 0.0f)
	{
		const uint32 Hash = static_cast<uint32>(FPlatformTime::Seconds() * HoloHudLook::JumpRate) * 2654435761u;
		const float NoiseX = static_cast<float>((Hash >> 8) & 0xFFFF) / 65535.0f * 2.0f - 1.0f;
		const float NoiseY = static_cast<float>((Hash >> 24) & 0xFF) / 255.0f * 2.0f - 1.0f;
		Jump = FVector2D(NoiseX * HoloHudLook::JumpX, NoiseY * HoloHudLook::JumpY) * Glitch;
	}
	if (!Widget->GetRenderTransform().Translation.Equals(Jump))
	{
		Widget->SetRenderTranslation(Jump);
	}
}
