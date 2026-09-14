// CYB3RGUN THEGAME. A place behind cover where flight targets rise from.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FlightLaunchPoint.generated.h"

class UArrowComponent;

/**
 *  Marks a spot behind a bush, a wall or a tree line. The spawn director launches targets from here on rising paths, so
 *  a round is not only side entries. Place it at the height the target should appear, hidden from the player start.
 */
UCLASS()
class CYB3RGUN_API AFlightLaunchPoint : public AActor
{
	GENERATED_BODY()

public:

	AFlightLaunchPoint();

protected:

	UPROPERTY(VisibleAnywhere, Category="Components")
	TObjectPtr<USceneComponent> Root;

#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, Category="Components")
	TObjectPtr<UArrowComponent> Arrow;
#endif
};
