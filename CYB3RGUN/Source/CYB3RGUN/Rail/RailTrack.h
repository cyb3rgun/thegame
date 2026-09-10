// CYB3RGUN THEGAME. A rail segment: a spline route plus the beats that happen along it.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "RailTrack.generated.h"

class USplineComponent;
class UEncounterDefinition;
class AEnemySpawnPoint;
class ARailPawn;

/** Something that happens at a distance along a rail segment */
USTRUCT(BlueprintType)
struct FRailBeat
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Beat")
	FName Name;

	/** Distance along this segment's spline at which the beat fires */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Beat", meta = (ClampMin = 0.0, Units = "cm"))
	float TriggerDistance = 0.0f;

	/** Encounter to run when the beat fires. Optional. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Beat")
	TObjectPtr<UEncounterDefinition> Encounter;

	/** Spawn points for the encounter. Empty means every spawn point in the level. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Beat")
	TArray<TObjectPtr<AEnemySpawnPoint>> SpawnPoints;

	/** Stops the ride at the trigger distance until the beat is cleared */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Beat")
	bool bHoldUntilCleared = false;

	/** Changes the ride speed from this beat on */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Beat")
	bool bOverrideSpeed = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Beat", meta = (ClampMin = 0.0, EditCondition = "bOverrideSpeed"))
	float SpeedOverride = 0.0f;

	/** Hook for later setpieces, for example a Level Sequence started by this tag */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Beat")
	FGameplayTag SequenceTag;
};

/**
 *  One segment of a rail route (D-020). The spline is only geometry; the ride itself is driven by the
 *  rail pawn's own distance scalar. Branches are separate segments listed in NextTracks.
 */
UCLASS()
class CYB3RGUN_API ARailTrack : public AActor
{
	GENERATED_BODY()

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Rail")
	TObjectPtr<USplineComponent> Spline;

	/** Beats along this segment. Any order in the editor, sorted by distance when play starts. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Rail")
	TArray<FRailBeat> Beats;

	/** Segments that can follow this one. Empty ends the ride here. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Rail")
	TArray<TObjectPtr<ARailTrack>> NextTracks;

	/**
	 *  Optional route in actor space. When set, the spline is rebuilt from these points on construction,
	 *  which keeps a route authorable as plain data. Empty leaves the spline as edited in the viewport.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Rail")
	TArray<FVector> RoutePoints;

public:

	ARailTrack();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintPure, Category="Rail")
	USplineComponent* GetSpline() const { return Spline; }

	/** Length of the spline in centimetres */
	UFUNCTION(BlueprintPure, Category="Rail")
	float GetLength() const;

	/** World transform at a distance along the spline, clamped to the segment */
	UFUNCTION(BlueprintPure, Category="Rail")
	FTransform GetWorldTransformAtDistance(float Distance) const;

	/** Beats sorted by trigger distance once play has started */
	const TArray<FRailBeat>& GetBeats() const { return Beats; }

	const TArray<TObjectPtr<ARailTrack>>& GetNextTracks() const { return NextTracks; }

	/** Picks the segment that follows when a rider reaches the end. Default: the first valid entry. Override for branches. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Rail")
	ARailTrack* SelectNextTrack(ARailPawn* Rider) const;

protected:

	void RebuildFromRoutePoints();
};
