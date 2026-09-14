// CYB3RGUN THEGAME. Everything a flying target is, as data (D-079).

#include "FlightTargetDefinition.h"

const FFlightPathSettings* UFlightTargetDefinition::PickPath(const TArray<EFlightPathType>& Allowed) const
{
	float Total = 0.0f;
	for (const FFlightPathSettings& Path : Paths)
	{
		if (Path.Weight > 0.0f && (Allowed.IsEmpty() || Allowed.Contains(Path.Type)))
		{
			Total += Path.Weight;
		}
	}
	if (Total <= 0.0f)
	{
		return nullptr;
	}

	float Roll = FMath::FRandRange(0.0f, Total);
	const FFlightPathSettings* Last = nullptr;
	for (const FFlightPathSettings& Path : Paths)
	{
		if (Path.Weight <= 0.0f || (!Allowed.IsEmpty() && !Allowed.Contains(Path.Type)))
		{
			continue;
		}
		Last = &Path;
		Roll -= Path.Weight;
		if (Roll <= 0.0f)
		{
			return &Path;
		}
	}
	return Last;
}
