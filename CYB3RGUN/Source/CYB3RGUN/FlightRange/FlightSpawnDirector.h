// CYB3RGUN THEGAME. Keeps the flight range sky busy: launches targets from the sides and from behind cover.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FlightSpawnDirector.generated.h"

class AFlightLaunchPoint;
class AFlightTarget;
class UFlightGalleryControls;
class UFlightRangeSettings;

/** Where a target entered the field */
UENUM(BlueprintType)
enum class EFlightSource : uint8
{
	Left,
	Right,
	Cover
};

DECLARE_MULTICAST_DELEGATE_TwoParams(FFlightTargetLaunchedDelegate, AFlightTarget* /* Target */, EFlightSource /* Source */);

/**
 *  Holds a steady number of targets in flight for as long as it runs. Each launch picks a target definition and one of its
 *  paths, and a source: the left or right edge of the half circle in front of the player, or a launch point behind cover.
 *  Sources are weighted against their recent use and one source never launches more often in a row than the settings
 *  allow, so the targets never all come from one direction. Side entries start outside a normal field of view and cross
 *  in front of the player at a picked distance; cover launches rise from their point. On the gallery (D-078) side entries
 *  are planned for a place along the rail the player can slide to, most of them near the current view.
 */
UCLASS()
class CYB3RGUN_API AFlightSpawnDirector : public AActor
{
	GENERATED_BODY()

public:

	AFlightSpawnDirector();

	/** Starts launching. Field is the player start: its location on the ground and the yaw the field opens towards. */
	void StartLaunching(const UFlightRangeSettings* InSettings, const FTransform& InField, const FVector& InShooterLocation);

	/** Spreads side entries over the gallery's whole width and keeps every target's distance on the player as it slides */
	void SetGallery(UFlightGalleryControls* InGallery);

	/** Stops launching; targets still in the air fly on unless retired */
	void StopLaunching(bool bRetireTargets);

	bool IsLaunching() const { return bLaunching; }

	/** Targets in the air and not hit */
	int32 CountInFlight() const;

	/** Targets launched so far from a source */
	int32 GetLaunchCount(EFlightSource Source) const { return LaunchCounts[static_cast<int32>(Source)]; }

	/** Every target that is still in the level */
	void GetTargets(TArray<AFlightTarget*>& OutTargets) const;

	FFlightTargetLaunchedDelegate OnTargetLaunched;

protected:

	UPROPERTY(Transient)
	TObjectPtr<const UFlightRangeSettings> Settings;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AFlightLaunchPoint>> LaunchPoints;

	TArray<TWeakObjectPtr<AFlightTarget>> Targets;

	TWeakObjectPtr<UFlightGalleryControls> Gallery;

	/** Sources of the last launches, newest last */
	TArray<EFlightSource> RecentSources;

	FTransform Field;
	FVector ShooterLocation = FVector::ZeroVector;
	float TimeToNextLaunch = 0.0f;
	int32 LastLaunchPoint = INDEX_NONE;
	int32 LaunchCounts[3] = { 0, 0, 0 };
	bool bLaunching = false;

	virtual void Tick(float DeltaSeconds) override;

	EFlightSource PickSource() const;
	void Launch();

	/** Sideways place on the gallery rail a side entry is planned for: near the view most of the time, anywhere on the rail otherwise */
	float PickGalleryOffset() const;

	/** Start, heading and length of a flight from a side or from a launch point. Lateral shifts the field along the gallery rail. */
	bool PlanSideFlight(float SideSign, float MaxCrossing, const FVector& Lateral, FVector& OutStart, FVector& OutHeading, float& OutLength, float& OutCrossing) const;
	bool PlanCoverFlight(FVector& OutStart, FVector& OutHeading, float& OutLength);
};
