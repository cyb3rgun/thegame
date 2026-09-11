// CYB3RGUN THEGAME. The level selection: one card per playable level definition.

#pragma once

#include "CoreMinimal.h"
#include "CyberMenuScreen.h"
#include "LevelSelectWidget.generated.h"

class ULevelCardWidget;

/**
 *  Lists every UPlayableLevelDefinition as a card, scenarios first, then test maps (D-037). A card starts its
 *  level, Back or Escape returns to the main menu. Built in code, WBP_LevelSelect derives from it.
 */
UCLASS()
class CYB3RGUN_API ULevelSelectWidget : public UCyberMenuScreen
{
	GENERATED_BODY()

protected:

	UPROPERTY(Transient)
	TArray<TObjectPtr<ULevelCardWidget>> Cards;

	UPROPERTY(EditAnywhere, Category="Menu", meta = (ClampMin = 1))
	int32 CardsPerRow = 3;

	virtual void BuildLayout() override;

	UFUNCTION()
	void HandleBack();
};
