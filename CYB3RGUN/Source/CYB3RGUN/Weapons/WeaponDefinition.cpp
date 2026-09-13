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

float UWeaponDefinition::GetEnergyShare(float PressureBar) const
{
	return FMath::Max(EvaluateCurve(EnergyCurve, PressureBar, 1.0f), 0.0f);
}

float UWeaponDefinition::GetDropMultiplier(float PressureBar) const
{
	return FMath::Max(EvaluateCurve(DropCurve, PressureBar, 1.0f), 0.0f);
}

float UWeaponDefinition::GetPressureCone(float PressureBar) const
{
	return FMath::Max(EvaluateCurve(ConeCurve, PressureBar, 0.0f), 0.0f);
}

float UWeaponDefinition::EvaluateCurve(const TArray<FVector2D>& Points, float X, float Fallback)
{
	if (Points.IsEmpty())
	{
		return Fallback;
	}

	// the points may be entered in any order, the curve runs along X
	TArray<FVector2D> Sorted = Points;
	Sorted.Sort([](const FVector2D& A, const FVector2D& B) { return A.X < B.X; });
	if (X <= Sorted[0].X)
	{
		return Sorted[0].Y;
	}
	for (int32 Index = 1; Index < Sorted.Num(); ++Index)
	{
		const FVector2D& Low = Sorted[Index - 1];
		const FVector2D& High = Sorted[Index];
		if (X <= High.X)
		{
			const float Span = High.X - Low.X;
			return Span > UE_KINDA_SMALL_NUMBER ? FMath::Lerp(Low.Y, High.Y, (X - Low.X) / Span) : High.Y;
		}
	}
	return Sorted.Last().Y;
}
