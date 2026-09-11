// CYB3RGUN THEGAME. The brand in one place: the colour collection and the palette the UI builds around it.

#include "BrandStyle.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialParameterCollection.h"
#include "UObject/ConstructorHelpers.h"

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
