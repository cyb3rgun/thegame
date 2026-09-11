// CYB3RGUN THEGAME. One door of the door module.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DoorTypes.h"
#include "DoorRangeTarget.h"
#include "DoorSlot.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class USkeletalMeshComponent;
class UAnimSequenceBase;
class UMaterialInterface;
class USoundBase;
class UStyleScoringComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FDoorSlotHitDelegate, ADoorSlot*, Slot, EDoorOccupant, Occupant, float, ExposureFraction);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FDoorSlotClosedDelegate, ADoorSlot*, Slot, EDoorOccupant, Occupant, bool, bWasHit);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDoorSlotDrawnDelegate, ADoorSlot*, Slot);

/**
 *  A door frame with a swinging door panel and one occupant spawn point behind it.
 *  The game mode opens the slot with an occupant; the slot animates open, shows, closes,
 *  and reports hits and closings back through delegates.
 *
 *  Friend and foe read without colour (D-013, D-035). A hostile is the taller mannequin in dark
 *  gunmetal that draws a pistol during the telegraph and then aims it at the player; a friendly
 *  is the smaller mannequin in light matte paint, at ease with empty hands. Hidden engine shapes
 *  around each body are what shots hit.
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

	/** Pivot for the occupant bodies and their hit volumes, tips over when hit without a hit animation */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USceneComponent* OccupantRoot;

	/** Carries the hostile body, its pistol and its hit volumes. A hostage taker moves it behind the hostage. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USceneComponent* HostileRoot;

	/** Visible hostile body */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* HostileMesh;

	/** The hostile's pistol, in its right hand */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* HostileWeapon;

	/** Visible friendly body */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* FriendlyMesh;

	/** Hit volumes around the bodies. Never drawn, only their collision follows the occupant. */
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

	/** Size of the hostile body against the mannequin. The hit volumes are sized for the defaults. */
	UPROPERTY(EditAnywhere, Category="Door|Bodies", meta = (ClampMin = 0.5, ClampMax = 1.5))
	float HostileBodyScale = 1.05f;

	/** Size of the friendly body, smaller than the hostile so size reads as well */
	UPROPERTY(EditAnywhere, Category="Door|Bodies", meta = (ClampMin = 0.5, ClampMax = 1.5))
	float FriendlyBodyScale = 0.9f;

	/** Where a hostage taker stands against its hostage: behind it and to its gun side, so only a strip of it shows */
	UPROPERTY(EditAnywhere, Category="Door|Bodies", meta = (Units = "cm"))
	FVector TakerOffset = FVector(-30.0f, 20.0f, 0.0f);

	/** Size of a hostage taker's body, close to the hostage's so it can hide behind it */
	UPROPERTY(EditAnywhere, Category="Door|Bodies", meta = (ClampMin = 0.5, ClampMax = 1.5))
	float TakerBodyScale = 0.88f;

	/** Played across the telegraph and stretched to its length: the hostile draws its pistol */
	UPROPERTY(EditAnywhere, Category="Door|Bodies")
	TObjectPtr<UAnimSequenceBase> HostileDrawAnimation;

	/** Looped once the hostile is drawn */
	UPROPERTY(EditAnywhere, Category="Door|Bodies")
	TObjectPtr<UAnimSequenceBase> HostileAimAnimation;

	/** Looped while a friendly shows */
	UPROPERTY(EditAnywhere, Category="Door|Bodies")
	TObjectPtr<UAnimSequenceBase> FriendlyIdleAnimation;

	/** Played on the body that was hit. Without one the occupant tips over as a whole. */
	UPROPERTY(EditAnywhere, Category="Door|Bodies")
	TObjectPtr<UAnimSequenceBase> HitAnimation;

	UPROPERTY(EditAnywhere, Category="Door|Bodies", meta = (ClampMin = 0.1, ClampMax = 4.0))
	float HitAnimationRate = 1.5f;

	/** Seconds the occupant takes to tip over after a hit, when there is no hit animation */
	UPROPERTY(EditAnywhere, Category="Door", meta = (ClampMin = 0.05, Units = "s"))
	float HitReactionDuration = 0.3f;

	/** Surface of the frame posts and the lintel, darkest of the three so the opening reads */
	UPROPERTY(EditAnywhere, Category="Door|Look")
	TObjectPtr<UMaterialInterface> FrameMaterial;

	/** Surface of the swinging panel, between frame and wall in value */
	UPROPERTY(EditAnywhere, Category="Door|Look")
	TObjectPtr<UMaterialInterface> PanelMaterial;

	/** Surface of the wall behind the occupant */
	UPROPERTY(EditAnywhere, Category="Door|Look")
	TObjectPtr<UMaterialInterface> WallMaterial;

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

	/** How a hostage taker's door went: the taker is down, or the hostage was hit */
	bool bTakerDown = false;
	bool bHostageDown = false;

	/** World time the hit registered, a follow-up hit shortly after still reaches the style record */
	double HitRegisteredAt = 0.0;

	/** Closes the door once the pair window after a hit has passed */
	FTimerHandle HitCloseTimer;

	/** False while a restart closes the door, so the closing is not scored */
	bool bReportNextClose = true;

