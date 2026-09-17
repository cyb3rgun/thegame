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

FFlightSpriteLoop UFlightTargetDefinition::PickSpriteLoop() const
{
	// no loop listed: the whole sheet, in the order the artist set (D-092)
	FFlightSpriteLoop Whole;
	Whole.FirstFrame = 0;
	Whole.FrameCount = Sprite.Flight.Frames;

	float Total = 0.0f;
	for (const FFlightSpriteLoop& Loop : Sprite.Loops)
	{
		Total += FMath::Max(Loop.Weight, 0.0f);
	}
	if (Total <= 0.0f)
	{
		return Whole;
	}

	float Roll = FMath::FRandRange(0.0f, Total);
	const FFlightSpriteLoop* Last = nullptr;
	for (const FFlightSpriteLoop& Loop : Sprite.Loops)
	{
		if (Loop.Weight <= 0.0f)
		{
			continue;
		}
		Last = &Loop;
		Roll -= Loop.Weight;
		if (Roll <= 0.0f)
		{
			break;
		}
	}
	if (!Last)
	{
		return Whole;
	}

	FFlightSpriteLoop Picked = *Last;
	Picked.FirstFrame = FMath::Clamp(Picked.FirstFrame, 0, FMath::Max(Sprite.Flight.Frames - 1, 0));
	const int32 Rest = Sprite.Flight.Frames - Picked.FirstFrame;
	Picked.FrameCount = Picked.FrameCount > 0 ? FMath::Min(Picked.FrameCount, Rest) : Rest;
	return Picked;
}
