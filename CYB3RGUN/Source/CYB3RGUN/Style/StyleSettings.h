// CYB3RGUN THEGAME. Every value of the style system in one data asset (D-044, D-046).

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "StyleSettings.generated.h"

/** A band of the style meter, its label shows while the meter is at or above the threshold */
USTRUCT(BlueprintType)
struct FStyleRank
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = 0.0))
	float Threshold = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText Label;
};

/** One strength of the directional camera kick: a small turn toward the target that dies away fast */
USTRUCT(BlueprintType)
struct FStyleShake
{
	GENERATED_BODY()

	/** Largest turn of the view */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = 0.0, ClampMax = 5.0, Units = "Degrees"))
	float Degrees = 0.5f;

	/** Wall clock seconds from the kick back to rest */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = 0.02, ClampMax = 1.0, Units = "s"))
	float DecaySeconds = 0.12f;

	/** Swings per second while it dies away */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = 0.0, ClampMax = 40.0))
	float Frequency = 16.0f;
};

/**
 *  Thresholds, windows, bonuses and decay rates of the style meter and its scoring, tuned without a rebuild
 *  (D-046). Clean, disciplined shooting scores highest, and a hit on anyone who is not a target collapses the
 *  meter at once (D-044). A scenario names its own asset through IStyleScenario, every other scenario uses
 *  the project default from Project Settings, Game, Style. The bonus of the zone a shot lands in, the headshot
 *  among them, is the zone's score in UHitZoneSettings (D-049).
 */
UCLASS(BlueprintType)
class CYB3RGUN_API UStyleSettings : public UDataAsset
{
	GENERATED_BODY()

public:

	UStyleSettings();

	/** Points for a hit on a target that does not bring it down */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Scoring", meta = (ClampMin = 0))
	int32 HitPoints = 20;

	/** Points for bringing a target down */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Scoring", meta = (ClampMin = 0))
	int32 KillPoints = 100;

	/** Added when a second hit lands on the same target within the pair window, so a controlled pair is worth more than two separate hits */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Scoring", meta = (ClampMin = 0))
	int32 ControlledPairBonus = 80;

	/** Seconds between two hits on one target that still make a controlled pair. The pistol cycles in 0.18 s, so the window
	 *  asks for a deliberate double tap, while steady single shots about 0.3 s apart no longer count as a pair */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Scoring", meta = (ClampMin = 0.05, ClampMax = 2.0, Units = "s"))
	float ControlledPairWindow = 0.25f;

	/** Points for freeing a hostage */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Scoring", meta = (ClampMin = 0))
	int32 RescueBonus = 250;

	/** Points taken for a hit on a hostage or a bystander, never multiplied */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Scoring", meta = (ClampMin = 0))
	int32 NonTargetPenalty = 300;

	/** Seconds a projectile may fly before its shot counts as a miss */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Scoring", meta = (ClampMin = 0.5, Units = "s"))
	float ShotMissTimeout = 3.0f;

	/** Consecutive clean kills or disarms that raise the multiplier by one step */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combo", meta = (ClampMin = 1))
	int32 KillsPerMultiplierStep = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combo", meta = (ClampMin = 0.0))
	float MultiplierStep = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combo", meta = (ClampMin = 1.0))
	float MaxMultiplier = 4.0f;

	/** Kills a miss takes off the combo */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combo", meta = (ClampMin = 0))
	int32 MissComboLoss = 2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Meter", meta = (ClampMin = 1.0))
	float MeterMax = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Meter", meta = (ClampMin = 0.0))
	float MeterPerHit = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Meter", meta = (ClampMin = 0.0))
	float MeterPerKill = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Meter", meta = (ClampMin = 0.0))
	float MeterPerHeadshot = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Meter", meta = (ClampMin = 0.0))
	float MeterPerControlledPair = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Meter", meta = (ClampMin = 0.0))
	float MeterPerRescue = 20.0f;

	/** A weapon shot out of a hand or dropped from a hit arm, more than a kill (D-050) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Meter", meta = (ClampMin = 0.0))
	float MeterPerDisarm = 16.0f;

	/** Taken off the meter by a miss */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Meter", meta = (ClampMin = 0.0))
	float MeterPerMiss = 10.0f;

	/** Where the meter lands after a hit on a non target: the collapse */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Meter", meta = (ClampMin = 0.0))
	float MeterAfterPenalty = 0.0f;

	/** Seconds without a clean action before the meter starts to drain. Longer than the pause between door range waves, so the
	 *  meter carries from one wave into the next */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Meter", meta = (ClampMin = 0.0, Units = "s"))
	float MeterDecayDelay = 4.0f;

