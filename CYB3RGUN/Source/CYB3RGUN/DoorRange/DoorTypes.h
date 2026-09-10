// CYB3RGUN THEGAME. Door module shared types.

#pragma once

#include "CoreMinimal.h"
#include "DoorTypes.generated.h"

/** What stands behind a door when it opens. */
UENUM(BlueprintType)
enum class EDoorOccupant : uint8
{
	Empty,
	Hostile,
	Friendly
};

/** Lifecycle of a single door slot. */
UENUM(BlueprintType)
enum class EDoorState : uint8
{
	Closed,
	Opening,
	Showing,
	Closing
};
