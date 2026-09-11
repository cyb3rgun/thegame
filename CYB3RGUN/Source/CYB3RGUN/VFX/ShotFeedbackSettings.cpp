// CYB3RGUN THEGAME. Effects and lights for shots and hits, adjustable under Project Settings.

#include "ShotFeedbackSettings.h"

UShotFeedbackSettings::UShotFeedbackSettings()
{
	MuzzleFlashSystem = TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(TEXT("/Game/CYB3RGUN/VFX/NS_MuzzleFlash.NS_MuzzleFlash")));
	ImpactSystem = TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(TEXT("/Game/CYB3RGUN/VFX/NS_ImpactSparks.NS_ImpactSparks")));
	ImpactDecalMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/CYB3RGUN/VFX/M_ImpactDecal.M_ImpactDecal")));
}
