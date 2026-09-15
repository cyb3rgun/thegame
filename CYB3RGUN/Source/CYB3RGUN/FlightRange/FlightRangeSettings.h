// CYB3RGUN THEGAME. Tunables for the flight range: the countdown, the field, the spawning and the score.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "FlightRangeSettings.generated.h"

class USoundBase;
class UFlightTargetDefinition;
class UWeaponDefinition;

/** A target the range may launch, and how often */
USTRUCT(BlueprintType)
struct FFlightTargetEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Target")
	TObjectPtr<UFlightTargetDefinition> Definition;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Target", meta = (ClampMin = 0.0))
	float Weight = 1.0f;
};

/** Handling of one loadout weapon for this mode only; the weapon definition itself stays as it is */
USTRUCT(BlueprintType)
struct FFlightWeaponOverride
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon")
	TObjectPtr<UWeaponDefinition> Weapon;

	/** Rounds in a magazine in this mode, 0 keeps the weapon's own */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon", meta = (ClampMin = 0))
	int32 MagazineSize = 0;

	/** Seconds a reload takes in this mode, 0 keeps the weapon's own */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon", meta = (ClampMin = 0.0, Units = "s"))
	float ReloadSeconds = 0.0f;
};

/**
 *  One countdown, one run, one score (D-077). The field is the half circle in front of the player start; targets enter
 *  from its sides or rise from launch points behind cover and cross it. Their value grows with distance and speed and
 *  shrinks with size. There are no waves and no difficulty ramp: the density stays the same for the whole round.
 */
UCLASS(BlueprintType)
class CYB3RGUN_API UFlightRangeSettings : public UDataAsset
{
	GENERATED_BODY()

public:

	/** Length of the round */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Round", meta = (ClampMin = 5.0, Units = "s"))
	float RoundSeconds = 90.0f;

	/** Seconds between the player spawning and the countdown starting */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Round", meta = (ClampMin = 0.0, Units = "s"))
	float StartDelaySeconds = 3.0f;

	/** Weapons handed to the player, the first comes up in hand (D-080) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Round")
	TArray<TObjectPtr<UWeaponDefinition>> Loadout;

	/** Magazine and reload of loadout weapons in this mode. A timed round must not spend its time waiting, so reloading is a rhythm here. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Round")
	TArray<FFlightWeaponOverride> WeaponOverrides;

	/** The override for a loadout weapon, or null */
	const FFlightWeaponOverride* FindWeaponOverride(const UWeaponDefinition* Weapon) const;

	/** The targets the range launches */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spawning")
	TArray<FFlightTargetEntry> Targets;

	/** Targets kept in flight at the same time */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spawning", meta = (ClampMin = 1, ClampMax = 32))
	int32 TargetsInFlight = 4;

	/** Pause between two launches, picked between these; a launch waits while the field is full */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spawning", meta = (ClampMin = 0.0, Units = "s"))
	float LaunchIntervalMin = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spawning", meta = (ClampMin = 0.0, Units = "s"))
	float LaunchIntervalMax = 1.6f;

	/** Chance a launch comes from behind cover rather than from a side, when the level has launch points */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spawning", meta = (ClampMin = 0.0, ClampMax = 1.0))
	float CoverShare = 0.3f;

	/** The same source never launches more often than this in a row */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spawning", meta = (ClampMin = 1))
	int32 MaxSameSourceInRow = 2;

	/** Nearest and farthest the path of a side entry passes in front of the player */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Field", meta = (ClampMin = 100.0, Units = "cm"))
	float CrossingDistanceMin = 900.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Field", meta = (ClampMin = 100.0, Units = "cm"))
	float CrossingDistanceMax = 2300.0f;

