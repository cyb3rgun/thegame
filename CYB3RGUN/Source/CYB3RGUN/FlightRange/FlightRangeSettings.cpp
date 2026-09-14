// CYB3RGUN THEGAME. Tunables for the flight range: the countdown, the field, the spawning and the score.

#include "FlightRangeSettings.h"
#include "FlightTargetDefinition.h"
#include "WeaponDefinition.h"

int32 UFlightRangeSettings::ComputeScore(int32 BaseScore, float Distance, float Speed, float Size) const
{
	// far, fast and small is hard: distance and speed raise the value, size lowers it
	const float DistanceFactor = Distance / FMath::Max(ReferenceDistance, 1.0f);
	const float SpeedFactor = Speed / FMath::Max(ReferenceSpeed, 1.0f);
	const float SizeFactor = ReferenceSize / FMath::Max(Size, 0.1f);
	const float Factor = FMath::Clamp(DistanceFactor * SpeedFactor * SizeFactor, MinScoreFactor, FMath::Max(MinScoreFactor, MaxScoreFactor));

	const int32 Step = FMath::Max(ScoreStep, 1);
	const int32 Rounded = FMath::RoundToInt(BaseScore * Factor / Step) * Step;
	return FMath::Max(Rounded, Step);
}

const FFlightWeaponOverride* UFlightRangeSettings::FindWeaponOverride(const UWeaponDefinition* Weapon) const
{
	return Weapon ? WeaponOverrides.FindByPredicate([Weapon](const FFlightWeaponOverride& Entry) { return Entry.Weapon == Weapon; }) : nullptr;
}

UFlightTargetDefinition* UFlightRangeSettings::PickTarget() const
{
	float Total = 0.0f;
	for (const FFlightTargetEntry& Entry : Targets)
	{
		if (Entry.Definition && Entry.Weight > 0.0f)
		{
			Total += Entry.Weight;
		}
	}
	if (Total <= 0.0f)
	{
		return nullptr;
	}

	float Roll = FMath::FRandRange(0.0f, Total);
	UFlightTargetDefinition* Last = nullptr;
	for (const FFlightTargetEntry& Entry : Targets)
	{
		if (!Entry.Definition || Entry.Weight <= 0.0f)
		{
			continue;
		}
		Last = Entry.Definition;
		Roll -= Entry.Weight;
		if (Roll <= 0.0f)
		{
			return Entry.Definition;
		}
	}
	return Last;
}