public:

	/** Fired once when a shot is accepted on the occupant */
	UPROPERTY(BlueprintAssignable, Category="Door")
	FDoorSlotHitDelegate OnSlotHit;

	/** Fired once when the door has fully closed again */
	UPROPERTY(BlueprintAssignable, Category="Door")
	FDoorSlotClosedDelegate OnSlotClosed;

	/** Fired when a hostile finished its draw and became shootable */
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

	/** World position of the occupant's head */
	UFUNCTION(BlueprintPure, Category="Door")
	FVector GetOccupantHeadPoint() const;

	/** Counts the hostile parts that are not under the hostile root: slots saved before it existed carry them on the occupant
	 *  root. With bFix it moves them there and marks the slot modified, so the level can be saved with the right attachment. */
	int32 RepairHostileSet(bool bFix);

	/** World position of a hostage taker's hostage, its chest */
	UFUNCTION(BlueprintPure, Category="Door")
	FVector GetHostageAimPoint() const;

	//~ Begin IDoorRangeTarget
	virtual bool NotifyShot(UPrimitiveComponent* HitComponent, const FVector& HitLocation, AController* InstigatedBy) override;
	//~ End IDoorRangeTarget

protected:

	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

	/** Puts the look materials on the frame, panel and back wall */
	void ApplyLookMaterials();

	void SetState(EDoorState NewState);
	void ApplyPanelAlpha(float Alpha);
	void ShowOccupant(EDoorOccupant Occupant, UMaterialInterface* Material);
	void HideOccupant();
	void ApplyBodyScale();

	/** Holds the hostile at the first frame of the draw while the door opens */
	void PoseHostileReady();

	/** Starts the draw so it ends exactly when the telegraph does */
	void StartHostileDraw();

	void PlayBodyLoop(USkeletalMeshComponent* Body, UAnimSequenceBase* Animation);
	USkeletalMeshComponent* GetShownBody() const;
	void ApplyHitReaction(float ReactionAlpha);
	void PlaySlotSound(USoundBase* Sound, float Pitch) const;
	bool IsOccupantComponent(const UPrimitiveComponent* Component) const;

	/** A shot on a hostage taker's door: the taker's showing strip frees the hostage, the hostage is the full penalty */
	bool NotifyHostageShot(UPrimitiveComponent* HitComponent, UStyleScoringComponent* Style);

	/** Plays the hit animation on one body */
	void PlayHitOn(USkeletalMeshComponent* Body);

	/** Shuts the door once the controlled pair window after a hit has passed */
	void CloseAfterHit();

	/** Moves the hostile set behind the hostage for a hostage taker, or back to the front */
	void ApplyHostileLayout(bool bTaker);

	/** Tells the threat subsystem how close the occupant is to shooting: rising while a hostile draws, full while it can fire */
	void ReportThreat() const;

};
