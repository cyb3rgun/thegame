// CYB3RGUN THEGAME. Camera kicks and the kill and Overclock screen look, on the wall clock.

#include "CombatFeelCameraModifier.h"
#include "Camera/CameraTypes.h"
#include "Engine/Scene.h"
#include "HAL/PlatformTime.h"

void UCombatFeelCameraModifier::AddKick(const FVector2D& Direction, float Degrees, float DecaySeconds, float Frequency)
{
	FKick& Kick = Kicks.AddDefaulted_GetRef();
	Kick.Direction = Direction;
	Kick.Degrees = Degrees;
	Kick.DecaySeconds = FMath::Max(DecaySeconds, 0.02f);
	Kick.Frequency = Frequency;
	Kick.StartSeconds = FPlatformTime::Seconds();
}

void UCombatFeelCameraModifier::Flash(float Seconds, float Strength)
{
	FlashStartSeconds = FPlatformTime::Seconds();
	FlashSeconds = Seconds;
	FlashStrength = Strength;
}

bool UCombatFeelCameraModifier::ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV)
{
	Super::ModifyCamera(DeltaTime, InOutPOV);

	// kicks run on the wall clock, a hit stop or a slowed world does not hold them
	const double Now = FPlatformTime::Seconds();
	FRotator Offset = FRotator::ZeroRotator;
	for (int32 Index = Kicks.Num() - 1; Index >= 0; --Index)
	{
		const FKick& Kick = Kicks[Index];
		const float Age = static_cast<float>(Now - Kick.StartSeconds);
		if (Age >= Kick.DecaySeconds)
		{
			Kicks.RemoveAtSwap(Index);
			continue;
		}

		// full strength at once, then a squared fall to rest with a few swings on the way
		const float Envelope = FMath::Square(1.0f - Age / Kick.DecaySeconds);
		const float Amount = Kick.Degrees * Envelope * FMath::Cos(2.0f * PI * Kick.Frequency * Age);
		Offset.Yaw += Kick.Direction.X * Amount;
		Offset.Pitch += Kick.Direction.Y * Amount;
	}
	InOutPOV.Rotation += Offset;
	return false;
}

void UCombatFeelCameraModifier::ModifyPostProcess(float DeltaTime, float& PostProcessBlendWeight, FPostProcessSettings& PostProcessSettings)
{
	const float FlashAge = static_cast<float>(FPlatformTime::Seconds() - FlashStartSeconds);
	const float Flash = (FlashSeconds > 0.0f && FlashAge < FlashSeconds) ? FlashStrength * (1.0f - FlashAge / FlashSeconds) : 0.0f;
	const float Strength = FMath::Max(Flash, OverclockLook);
	if (Strength <= 0.0f)
	{
		PostProcessBlendWeight = 0.0f;
		return;
	}

	// dark edges, a colour fringe and less colour: a blink on a kill, held softer while Overclock runs
	PostProcessBlendWeight = 1.0f;
	PostProcessSettings.bOverride_VignetteIntensity = true;
	PostProcessSettings.VignetteIntensity = 0.4f + 0.9f * Strength;
	PostProcessSettings.bOverride_SceneFringeIntensity = true;
	PostProcessSettings.SceneFringeIntensity = 4.0f * Flash + 1.5f * OverclockLook;
	PostProcessSettings.bOverride_ColorSaturation = true;
	const float Saturation = 1.0f - 0.6f * Strength;
	PostProcessSettings.ColorSaturation = FVector4(Saturation, Saturation, Saturation, 1.0f);
}
