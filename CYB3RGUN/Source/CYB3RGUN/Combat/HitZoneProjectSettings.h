// CYB3RGUN THEGAME. Which hit zone values the game uses.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "HitZoneProjectSettings.generated.h"

class UHitZoneSettings;

/** Project wide pointer to the hit zone values, shown under Project Settings, Game, Hit Zones */
UCLASS(Config=Game, DefaultConfig, meta = (DisplayName = "Hit Zones"))
class CYB3RGUN_API UHitZoneProjectSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:

	UHitZoneProjectSettings();

	/** Hit zones of every target: bones, damage multipliers, reactions, scores and reaction animations */
	UPROPERTY(Config, EditAnywhere, Category="Hit Zones")
	TSoftObjectPtr<UHitZoneSettings> DefaultHitZoneSettings;

	virtual FName GetCategoryName() const override { return TEXT("Game"); }
};
