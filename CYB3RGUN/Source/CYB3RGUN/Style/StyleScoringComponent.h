// CYB3RGUN THEGAME. The style record of one player: meter, combo, points and run statistics.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
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
	Penalty
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
 *  hits, kills, headshots, controlled pairs and rescues. A miss takes meter and combo, a hit on a hostage or
 *  bystander drops the meter to the floor and the combo to zero. Consecutive clean kills raise a multiplier
 *  that applies to every clean action. Every value comes from UStyleSettings (D-046).
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

	/** A shot landed on a target. Several reports for one target within one resolution count once. */
	void RecordTargetHit(AActor* Target, bool bHeadshot, bool bKill);

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
	void ScoreKill(AActor* Target, bool bHeadshot);
	void Award(EStyleEvent Event, int32 BasePoints, AActor* Target, bool bMultiplied);
	void AddMeter(float Delta);
	void SetCombo(int32 NewCombo);
};
