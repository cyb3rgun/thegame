// CYB3RGUN THEGAME. The brand in one place: the colour collection and the palette the UI builds around it.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/DeveloperSettings.h"
#include "BrandStyle.generated.h"

class UMaterialInterface;
class UMaterialParameterCollection;
class UTexture2D;

/**
 *  The UI style of the brand (D-047). The logo cyan, the magenta counter colour and the danger red are defined once, as the
 *  defaults of the material parameter collection MPC_Brand. The HUD materials read that collection directly and this asset
 *  reads the same values for Slate and UMG, so a rebrand is one edit of the collection. Around the brand colours it holds
 *  the neutral palette: text, panels, bars and shadows. No brand colour is typed anywhere in code.
 */
UCLASS(BlueprintType)
class CYB3RGUN_API UBrandStyle : public UDataAsset
{
	GENERATED_BODY()

public:

	UBrandStyle();

	/** The style every widget uses: the asset named under Project Settings, Game, Brand, or these class defaults without it */
	static const UBrandStyle& Get();

	/** Where the brand colours live, their only definition */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Brand")
	TObjectPtr<UMaterialParameterCollection> Colors;

	/** The logo cyan: reticle, gauges, focus, good news */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Brand")
	FName BrandParameter = TEXT("BrandCyan");

	/** The magenta counter colour: Overclock, warnings, the second voice beside the brand */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Brand")
	FName CounterParameter = TEXT("BrandMagenta");

	/** Red, reserved for danger and penalties */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Brand")
	FName DangerParameter = TEXT("Danger");

	/** Body text */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Neutral")
	FLinearColor Text = FLinearColor(0.93f, 0.94f, 0.96f, 1.0f);

	/** Hints, disabled states, the rank at an empty meter */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Neutral")
	FLinearColor DimText = FLinearColor(0.56f, 0.6f, 0.66f, 1.0f);

	/** Menu bands and summary panels */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Neutral")
	FLinearColor Panel = FLinearColor(0.008f, 0.01f, 0.016f, 0.8f);

	/** The settings panel, nearly opaque so its many rows stay readable over any scene */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Neutral")
	FLinearColor PanelSolid = FLinearColor(0.02f, 0.02f, 0.03f, 0.92f);

	/** Darkens the scene behind a menu */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Neutral")
	FLinearColor Veil = FLinearColor(0.0f, 0.0f, 0.0f, 0.6f);

	/** A button without the focus; the focused one takes the brand colour */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Neutral")
	FLinearColor ButtonIdle = FLinearColor(0.06f, 0.065f, 0.08f, 0.7f);

	/** Opacity of the brand colour behind the focused button */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Neutral", meta = (ClampMin = 0.0, ClampMax = 1.0))
	float ButtonActiveOpacity = 0.95f;

	/** The empty part of a HUD bar */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Neutral")
	FLinearColor Track = FLinearColor(0.02f, 0.025f, 0.03f, 0.75f);

	/** Text shadow on the HUD */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Neutral")
	FLinearColor Shadow = FLinearColor(0.0f, 0.0f, 0.0f, 0.85f);

	/** Stands in for a missing preview image */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Neutral")
	FLinearColor Placeholder = FLinearColor(0.02f, 0.022f, 0.03f, 1.0f);

	/** The projection material the HUD text is drawn through (D-051) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Assets")
	TObjectPtr<UMaterialInterface> HoloTextMaterial;

	/** The arc of the mark as an alpha mask, tinted in the brand colour (D-048) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Assets")
	TObjectPtr<UTexture2D> LogoArc;

	/** The ring of the mark as an alpha mask */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Assets")
	TObjectPtr<UTexture2D> LogoRing;

	FLinearColor GetBrand() const { return ReadColor(BrandParameter); }
	FLinearColor GetCounter() const { return ReadColor(CounterParameter); }
	FLinearColor GetDanger() const { return ReadColor(DangerParameter); }

private:

	/** A colour from the collection's defaults. Materials read the same values, so UI and HUD materials always agree. */
	FLinearColor ReadColor(FName Parameter) const;
};

/** Project wide pointer to the brand style, shown under Project Settings, Game, Brand */
UCLASS(Config=Game, DefaultConfig, meta = (DisplayName = "Brand"))
class CYB3RGUN_API UBrandSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:

	UBrandSettings();

	UPROPERTY(Config, EditAnywhere, Category="Brand")
	TSoftObjectPtr<UBrandStyle> Style;

	/** The style once loaded, held here on the settings object so it stays in memory */
	UPROPERTY(Transient)
	TObjectPtr<UBrandStyle> LoadedStyle;

	/** True once loading the named asset was tried, so a missing asset is not searched for again every frame */
	bool bLoadTried = false;

	virtual FName GetCategoryName() const override { return TEXT("Game"); }
};
