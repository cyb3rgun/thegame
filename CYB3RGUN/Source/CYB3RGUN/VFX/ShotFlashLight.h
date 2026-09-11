// CYB3RGUN THEGAME. A point light that flashes up and fades out, for muzzle flashes and hits.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShotFlashLight.generated.h"

class UPointLightComponent;

/**
 *  Starts at full brightness and falls to dark over its duration, then removes itself. Movable and without
 *  shadows, so a burst of shots costs a handful of short lived unshadowed lights.
 */
UCLASS(NotPlaceable, Transient)
class CYB3RGUN_API AShotFlashLight : public AActor
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UPointLightComponent* Light;

public:

	AShotFlashLight();

	/** Sets the flash up, call right after spawning */
	void Flash(const FLinearColor& Color, float Lumens, float Radius, float InDuration);

	virtual void Tick(float DeltaSeconds) override;

protected:

	float PeakLumens = 0.0f;
	float Duration = 0.1f;
	float Elapsed = 0.0f;
};
