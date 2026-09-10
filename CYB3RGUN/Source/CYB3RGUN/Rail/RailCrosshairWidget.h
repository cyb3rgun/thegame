// CYB3RGUN THEGAME. Crosshair drawn wherever the aim component says the crosshair is.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RailCrosshairWidget.generated.h"

class URailAimComponent;
struct FHitResult;

/**
 *  Full screen, hit test invisible overlay. Paints a crosshair at the aim component's screen position,
 *  flashes on a damaging hit and dims while firing is blocked. Style values are editable in WBP_RailCrosshair.
 */
UCLASS()
class CYB3RGUN_API URailCrosshairWidget : public UUserWidget
{
	GENERATED_BODY()

protected:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Crosshair")
	FLinearColor Color = FLinearColor(1.0f, 1.0f, 1.0f, 0.95f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Crosshair")
	FLinearColor HitColor = FLinearColor(1.0f, 0.15f, 0.1f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Crosshair")
	FLinearColor BlockedColor = FLinearColor(0.6f, 0.6f, 0.6f, 0.35f);

	/** Length of each arm */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Crosshair", meta = (ClampMin = 1.0))
	float ArmLength = 14.0f;

	/** Empty space between the centre and each arm */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Crosshair", meta = (ClampMin = 0.0))
	float Gap = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Crosshair", meta = (ClampMin = 0.5))
	float Thickness = 2.5f;

	/** Seconds the hit flash lasts */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Crosshair", meta = (ClampMin = 0.05, Units = "s"))
	float HitFlashSeconds = 0.25f;

	UPROPERTY(Transient)
	TWeakObjectPtr<URailAimComponent> Aim;

	float HitFlash = 0.0f;

public:

	UFUNCTION(BlueprintCallable, Category="Crosshair")
	void BindAim(URailAimComponent* InAim);

protected:

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	UFUNCTION()
	void HandleShotFired(bool bHit, const FHitResult& Hit, AActor* DamagedActor);

	void UnbindAim();
};
