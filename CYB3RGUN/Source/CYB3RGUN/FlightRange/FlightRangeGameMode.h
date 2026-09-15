// CYB3RGUN THEGAME. Game mode running the flight range: one countdown, one run, one score (D-077).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "HitZoneSettings.h"
#include "StyleScenario.h"
#include "FlightSpawnDirector.h"
#include "FlightRangeGameMode.generated.h"

class AController;
class AFlightScoringProp;
class AFlightTarget;
class UFlightGalleryControls;
class ULogoCrosshairWidget;
class UFlightRangeHUD;
class UFlightRangeSettings;
class UStyleHUDWidget;
class UStyleSettings;
class UWeaponDefinition;

UENUM(BlueprintType)
enum class EFlightRoundState : uint8
{
	/** The player is in place, the countdown has not started */
	GetReady,
	Running,
	Finished
};

/** Numbers of one round, for the HUD and the end of run summary */
USTRUCT(BlueprintType)
struct FFlightRangeStats
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	int32 FinalScore = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 TargetsLaunched = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 TargetsHit = 0;

	/** Targets that crossed the field untouched */
	UPROPERTY(BlueprintReadOnly)
	int32 TargetsEscaped = 0;

	/** Trigger pulls, the pellets of one pull are one shot */
	UPROPERTY(BlueprintReadOnly)
	int32 ShotsFired = 0;

	/** Shots that hit nothing */
	UPROPERTY(BlueprintReadOnly)
	int32 Misses = 0;

	/** Share of shots that landed on a target, 0 to 1 */
	UPROPERTY(BlueprintReadOnly)
	float Accuracy = 0.0f;

	/** Points of the best single hit */
	UPROPERTY(BlueprintReadOnly)
	int32 BestHit = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 LaunchesLeft = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 LaunchesRight = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 LaunchesCover = 0;

	/** Points the windmill, the scarecrow and other bonus props paid */
	UPROPERTY(BlueprintReadOnly)
	int32 BonusPoints = 0;

	/** Shots that landed on a penalty prop, the sign, and the points they cost */
	UPROPERTY(BlueprintReadOnly)
	int32 PenaltyHits = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 PenaltyPoints = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFlightRangeTimeDelegate, int32, SecondsLeft);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFlightRangeScoreDelegate, int32, Score, int32, Delta);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFlightRangeHitDelegate, int32, Points, FVector, Location);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFlightRangeStateDelegate, EFlightRoundState, State);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFlightRangeFinishedDelegate, const FFlightRangeStats&, Stats);

/**
 *  The arcade range: a gallery (D-078), the view fixed forward with a free crosshair that slides the player along the scene
 *  at the screen edges, a spawn director keeps targets crossing the field, every hit scores by the target's distance, speed
 *  and size, props in the scene pay a bonus or cost a penalty, and when the countdown runs out the round ends with a
 *  summary. No waves, no telegraphs, no hostages, no difficulty ramp. The style meter, combo and controlled pairs run as everywhere.
 */
UCLASS()
class CYB3RGUN_API AFlightRangeGameMode : public AGameModeBase, public IStyleScenario
{
	GENERATED_BODY()

public:

	AFlightRangeGameMode();

	UFUNCTION(BlueprintCallable, Category="Flight Range")
	void StartRound();

	/** Ends the round now, as if the time had run out */
	UFUNCTION(BlueprintCallable, Category="Flight Range")
	void FinishRound();

	UFUNCTION(BlueprintPure, Category="Flight Range")
	EFlightRoundState GetRoundState() const { return RoundState; }

	UFUNCTION(BlueprintPure, Category="Flight Range")
	int32 GetScore() const { return Score; }

	/** Whole seconds left, rounded up */
	UFUNCTION(BlueprintPure, Category="Flight Range")
	int32 GetSecondsLeft() const;

	/** Seconds until the countdown starts while getting ready */
	UFUNCTION(BlueprintPure, Category="Flight Range")
	float GetReadySecondsLeft() const;

	/** The round so far, with the shots of the style record */
	UFUNCTION(BlueprintPure, Category="Flight Range")
	FFlightRangeStats GetStats() const;