	/**
	 *  Every side entry passes the player within this distance at its closest point, height and wobble included, so every
	 *  target flies within the weapons' reach at some point; keep it inside the shortest range of the loadout, the scattergun's 25 m
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Field", meta = (ClampMin = 100.0, Units = "cm"))
	float ReachDistance = 2200.0f;

	/** Bearing from the player's forward direction where side entries start and end, outside a normal field of view */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Field", meta = (ClampMin = 10.0, ClampMax = 80.0, Units = "Degrees"))
	float EntryBearingDegrees = 62.0f;

	/** Largest angle a side entry's heading turns off the straight crossing */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Field", meta = (ClampMin = 0.0, ClampMax = 45.0, Units = "Degrees"))
	float HeadingJitterDegrees = 12.0f;

	/** Length of a flight from a launch point behind cover */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Field", meta = (ClampMin = 500.0, Units = "cm"))
	float CoverFlightLength = 3500.0f;

	/** Width of the whole scene in screen widths at the gallery depth. The view slides across it between hard stops (D-078). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gallery", meta = (ClampMin = 1.0, ClampMax = 10.0))
	float MapWidthScreens = 3.5f;

	/** Distance in front of the player where a screen width is measured, about where the targets cross */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gallery", meta = (ClampMin = 100.0, Units = "cm"))
	float GalleryDepth = 1500.0f;

	/** Share of the screen width at either side where the crosshair slides the view; the middle stands still */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gallery", meta = (ClampMin = 0.03, ClampMax = 0.45))
	float EdgeZone = 0.12f;

	/** Slide speed in screen widths per second with the crosshair at the very edge, or a key or the stick held; it grows from nothing at the zone's inner border */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gallery", meta = (ClampMin = 0.0))
	float ScrollSpeed = 0.45f;

	/** Share of the screen the crosshair moves per unit of mouse input */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gallery", meta = (ClampMin = 0.0))
	float MouseAimSensitivity = 0.0012f;

	/** Screens per second the crosshair moves at full right stick deflection */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gallery", meta = (ClampMin = 0.0))
	float StickAimSpeed = 0.8f;

	/** Pitch of the fixed view, positive looks up over the field */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gallery", meta = (ClampMin = -30.0, ClampMax = 30.0, Units = "Degrees"))
	float ViewPitchDegrees = 3.0f;

	/** The speed a charge is taken to fly at for the lead of hitscan pellets (D-078) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Field", meta = (ClampMin = 1000.0, Units = "cm/s"))
	float LeadShotSpeed = 35000.0f;

	/**
	 *  Score of a hit: BaseScore x clamp((Distance / ReferenceDistance) x (Speed / ReferenceSpeed) x (ReferenceSize / Size),
	 *  MinScoreFactor, MaxScoreFactor), rounded to ScoreStep and at least one step. Distance and speed are taken at the hit.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Score", meta = (ClampMin = 1.0, Units = "cm"))
	float ReferenceDistance = 1500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Score", meta = (ClampMin = 1.0, Units = "cm/s"))
	float ReferenceSpeed = 900.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Score", meta = (ClampMin = 0.1))
	float ReferenceSize = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Score", meta = (ClampMin = 0.0))
	float MinScoreFactor = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Score", meta = (ClampMin = 0.0))
	float MaxScoreFactor = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Score", meta = (ClampMin = 1))
	int32 ScoreStep = 5;

	/** Played when the countdown starts */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Feedback")
	TObjectPtr<USoundBase> StartSound;

	/** Played for each of the last seconds */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Feedback")
	TObjectPtr<USoundBase> LastSecondsSound;

	/** How many of the last seconds tick */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Feedback", meta = (ClampMin = 0))
	int32 LastSecondsCount = 5;

	/** Played when the time is up */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Feedback")
	TObjectPtr<USoundBase> EndSound;

	/** Points of a hit on a target of the given base score, distance, speed and size */
	int32 ComputeScore(int32 BaseScore, float Distance, float Speed, float Size) const;

	/** A target definition by weight, or null */
	UFlightTargetDefinition* PickTarget() const;
};
