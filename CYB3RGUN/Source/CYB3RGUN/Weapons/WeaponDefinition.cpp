// CYB3RGUN THEGAME. How a weapon handles, as data.

#include "WeaponDefinition.h"

float UWeaponDefinition::GetDamageScale(float Distance) const
{
	if (Distance <= FalloffStart || FalloffEnd <= FalloffStart)
	{
		return 1.0f;
	}
	const float Alpha = FMath::Clamp((Distance - FalloffStart) / (FalloffEnd - FalloffStart), 0.0f, 1.0f);
	return FMath::Lerp(1.0f, FalloffMinScale, Alpha);
}
