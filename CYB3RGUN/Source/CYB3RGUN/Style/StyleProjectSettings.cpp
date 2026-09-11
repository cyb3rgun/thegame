// CYB3RGUN THEGAME. Which style values every scenario uses unless it names its own.

#include "StyleProjectSettings.h"
#include "StyleSettings.h"

UStyleProjectSettings::UStyleProjectSettings()
{
	DefaultStyleSettings = TSoftObjectPtr<UStyleSettings>(FSoftObjectPath(TEXT("/Game/CYB3RGUN/Core/Style/DA_Style_Default.DA_Style_Default")));
}