	const UFlightRangeSettings* GetSettings() const;

	AFlightSpawnDirector* GetDirector() const { return Director; }

	UFlightGalleryControls* GetGallery() const { return Gallery; }

	/** A scene prop was shot while the round runs: its points go on the score, a bonus or a penalty */
	void AddPropPoints(AFlightScoringProp* Prop, int32 Points, const FVector& Location);

	/** Sets the time left, for tests */
	void SetSecondsLeft(float Seconds) { TimeLeft = FMath::Max(Seconds, 0.0f); }

	//~ Begin IStyleScenario
	virtual const UStyleSettings* GetStyleSettings() const override { return RoundStyle ? RoundStyle.Get() : StyleSettings.Get(); }
	//~ End IStyleScenario

	UPROPERTY(BlueprintAssignable, Category="Flight Range")
	FFlightRangeTimeDelegate OnTimeChanged;

	UPROPERTY(BlueprintAssignable, Category="Flight Range")
	FFlightRangeScoreDelegate OnScoreChanged;

	UPROPERTY(BlueprintAssignable, Category="Flight Range")
	FFlightRangeHitDelegate OnTargetScored;

	UPROPERTY(BlueprintAssignable, Category="Flight Range")
	FFlightRangeStateDelegate OnRoundStateChanged;

	UPROPERTY(BlueprintAssignable, Category="Flight Range")
	FFlightRangeFinishedDelegate OnRoundFinished;

protected:

	/** Tunables for this range. Falls back to class defaults when unset. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Flight Range")
	TObjectPtr<UFlightRangeSettings> Settings;

	/** HUD widget created for the local player */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Flight Range")
	TSubclassOf<UFlightRangeHUD> RangeHUDClass;

	/** Style values of this scenario, empty uses the project default */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Flight Range")
	TObjectPtr<UStyleSettings> StyleSettings;

	/** Start the countdown on its own once the player is in place */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Flight Range")
	bool bAutoStart = true;

	UPROPERTY(Transient)
	TObjectPtr<AFlightSpawnDirector> Director;

	UPROPERTY(Transient)
	TObjectPtr<UFlightGalleryControls> Gallery;

	UPROPERTY(Transient)
	TObjectPtr<UFlightRangeHUD> HUD;

	UPROPERTY(Transient)
	TObjectPtr<UStyleHUDWidget> StyleHUD;

	/** Hidden while the summary shows, so it never draws over the result */
	UPROPERTY(Transient)
	TObjectPtr<ULogoCrosshairWidget> Crosshair;

	/** The style values of the round: this scenario's or the project's, with Overclock locked, since slowed time would stretch the countdown */
	UPROPERTY(Transient)
	TObjectPtr<UStyleSettings> RoundStyle;

	/** Copies of loadout weapons carrying this mode's magazine and reload */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UWeaponDefinition>> RoundWeapons;

	FFlightRangeStats Stats;
	EFlightRoundState RoundState = EFlightRoundState::GetReady;
	FTransform FieldTransform;
	FVector ShooterLocation = FVector::ZeroVector;
	float TimeLeft = 0.0f;
	double ReadyAt = 0.0;
	int32 Score = 0;
	int32 LastBroadcastSeconds = -1;
	FTimerHandle StartTimer;

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

	void CreateHUD(APlayerController* Player);

	/** Plants the player on the gallery: no movement, the fixed view and the free crosshair, the loadout, and the field taken from where it stands */
	void SetupPlayer();

	/** The weapon to hand out: the definition itself, or a copy with the overrides of the settings */
	const UWeaponDefinition* MakeRoundWeapon(const UWeaponDefinition* Weapon);

	void SetRoundState(EFlightRoundState NewState);
	void HandleTargetLaunched(AFlightTarget* Target, EFlightSource Source);
	void HandleTargetHit(AFlightTarget* Target, EHitZone Zone, AController* InstigatedBy);
	void HandleTargetDone(AFlightTarget* Target, bool bWasHit);
	void PlaySound2D(class USoundBase* Sound) const;
};
