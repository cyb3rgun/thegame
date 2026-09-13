// CYB3RGUN THEGAME. Everything a weapon is, as data: name, body, mounts, fire mode, ballistics, feed, handling, sound (D-052).

#include "WeaponDefinition.h"

FWeaponMounts::FWeaponMounts()
{
	// the socket names a weapon model carries (D-054)
	Grip.Socket = FName(TEXT("Grip"));
	Muzzle.Socket = FName(TEXT("Muzzle"));
	Magazine.Socket = FName(TEXT("Magazine"));
	Optic.Socket = FName(TEXT("Optic"));
	PressureGauge.Socket = FName(TEXT("PressureGauge"));
}

const FWeaponMountPoint& FWeaponMounts::Get(EWeaponMount Mount) const
{
	switch (Mount)
	{
	case EWeaponMount::Muzzle:
		return Muzzle;
	case EWeaponMount::Magazine:
		return Magazine;
	case EWeaponMount::Optic:
		return Optic;
	case EWeaponMount::PressureGauge:
		return PressureGauge;
	case EWeaponMount::Grip:
	default:
		return Grip;
	}
}

float UWeaponDefinition::GetDamageScale(float Distance) const
{
	if (Distance <= FalloffStart || FalloffEnd <= FalloffStart)
	{
		return 1.0f;
	}
	const float Alpha = FMath::Clamp((Distance - FalloffStart) / (FalloffEnd - FalloffStart), 0.0f, 1.0f);
	return FMath::Lerp(1.0f, FalloffMinScale, Alpha);
}

float UWeaponDefinition::GetMuzzleSpeed(float Energy) const
{
	if (Energy <= 0.0f || ProjectileMassGrams <= 0.0f)
	{
		return 0.0f;
	}

	// kinetic energy E = m v^2 / 2, with the mass in kilograms, and the speed from metres to centimetres per second
	const float MassKilograms = ProjectileMassGrams / 1000.0f;
	return FMath::Sqrt(2.0f * Energy / MassKilograms) * 100.0f;
}
