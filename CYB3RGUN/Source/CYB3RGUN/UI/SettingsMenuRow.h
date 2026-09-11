// CYB3RGUN THEGAME. One option row of the settings menu: label, previous, value, next, measured cost.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CyberSettingsTypes.h"
#include "SettingsMenuRow.generated.h"

class UButton;
class UTextBlock;
class USettingsMenuWidget;

/** Built in code, like the HUDs. Steps its option through the owning menu so the menu keeps the pending state. */
UCLASS()
class CYB3RGUN_API USettingsMenuRow : public UUserWidget
{
	GENERATED_BODY()

protected:

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> LabelText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ValueText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> CostText;

	UPROPERTY(Transient)
	TObjectPtr<UButton> PrevButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> NextButton;

	TWeakObjectPtr<USettingsMenuWidget> Menu;
	ECyberSettingOption Option = ECyberSettingOption::Preset;

public:

	void Setup(USettingsMenuWidget* InMenu, ECyberSettingOption InOption);
	/** Cost is the measured cost of the shown value, empty when there is none */
	void Refresh(const FText& Label, const FText& Value, bool bExperimental, const FText& Cost);

	ECyberSettingOption GetOption() const { return Option; }
	UButton* GetNextButton() const { return NextButton; }

protected:

	virtual TSharedRef<SWidget> RebuildWidget() override;

	UFUNCTION()
	void HandlePrev();

	UFUNCTION()
	void HandleNext();
};
