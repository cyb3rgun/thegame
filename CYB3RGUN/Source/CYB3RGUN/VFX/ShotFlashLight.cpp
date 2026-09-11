// CYB3RGUN THEGAME. A point light that flashes up and fades out, for muzzle flashes and hits.

#include "ShotFlashLight.h"
#include "Components/PointLightComponent.h"

AShotFlashLight::AShotFlashLight()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("Light"));
	RootComponent = Light;
	Light->Mobility = EComponentMobility::Movable;
	Light->IntensityUnits = ELightUnits::Lumens;
	Light->Intensity = 0.0f;
	Light->CastShadows = false;
	Light->SourceRadius = 4.0f;

	SetCanBeDamaged(false);
}

void AShotFlashLight::Flash(const FLinearColor& Color, float Lumens, float Radius, float InDuration)
{
	PeakLumens = Lumens;
	Duration = FMath::Max(InDuration, 0.01f);
	Elapsed = 0.0f;
	Light->SetLightColor(Color);
	Light->SetAttenuationRadius(Radius);
	Light->SetIntensity(PeakLumens);
}

void AShotFlashLight::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	Elapsed += DeltaSeconds;
	const float Remaining = 1.0f - FMath::Clamp(Elapsed / Duration, 0.0f, 1.0f);

	// squared fall off: bright for the first frame or two, then a quick tail
	Light->SetIntensity(PeakLumens * Remaining * Remaining);
	if (Remaining <= 0.0f)
	{
		Destroy();
	}
}