	/** Drain once the hold has run out. A full meter takes more than a minute to empty, a lull costs a little, not the rank */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Meter", meta = (ClampMin = 0.0))
	float MeterDecayPerSecond = 1.5f;

	/** Named bands of the meter, lowest threshold first */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Meter")
	TArray<FStyleRank> Ranks;

	/** Radius of an enemy's head around its head bone. A shot is a head hit when its line passes this close to the head. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hits", meta = (ClampMin = 1.0, Units = "cm"))
	float EnemyHeadRadius = 15.0f;

	/** Where the head sits on an enemy without a skeletal body, measured down from the top of its capsule */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hits", meta = (ClampMin = 0.0, Units = "cm"))
	float PlaceholderHeadDepth = 14.0f;

	/** Wall clock seconds the world stops on a kill */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Feel", meta = (ClampMin = 0.0, ClampMax = 0.2, Units = "s"))
	float HitStopSeconds = 0.05f;

	/** Hit stop on a kill with a head hit, a touch longer */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Feel", meta = (ClampMin = 0.0, ClampMax = 0.2, Units = "s"))
	float HeadshotHitStopSeconds = 0.08f;

	/** World speed during a hit stop, close to a standstill */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Feel", meta = (ClampMin = 0.001, ClampMax = 1.0))
	float HitStopDilation = 0.02f;

	/** Camera kick on a hit that does not bring the target down */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Feel")
	FStyleShake LightShake;

	/** Camera kick on a kill */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Feel")
	FStyleShake MediumShake;

	/** Camera kick on a kill with a head hit, on a disarm, on a rescue and on a penalty */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Feel")
	FStyleShake HeavyShake;

	/** Wall clock seconds the screen effect of a kill lasts */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Feel", meta = (ClampMin = 0.0, ClampMax = 1.0, Units = "s"))
	float KillFlashSeconds = 0.12f;

	/** Strength of the kill screen effect: vignette, colour fringe and a drop in saturation */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Feel", meta = (ClampMin = 0.0, ClampMax = 1.0))
	float KillFlashStrength = 0.6f;

	/** Wall clock seconds the projected HUD glitches after the player takes damage (D-051) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Feel", meta = (ClampMin = 0.0, ClampMax = 2.0, Units = "s"))
	float DamageGlitchSeconds = 0.4f;

	/** Overclock can be used in this scenario. Off for a precision scenario. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Overclock")
	bool bOverclockAllowed = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Overclock", meta = (ClampMin = 1.0))
	float OverclockMax = 100.0f;

	/** Charge for a clean hit that does not bring the target down */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Overclock", meta = (ClampMin = 0.0))
	float OverclockPerHit = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Overclock", meta = (ClampMin = 0.0))
	float OverclockPerKill = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Overclock", meta = (ClampMin = 0.0))
	float OverclockPerHeadshot = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Overclock", meta = (ClampMin = 0.0))
	float OverclockPerControlledPair = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Overclock", meta = (ClampMin = 0.0))
	float OverclockPerRescue = 20.0f;

	/** Charge for a disarm, more than a kill (D-050) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Overclock", meta = (ClampMin = 0.0))
	float OverclockPerDisarm = 20.0f;

	/** Charge needed to start, so a press on an almost empty meter does nothing */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Overclock", meta = (ClampMin = 0.0))
	float OverclockMinToStart = 25.0f;

	/** Charge used per wall clock second while it runs */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Overclock", meta = (ClampMin = 1.0))
	float OverclockDrainPerSecond = 20.0f;

	/** Speed of the world while it runs */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Overclock", meta = (ClampMin = 0.05, ClampMax = 1.0))
	float OverclockWorldDilation = 0.35f;

	/** Speed of the player while it runs, against the wall clock and separate from the world */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Overclock", meta = (ClampMin = 0.05, ClampMax = 1.0))
	float OverclockPlayerScale = 0.7f;

	/** Wall clock seconds to ease into and out of the slowdown */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Overclock", meta = (ClampMin = 0.0, ClampMax = 1.0, Units = "s"))
	float OverclockBlendSeconds = 0.15f;

	/** Strength of the screen look while it runs */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Overclock", meta = (ClampMin = 0.0, ClampMax = 1.0))
	float OverclockLookStrength = 0.5f;

	/** The values that apply in this object's world: the scenario's own asset, else the project default */
	static const UStyleSettings* Get(const UObject* WorldContext);

	/** Label of the band the meter value falls in */
	FText GetRankLabel(float MeterValue) const;
};
