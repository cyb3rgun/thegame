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

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FDoorSlotHitDelegate, ADoorSlot*, Slot, EDoorOccupant, Occupant, float, ExposureFraction);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FDoorSlotClosedDelegate, ADoorSlot*, Slot, EDoorOccupant, Occupant, bool, bWasHit);

/**
 *  A door frame with a swinging door panel and one occupant spawn point behind it.
 *  The game mode opens the slot with an occupant; the slot animates open, shows, closes,
 *  and reports hits and closings back through delegates.
 *  Placeholder visuals: engine basic shapes only.
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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* OccupantBody;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* OccupantHead;

protected:

	/** Yaw the panel swings to when fully open, positive swings away from the player side */
	UPROPERTY(EditAnywhere, Category="Door", meta = (ClampMin = 10, ClampMax = 170, Units = "Degrees"))
	float OpenAngle = 110.0f;

	EDoorState State = EDoorState::Closed;
	EDoorOccupant Occupant = EDoorOccupant::Empty;

	float OpenDuration = 0.4f;
	float ExposureWindow = 1.5f;
	float CloseDuration = 0.4f;

	/** Seconds spent in the current state */
	float StateElapsed = 0.0f;

	/** Panel open fraction, 0 closed to 1 fully open */
	float PanelAlpha = 0.0f;

	/** Open fraction at the moment closing started, so an early close swings back from where it was */
	float ClosingStartAlpha = 1.0f;

	bool bHitRegistered = false;

public:

	/** Fired once when a shot is accepted on the occupant */
	UPROPERTY(BlueprintAssignable, Category="Door")
	FDoorSlotHitDelegate OnSlotHit;

	/** Fired once when the door has fully closed again */
	UPROPERTY(BlueprintAssignable, Category="Door")
	FDoorSlotClosedDelegate OnSlotClosed;

public:

	ADoorSlot();

	virtual void Tick(float DeltaSeconds) override;

	/** Opens the door with the given occupant. Ignored unless the slot is closed. */
	UFUNCTION(BlueprintCallable, Category="Door")
	void Open(EDoorOccupant InOccupant, UMaterialInterface* OccupantMaterial, float InOpenDuration, float InExposureWindow, float InCloseDuration);

	/** Starts closing right away from the current panel position */
	UFUNCTION(BlueprintCallable, Category="Door")
	void ForceClose();

	/** Registers a hit on the occupant. Returns true if the hit counted. */
	UFUNCTION(BlueprintCallable, Category="Door")
	bool RegisterHit();

	UFUNCTION(BlueprintPure, Category="Door")
	EDoorState GetDoorState() const { return State; }

	UFUNCTION(BlueprintPure, Category="Door")
	EDoorOccupant GetOccupant() const { return Occupant; }

	/** True when the door is closed and can be opened again */
	UFUNCTION(BlueprintPure, Category="Door")
	bool IsAvailable() const { return State == EDoorState::Closed; }

	/** Returns 0 at the start of the exposure window, 1 at its end */
	UFUNCTION(BlueprintPure, Category="Door")
	float GetExposureFraction() const;

	//~ Begin IDoorRangeTarget
	virtual bool NotifyShot(UPrimitiveComponent* HitComponent, const FVector& HitLocation, AController* InstigatedBy) override;
	//~ End IDoorRangeTarget

protected:

	virtual void BeginPlay() override;

	void SetState(EDoorState NewState);
	void ApplyPanelAlpha(float Alpha);
	void SetOccupantVisible(bool bVisible);
	bool IsOccupantComponent(const UPrimitiveComponent* Component) const;
};
