// CYB3RGUN THEGAME. Interface for anything a weapon shot can score on.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "DoorRangeTarget.generated.h"

class AController;
struct FHitResult;

UINTERFACE(MinimalAPI)
class UDoorRangeTarget : public UInterface
{
	GENERATED_BODY()
};

/**
 *  Implemented by actors that react to weapon hits with scoring logic.
 *  Weapons call NotifyShot on the actor they hit with the full hit, its component and bone, and the direction the shot
 *  travelled; the target decides whether the shot counts and which hit zone it landed in (D-049).
 */
class CYB3RGUN_API IDoorRangeTarget
{
	GENERATED_BODY()

public:

	/** Called by a weapon or projectile when a shot lands on this actor, before any damage. Returns true if the shot was accepted. */
	virtual bool NotifyShot(const FHitResult& Hit, const FVector& ShotDirection, AController* InstigatedBy) = 0;
};
