// CYB3RGUN THEGAME. One door of the door module.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DoorTypes.h"
#include "DoorRangeTarget.h"
#include "DoorSlot.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UMaterialInterface;
class USoundBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FDoorSlotHitDelegate, ADoorSlot*, Slot, EDoorOccupant, Occupant, float, ExposureFraction);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FDoorSlotClosedDelegate, ADoorSlot*, Slot, EDoorOccupant, Occupant, bool, bWasHit);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDoorSlotDrawnDelegate, ADoorSlot*, Slot);

/**
 *  A door frame with a swinging door panel and one occupant spawn point behind it.
 *  The game mode opens the slot with an occupant; the slot animates open, shows, closes,
 *  and reports hits and closings back through delegates.
 *
 *  Placeholder visuals, engine shapes only, chosen so friend and foe read by silhouette:
 *  a hostile is a tall spike with a wide arm bar and a cone head that rises from a crouch
 *  before it is shootable, a friendly is a short round snowman that is up at once.
 */
UCLASS()
class CYB3RGUN_API ADoorSlot : public AActor, public IDoorRangeTarget
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USceneComponent* Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* FramePostLeft;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* FramePostRight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* FrameTop;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* BackWall;

	/** Pivot for the door panel, sits on the left post */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USceneComponent* Hinge;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* DoorPanel;

	/** Where the occupant stands, behind the door plane */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USceneComponent* SpawnPoint;

	/** Animated pivot for the occupant: rises during the telegraph, tips over when hit */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USceneComponent* OccupantRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* HostileBody;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* HostileArms;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* HostileHead;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* FriendlyBody;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* FriendlyHead;

protected:

	/** Yaw the panel swings to when fully open, positive swings away from the player side */
	UPROPERTY(EditAnywhere, Category="Door", meta = (ClampMin = 10, ClampMax = 170, Units = "Degrees"))
	float OpenAngle = 110.0f;

	/** Height scale a hostile starts at before it rises */
	UPROPERTY(EditAnywhere, Category="Door", meta = (ClampMin = 0.05, ClampMax = 1.0))
	float CrouchScale = 0.35f;

	/** Seconds the occupant takes to tip over after a hit */
	UPROPERTY(EditAnywhere, Category="Door", meta = (ClampMin = 0.05, Units = "s"))
	float HitReactionDuration = 0.3f;

	EDoorState State = EDoorState::Closed;
	FDoorOpenParams Params;

	/** Seconds spent in the current state */
	float StateElapsed = 0.0f;

	/** Panel open fraction, 0 closed to 1 fully open */
	float PanelAlpha = 0.0f;

	/** Open fraction at the moment closing started, so an early close swings back from where it was */
	float ClosingStartAlpha = 1.0f;

	/** Draw bonus fraction at the moment closing started, decays to zero with the panel */
	float BonusAtCloseStart = 0.0f;

	/** True once the occupant can be hit. Friendlies are drawn at once, hostiles after the telegraph. */
	bool bDrawn = false;

	/** State time at which the occupant became drawn */
	float DrawnAt = 0.0f;

	bool bHitRegistered = false;
	float HitReactionElapsed = 0.0f;

	/** False while a restart closes the door, so the closing is not scored */
	bool bReportNextClose = true;

public:

	/** Fired once when a shot is accepted on the occupant */
	UPROPERTY(BlueprintAssignable, Category="Door")
	FDoorSlotHitDelegate OnSlotHit;

	/** Fired once when the door has fully closed again */
	UPROPERTY(BlueprintAssignable, Category="Door")
	FDoorSlotClosedDelegate OnSlotClosed;

	/** Fired when a hostile finished rising and became shootable */
	UPROPERTY(BlueprintAssignable, Category="Door")
	FDoorSlotDrawnDelegate OnSlotDrawn;

public:

	ADoorSlot();

	virtual void Tick(float DeltaSeconds) override;

	/** Opens the door with the given occupant and timings. Ignored unless the slot is closed. */
	UFUNCTION(BlueprintCallable, Category="Door")
	void Open(const FDoorOpenParams& InParams);

	/** Starts closing right away from the current panel position. With bReportClose false the closing is not broadcast, used when a range restarts. */
	UFUNCTION(BlueprintCallable, Category="Door")
	void ForceClose(bool bReportClose = true);

	/** Registers a hit on the occupant. Returns true if the hit counted. */
	UFUNCTION(BlueprintCallable, Category="Door")
	bool RegisterHit();

	UFUNCTION(BlueprintPure, Category="Door")
	EDoorState GetDoorState() const { return State; }

	UFUNCTION(BlueprintPure, Category="Door")
	EDoorOccupant GetOccupant() const { return Params.Occupant; }

	/** True when the door is closed and can be opened again */
	UFUNCTION(BlueprintPure, Category="Door")
	bool IsAvailable() const { return State == EDoorState::Closed; }

	/** True while the occupant is up and can be hit */
	UFUNCTION(BlueprintPure, Category="Door")
	bool IsDrawn() const { return bDrawn && !bHitRegistered && (State == EDoorState::Showing); }

	/** Draw bonus fraction: rises from 0 to 1 over the exposure window, then falls back to 0 while the panel closes */
	UFUNCTION(BlueprintPure, Category="Door")
	float GetExposureFraction() const;

	/** World position to aim at for the occupant, roughly its centre of mass */
	UFUNCTION(BlueprintPure, Category="Door")
	FVector GetOccupantAimPoint() const;

	//~ Begin IDoorRangeTarget
	virtual bool NotifyShot(UPrimitiveComponent* HitComponent, const FVector& HitLocation, AController* InstigatedBy) override;
	//~ End IDoorRangeTarget

protected:

	virtual void BeginPlay() override;

	void SetState(EDoorState NewState);
	void ApplyPanelAlpha(float Alpha);
	void ShowOccupant(EDoorOccupant Occupant, UMaterialInterface* Material);
	void HideOccupant();
	void SetOccupantRise(float RiseAlpha);
	void ApplyHitReaction(float ReactionAlpha);
	void PlaySlotSound(USoundBase* Sound, float Pitch) const;
	bool IsOccupantComponent(const UPrimitiveComponent* Component) const;
};
