// CYB3RGUN THEGAME. A flying target: flies its path, reacts to a hit, falls (D-078, D-079).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DoorRangeTarget.h"
#include "FlightPath.h"
#include "HitZoneSettings.h"
#include "FlightTarget.generated.h"

class AController;
class USphereComponent;
class UStaticMeshComponent;
class UFlightTargetDefinition;
class AFlightTarget;

DECLARE_MULTICAST_DELEGATE_ThreeParams(FFlightTargetHitDelegate, AFlightTarget* /* Target */, EHitZone /* Zone */, AController* /* InstigatedBy */);
DECLARE_MULTICAST_DELEGATE_TwoParams(FFlightTargetDoneDelegate, AFlightTarget* /* Target */, bool /* bWasHit */);

/**
 *  One flying target built from its definition. It flies a FFlightPathMotion, beats its wings, and on a hit stops,
 *  reports the hit to the style record and to its listeners, and falls or vanishes as the definition says.
 *
 *  Leading (D-078): projectiles fly to the target and meet it where it has moved to. Hitscan pellets arrive at once, so for
 *  them the target's hit spheres sit ahead of the body by the distance it covers while a charge at the lead shot speed
 *  crosses the range. Both weapons ask for the same lead. The body's own spheres answer projectiles only, the leading
 *  spheres traces only.
 */
UCLASS(NotBlueprintable)
class CYB3RGUN_API AFlightTarget : public AActor, public IDoorRangeTarget
{
	GENERATED_BODY()

public:

	AFlightTarget();

	/** Builds the body and starts the flight. ShooterLocation is where the player stands, LeadShotSpeed the speed the hitscan lead assumes. */
	void Launch(const UFlightTargetDefinition* InDefinition, const FFlightPathMotion& InMotion, float InSize, const FVector& InShooterLocation, float InLeadShotSpeed, float InGroundZ);

	/** Leaves the field at once, counted as not hit unless it already was */
	void Retire();

	//~ Begin IDoorRangeTarget
	virtual bool NotifyShot(const FHitResult& Hit, const FVector& ShotDirection, AController* InstigatedBy) override;
	//~ End IDoorRangeTarget

	const UFlightTargetDefinition* GetDefinition() const { return Definition; }
	float GetSize() const { return Size; }
	bool IsDown() const { return bDown; }
	EFlightPathType GetPathType() const { return Motion.GetType(); }

	/** Flight speed now, or at the moment of the hit once down */
	float GetFlightSpeed() const { return bDown ? SpeedAtHit : Motion.GetSpeed(); }

	/** Distance from the shooter now, or at the moment of the hit once down */
	float GetShooterDistance() const;

	/** Where a charge at the lead shot speed would meet the body, for aiming helpers */
	FVector GetLeadPoint() const;

	FVector GetFlightVelocity() const { return bDown ? FallVelocity : Motion.GetVelocity(); }

	FFlightTargetHitDelegate OnHit;
	FFlightTargetDoneDelegate OnDone;

protected:

	UPROPERTY(VisibleAnywhere, Category="Components")
	TObjectPtr<USceneComponent> Root;

	/** Turns and scales the visible body */
	UPROPERTY(VisibleAnywhere, Category="Components")
	TObjectPtr<USceneComponent> BodyRoot;

	/** Hit spheres at the body, answering projectiles */
	UPROPERTY(VisibleAnywhere, Category="Components")
	TObjectPtr<USphereComponent> BodyHit;

	UPROPERTY(VisibleAnywhere, Category="Components")
	TObjectPtr<USphereComponent> HeadHit;

	/** Hit spheres ahead of the body by the hitscan lead, answering traces */
	UPROPERTY(VisibleAnywhere, Category="Components")
	TObjectPtr<USphereComponent> BodyLead;

	UPROPERTY(VisibleAnywhere, Category="Components")
	TObjectPtr<USphereComponent> HeadLead;

	UPROPERTY(Transient)
	TObjectPtr<const UFlightTargetDefinition> Definition;

	/** Pivots of the parts that flap, with the side each belongs to */
	UPROPERTY(Transient)
	TArray<TObjectPtr<USceneComponent>> WingPivots;

	TArray<float> WingSigns;

	FFlightPathMotion Motion;
	FVector ShooterLocation = FVector::ZeroVector;
	FVector FallVelocity = FVector::ZeroVector;
	float LeadShotSpeed = 35000.0f;
	float GroundZ = 0.0f;
	float Size = 1.0f;
	float FlightTime = 0.0f;
	float FallTime = 0.0f;
	float SpeedAtHit = 0.0f;
	float DistanceAtHit = 0.0f;
	double HitAt = -1000.0;
	bool bDown = false;
	bool bDone = false;
	bool bHitSpheresOn = true;

	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void BuildBody();
	void SetupHitSphere(USphereComponent* Sphere, ECollisionChannel Channel) const;
	void PlaceHitSpheres();
	void Finish(bool bWasHit);
};
