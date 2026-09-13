// CYB3RGUN THEGAME. Which hit zone values the game uses.

#include "HitZoneProjectSettings.h"
#include "HitZoneSettings.h"

UHitZoneProjectSettings::UHitZoneProjectSettings()
{
	DefaultHitZoneSettings = TSoftObjectPtr<UHitZoneSettings>(FSoftObjectPath(TEXT("/Game/CYB3RGUN/Core/Combat/DA_HitZones.DA_HitZones")));
}
