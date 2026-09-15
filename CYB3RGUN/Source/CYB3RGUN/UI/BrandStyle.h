// CYB3RGUN THEGAME. The brand in one place: the colour collection, the palette and the typography the UI builds around it.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/DeveloperSettings.h"
#include "BrandStyle.generated.h"

class UMaterialInterface;
class UMaterialParameterCollection;
class UTexture2D;

/** The roles text plays in player facing UI, each with one face, tracking and case (D-082) */
UENUM(BlueprintType)
enum class EBrandText : uint8
{
	/** CYB3RGUN itself: Michroma with the gradient fill */
	Wordmark,
	/** Screen and panel titles: Michroma in capitals */
	Heading,
	/** Large headlines: Saira Condensed bold in capitals */
	Title,
	/** Buttons: Saira Condensed bold, capitals, wide tracking */
	Button,
	/** Small capitals with very wide tracking: Share Tech Mono */
	Label,
	/** Running text: Source Sans 3 */
	Body,
	/** Names and emphasis in running text: Source Sans 3 semibold */
	BodyStrong,
	/** Large numerals for values: Saira Condensed bold */
	Value,
	/** Numeric and technical readouts: Share Tech Mono */
	Readout
};

/** How one text role is set */
USTRUCT(BlueprintType)
struct FBrandTextStyle
{
	GENERATED_BODY()

	/** A font asset (UFont); empty falls back to the engine font */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Text", meta = (AllowedClasses = "/Script/Engine.Font"))
	TObjectPtr<UObject> Font;

	/** Typeface inside the font, for example Medium or Bold */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Text")
	FName Typeface = TEXT("Regular");

	/** Tracking in thousandths of an em, the website's letter-spacing */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Text")
	int32 LetterSpacing = 0;

	/** Set in capitals whatever the text says */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Text")
	bool bUppercase = false;
};

/**
 *  The UI style of the brand (D-047, D-081). The brand cyan, the danger red, the near white and the magenta counter colour are
 *  defined once, as the defaults of the material parameter collection MPC_Brand. The HUD materials read that collection
 *  directly and this asset reads the same values for Slate and UMG, so a rebrand is one edit of the collection. Around the
 *  brand colours it holds the website's neutral palette, the framing colours and the text roles (D-082). No brand colour
 *  is typed anywhere in widget code.
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

	/** The brand cyan: reticle, gauges, focus, labels, good news */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Brand")
	FName BrandParameter = TEXT("BrandCyan");

	/** The magenta counter colour: Overclock, warnings, the second voice beside the brand */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Brand")
	FName CounterParameter = TEXT("BrandMagenta");

	/** Red, reserved for danger and penalties */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Brand")
	FName DangerParameter = TEXT("Danger");

	/** The near white of the brand: the bright band of the wordmark and focused text */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Brand")
	FName NearWhiteParameter = TEXT("NearWhite");

	/** Primary text, the website's ink */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Neutral")
	FLinearColor Text;

	/** Body text and hints, the website's dim */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Neutral")
	FLinearColor DimText;

	/** Micro labels, the website's faint */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Neutral")
	FLinearColor FaintText;

	/** The deep black ground behind every screen */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Neutral")
	FLinearColor Ground;

	/** Menu bands, plates and summary panels */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Neutral")
	FLinearColor Panel;

	/** The settings panel, nearly opaque so its many rows stay readable over any scene */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Neutral")
	FLinearColor PanelSolid;

	/** Darkens the scene behind a menu */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Neutral")
	FLinearColor Veil;

	/** Fill of a button without the focus */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Neutral")
	FLinearColor ButtonIdle;

	/** Opacity of the brand colour behind a focused element that fills instead of framing */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Neutral", meta = (ClampMin = 0.0, ClampMax = 1.0))
	float ButtonActiveOpacity = 0.95f;

	/** The empty part of a HUD bar */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Neutral")
	FLinearColor Track;

	/** Text shadow on the HUD */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Neutral")
	FLinearColor Shadow;

	/** Stands in for a missing preview image */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Neutral")
	FLinearColor Placeholder;

	/** Opacity of the brand colour in thin rules and cell dividers */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Framing", meta = (ClampMin = 0.0, ClampMax = 1.0))
	float HairlineOpacity = 0.22f;

	/** The one pixel frame of a plate */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Framing")
	FLinearColor PlateBorder;

	/** Opacity of the brand colour in the corner brackets of a plate, top left and bottom right */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Framing", meta = (ClampMin = 0.0, ClampMax = 1.0))
	float CornerTickOpacity = 0.45f;

	/** Length of a corner bracket arm */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Framing", meta = (ClampMin = 2.0))
	float CornerTickSize = 12.0f;

	/** The frame of a button without the focus; the focused button is framed in the brand colour */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Framing")
	FLinearColor ButtonBorder;

	/** Fill of the focused button: the brand colour at this opacity */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Framing", meta = (ClampMin = 0.0, ClampMax = 1.0))
	float ButtonActiveFill = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Typography")
	FBrandTextStyle WordmarkText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Typography")
	FBrandTextStyle HeadingText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Typography")
	FBrandTextStyle TitleText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Typography")
	FBrandTextStyle ButtonText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Typography")
	FBrandTextStyle LabelText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Typography")
	FBrandTextStyle BodyText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Typography")
	FBrandTextStyle BodyStrongText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Typography")
	FBrandTextStyle ValueText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Typography")
	FBrandTextStyle ReadoutText;

	/** The projection material the HUD text is drawn through (D-051) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Assets")
	TObjectPtr<UMaterialInterface> HoloTextMaterial;

	/** The wordmark's fill: a font material that sweeps the near white through the cyan, as on the website */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Assets")
	TObjectPtr<UMaterialInterface> WordmarkMaterial;

	/** The front end's background: rain on glass in front of the night range, a user interface material */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Assets")
	TObjectPtr<UMaterialInterface> MenuBackgroundMaterial;

	/** The arc of the mark as an alpha mask, tinted in the brand colour (D-048) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Assets")
	TObjectPtr<UTexture2D> LogoArc;

	/** The ring of the mark as an alpha mask */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Assets")
	TObjectPtr<UTexture2D> LogoRing;

	FLinearColor GetBrand() const { return ReadColor(BrandParameter); }
	FLinearColor GetCounter() const { return ReadColor(CounterParameter); }
	FLinearColor GetDanger() const { return ReadColor(DangerParameter); }
	FLinearColor GetNearWhite() const { return ReadColor(NearWhiteParameter); }
	FLinearColor GetHairline() const { return GetBrand().CopyWithNewOpacity(HairlineOpacity); }
	FLinearColor GetCornerTick() const { return GetBrand().CopyWithNewOpacity(CornerTickOpacity); }

	/** How a text role is set */
	const FBrandTextStyle& GetTextStyle(EBrandText Role) const;

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
