// CYB3RGUN THEGAME. The brand in one place: the colour collection and the palette the UI builds around it.

#include "BrandStyle.h"
#include "Materials/MaterialParameterCollection.h"
#include "UObject/ConstructorHelpers.h"

UBrandStyle::UBrandStyle()
{
	static ConstructorHelpers::FObjectFinder<UMaterialParameterCollection> BrandColors(TEXT("/Game/CYB3RGUN/UI/Brand/MPC_Brand.MPC_Brand"));
	Colors = BrandColors.Object;
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
