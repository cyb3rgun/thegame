// CYB3RGUN THEGAME. One level on the level selection screen: preview, kind, name and description.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LevelCardWidget.generated.h"

class UButton;
class UPlayableLevelDefinition;

/** A card button for one playable level, pressing it starts the level */
UCLASS()
class CYB3RGUN_API ULevelCardWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	/** Builds the card, call once right after creating it */
	void Setup(UPlayableLevelDefinition* InLevel);

	UButton* GetButton() const { return Button; }

protected:

	UPROPERTY(Transient)
	TObjectPtr<UPlayableLevelDefinition> Level;

	UPROPERTY(Transient)
	TObjectPtr<UButton> Button;

	UPROPERTY(EditAnywhere, Category="Card", meta = (ClampMin = 100.0))
	float CardWidth = 400.0f;

	UFUNCTION()
	void HandleClicked();
};
