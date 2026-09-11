// CYB3RGUN THEGAME. Which style values every scenario uses unless it names its own.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "StyleProjectSettings.generated.h"

class UStyleSettings;

/** Project wide pointer to the default style values, shown under Project Settings, Game, Style */
UCLASS(Config=Game, DefaultConfig, meta = (DisplayName = "Style"))
class CYB3RGUN_API UStyleProjectSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:

	UStyleProjectSettings();

	/** Style values of every scenario whose game mode does not name its own */
	UPROPERTY(Config, EditAnywhere, Category="Style")
	TSoftObjectPtr<UStyleSettings> DefaultStyleSettings;

	virtual FName GetCategoryName() const override { return TEXT("Game"); }
};
