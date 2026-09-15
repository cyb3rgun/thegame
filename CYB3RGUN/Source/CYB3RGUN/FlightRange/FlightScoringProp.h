// CYB3RGUN THEGAME. A fixed piece of the flight range scene that pays or costs points when shot: a windmill, a scarecrow, a sign.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DoorRangeTarget.h"
#include "FlightScoringProp.generated.h"

class AController;
class UStaticMeshComponent;

/** How the moving part answers a shot */
UENUM(BlueprintType)
enum class EFlightPropReaction : uint8
{
	/** Turns on the reaction axis all the time and spins up when hit, like windmill sails */
	Spin,
	/** Rests, and swings about the reaction axis and settles when hit, like a scarecrow or a hanging sign */
	Sway
};

/**
 *  Stands in the flight range scene and scores through the range's game mode when a shot lands on it. A positive value is a
 *  bonus that pays once per round, so it cannot be farmed; a negative value is a penalty that costs on every shot that
 *  lands (D-077: the sign is the mode's only penalty). The pellets of one shot count once. The props do not touch the
 *  style record: they are not targets. The body is the still part, the moving part turns on its pivot.
 */
UCLASS()
class CYB3RGUN_API AFlightScoringProp : public AActor, public IDoorRangeTarget
{
	GENERATED_BODY()

public:

	AFlightScoringProp();

	/** Clears what the prop paid, for a new round */
	void ResetForRound();

	int32 GetPoints() const { return Points; }
	const FText& GetDisplayName() const { return DisplayName; }

	//~ Begin IDoorRangeTarget
	virtual bool NotifyShot(const FHitResult& Hit, const FVector& ShotDirection, AController* InstigatedBy) override;
	//~ End IDoorRangeTarget

protected:

	UPROPERTY(VisibleAnywhere, Category="Components")
	TObjectPtr<USceneComponent> Root;

	/** The part that stands still: the tower, the post */
	UPROPERTY(VisibleAnywhere, Category="Components")
	TObjectPtr<UStaticMeshComponent> Body;

	/** Pivot of the moving part, placed where it turns or hangs */
	UPROPERTY(VisibleAnywhere, Category="Components")
	TObjectPtr<USceneComponent> MovingPivot;

	/** The part that reacts: the sails, the scarecrow, the board */
	UPROPERTY(VisibleAnywhere, Category="Components")
	TObjectPtr<UStaticMeshComponent> Moving;

	/** Shown with the points when it scores */
	UPROPERTY(EditAnywhere, Category="Score")
	FText DisplayName;

	/** Points a landed shot adds, negative for a penalty. A bonus pays once per round, a penalty every time. */
	UPROPERTY(EditAnywhere, Category="Score")
	int32 Points = 100;

	UPROPERTY(EditAnywhere, Category="Reaction")
	EFlightPropReaction Reaction = EFlightPropReaction::Sway;

	/** Axis of the moving part in the pivot's space the reaction turns about */
	UPROPERTY(EditAnywhere, Category="Reaction")
	FVector ReactionAxis = FVector::ForwardVector;

	/** Spin only: degrees per second while nothing happens */
	UPROPERTY(EditAnywhere, Category="Reaction", meta = (Units = "Degrees"))
	float IdleSpinSpeed = 25.0f;

	/** Spin: degrees per second added by a hit and running down again. Sway: the swing's first amplitude in degrees. */
	UPROPERTY(EditAnywhere, Category="Reaction", meta = (ClampMin = 0.0))
	float HitStrength = 360.0f;

	/** Sway only: swings per second */
	UPROPERTY(EditAnywhere, Category="Reaction", meta = (ClampMin = 0.1))
	float SwayFrequency = 1.4f;

	/** How fast the reaction dies down, per second */
	UPROPERTY(EditAnywhere, Category="Reaction", meta = (ClampMin = 0.0))
	float Damping = 1.2f;

	/** The moving part's rotation as placed, the reaction turns on top of it */
	FQuat RestRotation = FQuat::Identity;

	double LastScoredAt = -1000.0;
	float Angle = 0.0f;
	float Boost = 0.0f;
	float SwayAge = 1000.0f;
	float SwaySign = 1.0f;
	bool bPaidThisRound = false;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
};
