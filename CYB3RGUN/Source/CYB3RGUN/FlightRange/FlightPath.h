// CYB3RGUN THEGAME. The motion of one flying target along its path (D-078).

#pragma once

#include "CoreMinimal.h"
#include "FlightTargetDefinition.h"

/**
 *  Integrates one target along a path of its definition: a base point that travels across the field at the flight speed,
 *  climbing or dropping under the path's gravity, plus a small offset for the bob or the flutter. Every value comes from
 *  FFlightPathSettings; the spawn director only picks where the path starts, which way it heads and how long it is.
 */
struct CYB3RGUN_API FFlightPathMotion
{
	/** Starts the path. Direction is flattened to the horizontal; GroundZ is the height of the player's ground. */
	void Start(const FFlightPathSettings& InSettings, const FVector& StartLocation, const FVector& Direction, float InSpeed, float InGroundZ, float InLength);

	/** Moves the target on by one step */
	void Advance(float DeltaSeconds);

	FVector GetLocation() const { return Location; }
	FVector GetVelocity() const { return Velocity; }
	float GetSpeed() const { return Velocity.Size(); }
	EFlightPathType GetType() const { return Settings.Type; }

	/** True once the target has covered the length of its path */
	bool IsFinished() const { return Travelled >= Length; }

private:

	FFlightPathSettings Settings;
	FVector Base = FVector::ZeroVector;
	FVector Heading = FVector::ForwardVector;
	FVector Side = FVector::RightVector;
	FVector Location = FVector::ZeroVector;
	FVector Velocity = FVector::ZeroVector;
	float Speed = 0.0f;
	float VerticalSpeed = 0.0f;
	float PullOutRate = 0.0f;
	float GroundZ = 0.0f;
	float Length = 0.0f;
	float Travelled = 0.0f;
	float Time = 0.0f;
	float Phases[3] = { 0.0f, 0.0f, 0.0f };
	bool bPullingOut = false;

	/** The bob or flutter offset at the current time, sideways and up */
	FVector ComputeOffset(float& OutSpeedScale) const;
};
