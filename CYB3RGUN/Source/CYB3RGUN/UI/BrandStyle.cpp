// CYB3RGUN THEGAME. The brand in one place: the colour collection, the palette and the typography the UI builds around it.

#include "BrandStyle.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialParameterCollection.h"
#include "UObject/ConstructorHelpers.h"

namespace BrandStyleDefaults
{
	/** A website colour: sRGB hex with the CSS alpha */
	FLinearColor Web(const TCHAR* Hex, float Alpha = 1.0f)
	{
		return FLinearColor(FColor::FromHex(Hex)).CopyWithNewOpacity(Alpha);
	}

	FBrandTextStyle Role(UObject* Font, const TCHAR* Typeface, int32 LetterSpacing, bool bUppercase)
	{
		FBrandTextStyle Style;
		Style.Font = Font;
		Style.Typeface = Typeface;
		Style.LetterSpacing = LetterSpacing;
		Style.bUppercase = bUppercase;
		return Style;
	}
}

UBrandStyle::UBrandStyle()
{
	static ConstructorHelpers::FObjectFinder<UMaterialParameterCollection> BrandColors(TEXT("/Game/CYB3RGUN/UI/Brand/MPC_Brand.MPC_Brand"));
	Colors = BrandColors.Object;

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ProjectedText(TEXT("/Game/CYB3RGUN/UI/Materials/M_UI_HoloText.M_UI_HoloText"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> MarkArc(TEXT("/Game/CYB3RGUN/UI/Brand/T_CYB3RGUN_Arc_White.T_CYB3RGUN_Arc_White"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> MarkRing(TEXT("/Game/CYB3RGUN/UI/Brand/T_CYB3RGUN_Ring_White.T_CYB3RGUN_Ring_White"));
	HoloTextMaterial = ProjectedText.Object;
	LogoArc = MarkArc.Object;
	LogoRing = MarkRing.Object;

	// the website's tokens (D-081): ink, dim, faint, the steel of plates and the deep black ground
	using BrandStyleDefaults::Web;
	Text = Web(TEXT("EAF7FC"));
	DimText = Web(TEXT("8AA5B2"));
	FaintText = Web(TEXT("6D8B99"));
	Ground = Web(TEXT("020508"));
	Panel = Web(TEXT("06121A"), 0.62f);
	PanelSolid = Web(TEXT("04090E"), 0.94f);
	Veil = Web(TEXT("020508"), 0.72f);
	ButtonIdle = Web(TEXT("06141C"), 0.5f);
	Track = Web(TEXT("0C1C25"), 0.75f);
	Shadow = Web(TEXT("000000"), 0.85f);
	Placeholder = Web(TEXT("0C1C25"));
	PlateBorder = Web(TEXT("EAF7FC"), 0.14f);
	ButtonBorder = Web(TEXT("EAF7FC"), 0.2f);

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> WordmarkFill(TEXT("/Game/CYB3RGUN/UI/Materials/M_UI_Wordmark.M_UI_Wordmark"));
	WordmarkMaterial = WordmarkFill.Object;
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> RainGlass(TEXT("/Game/CYB3RGUN/UI/Materials/M_UI_MenuRainGlass.M_UI_MenuRainGlass"));
	MenuBackgroundMaterial = RainGlass.Object;

	// the faces the website ships: Michroma for the wordmark and headings, Saira Condensed and Share Tech Mono for labels and
	// values, Source Sans 3 for running text (D-081, corrected to the site)
	static ConstructorHelpers::FObjectFinder<UFont> Michroma(TEXT("/Game/CYB3RGUN/UI/Fonts/F_Michroma.F_Michroma"));
	static ConstructorHelpers::FObjectFinder<UFont> Saira(TEXT("/Game/CYB3RGUN/UI/Fonts/F_SairaCondensed.F_SairaCondensed"));
	static ConstructorHelpers::FObjectFinder<UFont> Mono(TEXT("/Game/CYB3RGUN/UI/Fonts/F_ShareTechMono.F_ShareTechMono"));
	static ConstructorHelpers::FObjectFinder<UFont> Sans(TEXT("/Game/CYB3RGUN/UI/Fonts/F_SourceSans3.F_SourceSans3"));

	using BrandStyleDefaults::Role;
	WordmarkText = Role(Michroma.Object, TEXT("Regular"), 70, true);
	HeadingText = Role(Michroma.Object, TEXT("Regular"), 100, true);
	TitleText = Role(Saira.Object, TEXT("Bold"), 60, true);
	ButtonText = Role(Saira.Object, TEXT("Bold"), 180, true);
	LabelText = Role(Mono.Object, TEXT("Regular"), 300, true);
	BodyText = Role(Sans.Object, TEXT("Regular"), 0, false);
	BodyStrongText = Role(Sans.Object, TEXT("Semibold"), 0, false);
	ValueText = Role(Saira.Object, TEXT("Bold"), 0, false);
	ReadoutText = Role(Mono.Object, TEXT("Regular"), 140, true);
}

const UBrandStyle& UBrandStyle::Get()
{
	UBrandSettings* Settings = GetMutableDefault<UBrandSettings>();
	if (!Settings->LoadedStyle || Settings->LoadedStyle->HasAnyFlags(RF_ClassDefaultObject))
	{
		UBrandStyle* Named = Settings->bLoadTried ? Settings->Style.Get() : Settings->Style.LoadSynchronous();
		Settings->bLoadTried = true;
		Settings->LoadedStyle = Named ? Named : GetMutableDefault<UBrandStyle>();
	}
	return *Settings->LoadedStyle;
}

const FBrandTextStyle& UBrandStyle::GetTextStyle(EBrandText Role) const
{
	switch (Role)
	{
	case EBrandText::Wordmark: return WordmarkText;
	case EBrandText::Heading: return HeadingText;
	case EBrandText::Title: return TitleText;
	case EBrandText::Button: return ButtonText;
	case EBrandText::Label: return LabelText;
	case EBrandText::BodyStrong: return BodyStrongText;
	case EBrandText::Value: return ValueText;
	case EBrandText::Readout: return ReadoutText;
	case EBrandText::Body:
	default: return BodyText;
	}
}

FLinearColor UBrandStyle::ReadColor(FName Parameter) const
{
	if (Colors)
	{
		if (const FCollectionVectorParameter* Found = Colors->GetVectorParameterByName(Parameter))
		{
			return Found->DefaultValue;
		}
	}
	// without the collection the UI stays readable in plain white, never in a guessed brand colour
	return FLinearColor::White;
}

UBrandSettings::UBrandSettings()
{
	Style = TSoftObjectPtr<UBrandStyle>(FSoftObjectPath(TEXT("/Game/CYB3RGUN/UI/Brand/DA_BrandStyle.DA_BrandStyle")));
}
