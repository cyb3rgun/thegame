// CYB3RGUN THEGAME. The pawn that rides a rail route. Its own distance scalar is the master clock of the ride.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "RailTrack.h"
#include "RailPawn.generated.h"

class UCapsuleComponent;
class UCameraComponent;
class USceneComponent;
class UNavigationInvokerComponent;
class URailAimComponent;
class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FRailBeatReachedDelegate, ARailTrack*, Track, int32, BeatIndex, const FRailBeat&, Beat);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FRailBeatClearedDelegate, ARailTrack*, Track, int32, BeatIndex, float, HeldSeconds);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FRailTrackEndedDelegate, ARailTrack*, FinishedTrack, ARailTrack*, NextTrack);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FRailRideFinishedDelegate, float, TotalDistance, float, Seconds);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FRailHealthChangedDelegate, float, Health, float, MaxHealth);

/**
 *  Rides ARailTrack segments (D-020). DistanceAlongSpline advances per tick at Speed and the pawn places itself
 *  with the spline transform at that distance. Beats fire when their distance is crossed; a holding beat stops
 *  the ride until ReleaseBeatHold is called. A finished segment hands over to the next one at distance zero.
 */
UCLASS()
class CYB3RGUN_API ARailPawn : public APawn
{
	GENERATED_BODY()

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UCapsuleComponent> Capsule;

	/** Parent of the camera. Offsets such as cover move this, never the pawn. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USceneComponent> CameraRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UCameraComponent> Camera;

	/** Grows the nav mesh around the rider so enemies can path to it without a level wide bake */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UNavigationInvokerComponent> NavInvoker;

	/** Screen space aiming and hitscan shots (D-019) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<URailAimComponent> Aim;

	/** Added to the local player while this pawn is possessed */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	TArray<TObjectPtr<UInputMappingContext>> MappingContexts;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	TObjectPtr<UInputAction> FireAction;

	/** Mouse deltas move the crosshair */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	TObjectPtr<UInputAction> MouseAimAction;

	/** Stick deflection moves the crosshair */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	TObjectPtr<UInputAction> StickAimAction;

	/** Viewport fraction the crosshair moves per unit of mouse input */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input", meta = (ClampMin = 0.0))
	float MouseAimSensitivity = 0.0015f;

	/** Viewport fractions per second the crosshair moves at full stick deflection */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input", meta = (ClampMin = 0.0))
	float StickAimSpeed = 0.9f;

	/** Segment the ride starts on. Empty picks the first track in the level. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Rail")
	TObjectPtr<ARailTrack> Track;

	/** Ride speed along the spline */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Rail", meta = (ClampMin = 0.0))
	float Speed = 450.0f;

	/** Distance along the current segment's spline. The master clock of the ride. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Rail")
	float DistanceAlongSpline = 0.0f;

	/** Start riding at BeginPlay after StartDelay */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Rail")
	bool bAutoStart = true;

	/** Seconds before an automatic start, gives the nav mesh around the rider time to build */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Rail", meta = (ClampMin = 0.0, Units = "s"))
	float StartDelay = 1.5f;

	/** Keep the camera level instead of following the spline's slope */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Rail")
	bool bFollowSplinePitch = false;

	/** Eye height above the spline */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Rail", meta = (Units = "cm"))
	float EyeHeight = 165.0f;

	/** Nav mesh generation radius around the rider */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Rail", meta = (ClampMin = 500.0, Units = "cm"))
	float NavInvokerRadius = 4000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Health", meta = (ClampMin = 1.0))
	float MaxHealth = 500.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Health")
	float Health = 0.0f;

	int32 NextBeatIndex = 0;
	int32 HeldBeatIndex = INDEX_NONE;
	float HoldStartTime = 0.0f;
	float RideStartTime = 0.0f;
	float RideDistance = 0.0f;
	bool bStarted = false;
	bool bFinished = false;
	bool bPausedByRequest = false;

	FTimerHandle StartTimer;

public:

	UPROPERTY(BlueprintAssignable, Category="Rail")
	FRailBeatReachedDelegate OnBeatReached;

	UPROPERTY(BlueprintAssignable, Category="Rail")
	FRailBeatClearedDelegate OnBeatCleared;

	/** A segment reached its end. NextTrack is null when the ride finishes. */
	UPROPERTY(BlueprintAssignable, Category="Rail")
	FRailTrackEndedDelegate OnTrackEnded;

	UPROPERTY(BlueprintAssignable, Category="Rail")
	FRailRideFinishedDelegate OnRideFinished;

	UPROPERTY(BlueprintAssignable, Category="Health")
	FRailHealthChangedDelegate OnHealthChanged;

public:

	ARailPawn();

	virtual void Tick(float DeltaSeconds) override;
	virtual float TakeDamage(float Damage, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	UFUNCTION(BlueprintCallable, Category="Rail")
	void StartRide();

	/** Stops the ride until Resume. Independent of beat holds. */
	UFUNCTION(BlueprintCallable, Category="Rail")
	void Pause();

	UFUNCTION(BlueprintCallable, Category="Rail")
	void Resume();

	UFUNCTION(BlueprintCallable, Category="Rail")
	void SetSpeed(float NewSpeed);

	/** Lets the ride continue past a holding beat. Ignored if that beat is not the one holding. */
	UFUNCTION(BlueprintCallable, Category="Rail")
	void ReleaseBeatHold(int32 BeatIndex);

	/** Moves the rider onto a segment at a distance, without firing beats before that distance */
	UFUNCTION(BlueprintCallable, Category="Rail")
	void SetTrack(ARailTrack* NewTrack, float StartDistance = 0.0f);

	UFUNCTION(BlueprintPure, Category="Rail")
	bool IsMoving() const;

	UFUNCTION(BlueprintPure, Category="Rail")
	float GetSpeed() const { return Speed; }

	UFUNCTION(BlueprintPure, Category="Rail")
	float GetDistanceAlongSpline() const { return DistanceAlongSpline; }

	UFUNCTION(BlueprintPure, Category="Rail")
	float GetRideDistance() const { return RideDistance; }

	UFUNCTION(BlueprintPure, Category="Rail")
	ARailTrack* GetTrack() const { return Track; }

	UFUNCTION(BlueprintPure, Category="Rail")
	int32 GetHeldBeatIndex() const { return HeldBeatIndex; }

	UFUNCTION(BlueprintPure, Category="Rail")
	bool IsFinished() const { return bFinished; }

	UFUNCTION(BlueprintPure, Category="Rail")
	bool IsPausedByRequest() const { return bPausedByRequest; }

	UFUNCTION(BlueprintPure, Category="Health")
	float GetHealth() const { return Health; }

	UCameraComponent* GetCamera() const { return Camera; }

	UFUNCTION(BlueprintPure, Category="Aim")
	URailAimComponent* GetAim() const { return Aim; }

	/** Pulls the trigger once through the aim component */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoFire();

protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void PawnClientRestart() override;

	void MouseAimInput(const FInputActionValue& Value);
	void StickAimInput(const FInputActionValue& Value);

	/** Extra hold conditions from subclasses and later features. The ride advances only when this is false. */
	virtual bool IsHeldByState() const { return false; }

	void Advance(float Delta);
	void ApplyTransform();
	void ReachEndOfTrack();
	float GetRideSeconds() const;
};
