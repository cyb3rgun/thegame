// CYB3RGUN THEGAME. Camera kicks and the kill and Overclock screen look, on the wall clock.

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraModifier.h"
#include "CombatFeelCameraModifier.generated.h"

/**
 *  Added to the local player's camera by the combat feel subsystem. Kicks are small turns of the view that
 *  fall back to rest with a few swings; the screen look darkens the edges, fringes the colours and takes
 *  saturation away, briefly on a kill and softly for as long as Overclock runs.
 */
UCLASS()
class CYB3RGUN_API UCombatFeelCameraModifier : public UCameraModifier
{
	GENERATED_BODY()

public:

	/** Turns the view along Direction, right and up in screen terms, by up to Degrees */
	void AddKick(const FVector2D& Direction, float Degrees, float DecaySeconds, float Frequency);

	/** The kill screen look, fading out over Seconds */
	void Flash(float Seconds, float Strength);

	/** The Overclock screen look, 0 off to 1 full */
	void SetOverclockLook(float Strength) { OverclockLook = Strength; }

	virtual bool ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV) override;
	virtual void ModifyPostProcess(float DeltaTime, float& PostProcessBlendWeight, FPostProcessSettings& PostProcessSettings) override;

protected:

	struct FKick
	{
		FVector2D Direction = FVector2D::ZeroVector;
		float Degrees = 0.0f;
		float DecaySeconds = 0.1f;
		float Frequency = 0.0f;
		double StartSeconds = 0.0;
	};

	TArray<FKick> Kicks;

	double FlashStartSeconds = -1000.0;
	float FlashSeconds = 0.0f;
	float FlashStrength = 0.0f;
	float OverclockLook = 0.0f;
};
