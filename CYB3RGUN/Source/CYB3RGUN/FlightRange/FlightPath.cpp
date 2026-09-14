// CYB3RGUN THEGAME. The motion of one flying target along its path (D-078).

#include "FlightPath.h"

namespace FlightPathMotion
{
	/** Lowest a target flies above the player's ground, so no path runs into the floor */
	constexpr float MinClearance = 120.0f;
}

void FFlightPathMotion::Start(const FFlightPathSettings& InSettings, const FVector& StartLocation, const FVector& Direction, float InSpeed, float InGroundZ, float InLength)
{
	Settings = InSettings;
	Base = StartLocation;
	Heading = FVector(Direction.X, Direction.Y, 0.0f).GetSafeNormal();
	if (Heading.IsNearlyZero())
	{
		Heading = FVector::ForwardVector;
	}
	Side = FVector::CrossProduct(FVector::UpVector, Heading);
	Speed = InSpeed;
	GroundZ = InGroundZ;
	Length = FMath::Max(InLength, 100.0f);
	Travelled = 0.0f;
	Time = 0.0f;
	bPullingOut = false;
	PullOutRate = 0.0f;
	for (float& Phase : Phases)
	{
		Phase = FMath::FRandRange(0.0f, UE_TWO_PI);
	}

	// a rising target leaves upward, a diving one enters on its way down
	switch (Settings.Type)
	{
	case EFlightPathType::Rising:
		VerticalSpeed = Settings.VerticalSpeed;
		break;
	case EFlightPathType::Diving:
		VerticalSpeed = -Settings.VerticalSpeed;
		break;
	default:
		VerticalSpeed = 0.0f;
		break;
	}

	float SpeedScale = 1.0f;
	Location = Base + ComputeOffset(SpeedScale);
	Velocity = Heading * Speed + FVector::UpVector * VerticalSpeed;
}

FVector FFlightPathMotion::ComputeOffset(float& OutSpeedScale) const
{
	const float W = UE_TWO_PI * Settings.WobbleFrequency;
	const float A = Settings.WobbleAmplitude;
	OutSpeedScale = 1.0f;

	switch (Settings.Type)
	{
	case EFlightPathType::Flutter:
	{
		// a few sines at unrelated rates read as erratic without the jitter of random steps
		const float Sideways = (FMath::Sin(W * Time + Phases[0]) + 0.5f * FMath::Sin(W * 2.3f * Time + Phases[1])) * A;
		const float Up = (FMath::Sin(W * 1.7f * Time + Phases[2]) + 0.4f * FMath::Sin(W * 3.1f * Time + Phases[0])) * A * 0.6f;
		OutSpeedScale = 1.0f + 0.35f * FMath::Sin(W * 0.8f * Time + Phases[1]);
		return Side * Sideways + FVector::UpVector * Up;
	}
	case EFlightPathType::Diving:
		return FVector::ZeroVector;
	default:
		return FVector::UpVector * FMath::Sin(W * Time + Phases[0]) * A;
	}
}

void FFlightPathMotion::Advance(float DeltaSeconds)
{
	if (DeltaSeconds <= 0.0f)
	{
		return;
	}

	const FVector Previous = Location;
	Time += DeltaSeconds;

	float SpeedScale = 1.0f;
	FVector Offset = ComputeOffset(SpeedScale);
	const float Step = Speed * SpeedScale * DeltaSeconds;
	Base += Heading * Step;
	Travelled += Step;

	switch (Settings.Type)
	{
	case EFlightPathType::Rising:
		// a ballistic climb: the launch speed runs out under the path's gravity, the target then sinks back
		VerticalSpeed -= Settings.Gravity * DeltaSeconds;
		break;
	case EFlightPathType::Diving:
		if (!bPullingOut)
		{
			VerticalSpeed -= Settings.Gravity * DeltaSeconds;
			if (Base.Z <= GroundZ + Settings.LevelOutHeight && VerticalSpeed < 0.0f)
			{
				// the pull out takes the drop speed away over a fixed time
				bPullingOut = true;
				PullOutRate = -VerticalSpeed / FMath::Max(Settings.PullOutSeconds, 0.05f);
			}
		}
		else
		{
			VerticalSpeed = FMath::Min(VerticalSpeed + PullOutRate * DeltaSeconds, 0.0f);
		}
		break;
	default:
		break;
	}
	Base.Z += VerticalSpeed * DeltaSeconds;
	Base.Z = FMath::Max(Base.Z, GroundZ + FlightPathMotion::MinClearance);

	Location = Base + Offset;
	Location.Z = FMath::Max(Location.Z, GroundZ + FlightPathMotion::MinClearance);
	Velocity = (Location - Previous) / DeltaSeconds;
}
