// CYB3RGUN THEGAME. The front end's background: rain running down glass in front of the night range, a faint cyan glow behind it.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MenuBackgroundWidget.generated.h"

class APlayerController;
class UImage;
class UMaterialInstanceDynamic;

/**
 *  A full screen image under every menu screen, drawn by the brand style's menu background material: condensation, beads and
 *  drops that slowly run down with their trails, the scene behind showing through the clear paths and a faint cyan glow from
 *  behind. It is a material, not a video, so it costs no disk space and scales with the resolution. Calm by design: the
 *  menu text stays readable.
 */
UCLASS()
class CYB3RGUN_API UMenuBackgroundWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	/** Creates the background for a local player and puts it under the menu screens */
	static UMenuBackgroundWidget* CreateFor(APlayerController* Player);

protected:

	UPROPERTY(Transient)
	TObjectPtr<UImage> Glass;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> GlassMaterial;

	float ShownAspect = 0.0f;

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
};
