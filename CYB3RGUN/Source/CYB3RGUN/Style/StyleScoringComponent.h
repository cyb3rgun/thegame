// CYB3RGUN THEGAME. The style record of one player: meter, combo, points and run statistics.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HitZoneSettings.h"
#include "StyleScoringComponent.generated.h"

class AController;
class UStyleSettings;

/** What the style record reacts to */
UENUM(BlueprintType)
enum class EStyleEvent : uint8
{
	Hit,
	Kill,
	Headshot,
	ControlledPair,
	Rescue,
	Miss,
	Penalty,
	/** A weapon shot out of a hand, or dropped from a hit arm: the threat is gone without a kill (D-050) */
	Disarm,
	/** Zone bonus of a leg hit */
	LegShot,
	/** Zone bonus of an arm hit */
	ArmShot
};

/** Numbers of one run, for the end of run summaries */
USTRUCT(BlueprintType)
struct FStyleRunStats
{
	GENERATED_BODY()

	/** Trigger pulls; the pellets of one pull are one shot */
	UPROPERTY(BlueprintReadOnly)
	int32 ShotsFired = 0;

	/** Shots that landed on at least one target */
	UPROPERTY(BlueprintReadOnly)
	int32 ShotsHit = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 Kills = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 Headshots = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 ControlledPairs = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 Rescues = 0;

	/** Hits on hostages and bystanders */
	UPROPERTY(BlueprintReadOnly)
	int32 Penalties = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 Misses = 0;

	/** Targets disarmed, cleanly or through the weapon arm */
	UPROPERTY(BlueprintReadOnly)
	int32 Disarms = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 LegShots = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 ArmShots = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 BestCombo = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 StylePoints = 0;

	UPROPERTY(BlueprintReadOnly)
	float PeakMeter = 0.0f;

	/** Share of shots that landed on a target, 0 to 1 */
	float GetAccuracy() const { return ShotsFired > 0 ? static_cast<float>(ShotsHit) / ShotsFired : 0.0f; }
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FStyleEventDelegate, EStyleEvent, Event, int32, Points, AActor*, Target);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FStyleComboDelegate, int32, Combo, float, Multiplier);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FStyleShotDelegate, int32, ShotsFired);

/**
 *  Scores how the player shoots, the same in every scenario (D-044). Clean actions raise the style meter:
 *  hits, kills, disarms, controlled pairs, rescues, and the bonus of the zone a shot landed in (D-049, D-050).
 *  A miss takes meter and combo, a hit on a hostage or bystander drops the meter to the floor and the combo to
 *  zero. Consecutive clean kills and disarms raise a multiplier that applies to every clean action. Every value
 *  comes from UStyleSettings (D-046), the zone bonuses from the zone scores of UHitZoneSettings.
 *
 *  Lives on the local player's controller and is created on first use, so a scenario needs no setup.
 *  Shot paths record each shot and bracket its resolution; whatever the shot lands on reports the outcome,
 *  and a resolution without a target hit is a miss.
 */
UCLASS(ClassGroup=(CYB3RGUN), meta = (BlueprintSpawnableComponent))
class CYB3RGUN_API UStyleScoringComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UStyleScoringComponent();

	/** The style record of the local player behind a pawn or controller, created on first use. Null for anyone else. */
	static UStyleScoringComponent* Get(AActor* PawnOrController);

	/** The style record of an instigating controller, null unless it is a local player */
	static UStyleScoringComponent* ForController(AController* Controller);

	/** Starts a new run: statistics, meter and combo back to zero */
	UFUNCTION(BlueprintCallable, Category="Style")
	void ResetRun();

	/** A trigger pull; call once per shot, not per pellet */
	void RecordShotFired();

	/** Brackets everything one shot lands on. Nested calls join the outer resolution. */
	void BeginShotResolution();
	void EndShotResolution();

	/**
	 *  A shot landed on a target in a hit zone. Several reports for one target within one resolution count once, and the best
	 *  zone among them pays its bonus when the resolution ends. The head bonus needs the target brought down.
	 */
	void RecordTargetHit(AActor* Target, EHitZone Zone, bool bKill);

	/** A shot landed on a target, on the head or not. Kept for callers without a hit zone. */
	void RecordTargetHit(AActor* Target, bool bHeadshot, bool bKill);

	/** A target lost its weapon to a shot: on the weapon itself when clean, through the weapon arm otherwise. Counts once per target and shot. */
	void RecordDisarm(AActor* Target, bool bClean);

	/** A shot landed on a hostage or bystander */
	void RecordNonTargetHit(AActor* Victim);

	/** A hostage was freed */
	void RecordRescue(AActor* FreedFrom);

	UFUNCTION(BlueprintPure, Category="Style")
	float GetMeter() const { return Meter; }

	UFUNCTION(BlueprintPure, Category="Style")
	float GetMeterFraction() const;

	UFUNCTION(BlueprintPure, Category="Style")
	FText GetRankLabel() const;

	UFUNCTION(BlueprintPure, Category="Style")
	int32 GetCombo() const { return Combo; }

	UFUNCTION(BlueprintPure, Category="Style")
	float GetMultiplier() const;

	UFUNCTION(BlueprintPure, Category="Style")
	const FStyleRunStats& GetStats() const { return Stats; }

	void LogStatus() const;

	UPROPERTY(BlueprintAssignable, Category="Style")
	FStyleEventDelegate OnStyleEvent;

	UPROPERTY(BlueprintAssignable, Category="Style")
	FStyleComboDelegate OnComboChanged;

	UPROPERTY(BlueprintAssignable, Category="Style")
	FStyleShotDelegate OnShotFired;

protected:

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** One target of the shot being resolved */
	struct FResolvedTarget
	{
		TWeakObjectPtr<AActor> Target;
		bool bKilled = false;
		bool bDisarmed = false;

		/** Best zone with a bonus reported for this target so far, and its points, paid when the resolution ends */
		EHitZone BonusZone = EHitZone::None;
		int32 BonusPoints = 0;

		/** Multiplier when the target was first hit, so its zone bonus is paid at the rate its kill was */
		float Multiplier = 1.0f;
	};

	FStyleRunStats Stats;
	float Meter = 0.0f;
	int32 Combo = 0;

	/** World time of the last clean action, the meter holds until the decay delay after it */
	double LastCleanTime = 0.0;

	/** Last target hit and when, for controlled pairs */
	TWeakObjectPtr<AActor> LastHitTarget;
	double LastHitTime = -1000.0;

	int32 ResolutionDepth = 0;
	bool bResolutionHitTarget = false;
	bool bResolutionPenalty = false;
	TArray<FResolvedTarget> ResolvedTargets;

	const UStyleSettings* GetSettings() const;
	double Now() const;
	void RecordMiss();
	void ScoreKill(AActor* Target);

	/** The entry of a target in the resolution; its first report adds it and checks for a controlled pair */
	FResolvedTarget& FindOrAddTarget(AActor* Target, bool& bOutAdded);

	/** Keeps the zone with the higher bonus for a target of the resolution */
	void NoteZone(FResolvedTarget& Entry, EHitZone Zone) const;

	/** Pays the zone bonus of every target of the resolution that is ending */
	void PayZoneBonuses();

	void Award(EStyleEvent Event, int32 BasePoints, AActor* Target, bool bMultiplied);
	void AwardAt(EStyleEvent Event, int32 BasePoints, AActor* Target, float Multiplier);
	void AddMeter(float Delta);
	void SetCombo(int32 NewCombo);
};
