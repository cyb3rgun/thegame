// CYB3RGUN THEGAME. One door of the door module.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DoorTypes.h"
#include "DoorRangeTarget.h"
#include "HitZoneSettings.h"
#include "DoorSlot.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class USkeletalMeshComponent;
class UAnimSequenceBase;
class UMaterialInterface;
class USoundBase;
class UStyleScoringComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FDoorSlotHitDelegate, ADoorSlot*, Slot, EDoorOccupant, Occupant, EHitZone, Zone, float, ExposureFraction);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FDoorSlotClosedDelegate, ADoorSlot*, Slot, EDoorOccupant, Occupant, bool, bWasHit);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDoorSlotDrawnDelegate, ADoorSlot*, Slot);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FDoorSlotDisarmedDelegate, ADoorSlot*, Slot, EHitZone, Zone, float, ExposureFraction);

/**
 *  A door frame with a swinging door panel and one occupant spawn point behind it.
 *  The game mode opens the slot with an occupant; the slot animates open, shows, closes,
 *  and reports hits and closings back through delegates.
 *
 *  Friend and foe read without colour (D-013, D-035). A hostile is the taller mannequin in dark
 *  gunmetal that draws a pistol during the telegraph and then aims it at the player; a friendly
 *  is the smaller mannequin in light matte paint, at ease with empty hands. The physics asset
 *  bodies are what shots hit, and the pistol is a target of its own: the hit zone decides the
 *  reaction and the score (D-049), a shot on the pistol or its arm disarms the hostile (D-050).
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

	/** Pivot for the occupant bodies, tips over when hit without any animation to fall with */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USceneComponent* OccupantRoot;

	/** Carries the hostile body and its pistol. A hostage taker moves it behind the hostage. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USceneComponent* HostileRoot;

	/** Visible hostile body, its physics asset bodies are the hit target */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* HostileMesh;

	/** The hostile's pistol, in its right hand. A target of its own while the hostile can shoot. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* HostileWeapon;

	/** Visible friendly body, the hostage of a hostage taker, its physics asset bodies are the hit target */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* FriendlyMesh;

protected:

	/** Yaw the panel swings to when fully open. The panel swings out towards the player, so its path never crosses the
	 *  alcove the occupant stands in (D-076). */
	UPROPERTY(EditAnywhere, Category="Door", meta = (ClampMin = 10, ClampMax = 170, Units = "Degrees"))
	float OpenAngle = 110.0f;

	/** Sideways margin added to the occupant's reach before the closing panel counts as covering it */
	UPROPERTY(EditAnywhere, Category="Door", meta = (ClampMin = 0.0, Units = "cm"))
	float OccupantCoverMargin = 6.0f;

	/** Size of the hostile body against the mannequin, its physics asset scales with it */
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

	/** How far a hostage taker's aim point sits out from its head bone, towards the side that shows past the hostage */
	UPROPERTY(EditAnywhere, Category="Door|Bodies", meta = (ClampMin = 0.0, Units = "cm"))
	float TakerAimSideOffset = 8.0f;

	/** Played across the telegraph and stretched to its length: the hostile draws its pistol */
	UPROPERTY(EditAnywhere, Category="Door|Bodies")
	TObjectPtr<UAnimSequenceBase> HostileDrawAnimation;

	/** Looped once the hostile is drawn */
	UPROPERTY(EditAnywhere, Category="Door|Bodies")
	TObjectPtr<UAnimSequenceBase> HostileAimAnimation;

	/** Looped while a friendly shows, and by a disarmed hostile when the hit zone settings have no disarmed idle */
	UPROPERTY(EditAnywhere, Category="Door|Bodies")
	TObjectPtr<UAnimSequenceBase> FriendlyIdleAnimation;

	/** Played on a body that falls when the hit zone settings have no fall for it. Without either the occupant tips over as a whole. */
	UPROPERTY(EditAnywhere, Category="Door|Bodies")
	TObjectPtr<UAnimSequenceBase> HitAnimation;

	UPROPERTY(EditAnywhere, Category="Door|Bodies", meta = (ClampMin = 0.1, ClampMax = 4.0))
	float HitAnimationRate = 1.5f;

	/** Seconds the occupant takes to tip over after a hit, when there is no animation to fall with */
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

	/** Open fraction at and below which the panel covers the whole occupant as seen from the front. The occupant shares the
	 *  door's timeline (D-076): it is shown while the closed panel still covers it and hidden once the closing panel covers
	 *  it again, so it never shows past the panel's edge and never pops into view. */
	float OccupantCoverAlpha = 0.0f;

	/** True while the occupant of the current opening is shown */
	bool bOccupantShown = false;

	/** Draw bonus fraction at the moment closing started, decays to zero with the panel */
	float BonusAtCloseStart = 0.0f;

	/** True once the occupant can be hit. Friendlies are drawn at once, hostiles after the telegraph. */
	bool bDrawn = false;

	/** State time at which the occupant became drawn */
	float DrawnAt = 0.0f;

	bool bHitRegistered = false;
	float HitReactionElapsed = 0.0f;

	/** The hostile lost its pistol to a shot and stands unarmed: out of the fight like a hit one, never escaped (D-050) */
	bool bDisarmed = false;

	/** True while the occupant tips over as a whole, with no animation to fall with */
	bool bTipOver = false;

	/** How a hostage taker's door went: the taker is down or disarmed, or the hostage was hit */
	bool bTakerDown = false;
	bool bHostageDown = false;

	/** World time the hit or the disarm registered, a follow-up hit shortly after still reaches the style record */
	double HitRegisteredAt = 0.0;

	/** Closes the door once the pair window after a hit has passed */
	FTimerHandle HitCloseTimer;

	/** False while a restart closes the door, so the closing is not scored */
	bool bReportNextClose = true;

public:

	/** Fired once when a shot is accepted on the occupant, with the zone it landed in */
	UPROPERTY(BlueprintAssignable, Category="Door")
	FDoorSlotHitDelegate OnSlotHit;

	/** Fired once when the door has fully closed again */
	UPROPERTY(BlueprintAssignable, Category="Door")
	FDoorSlotClosedDelegate OnSlotClosed;

	/** Fired when a hostile finished its draw and became shootable */
	UPROPERTY(BlueprintAssignable, Category="Door")
	FDoorSlotDrawnDelegate OnSlotDrawn;

	/** Fired once when a shot on the pistol, or on the arm holding it, disarms the hostile or the hostage taker */
	UPROPERTY(BlueprintAssignable, Category="Door")
	FDoorSlotDisarmedDelegate OnSlotDisarmed;

public:

	ADoorSlot();

	virtual void Tick(float DeltaSeconds) override;

	/** Opens the door with the given occupant and timings. Ignored unless the slot is closed. */
	UFUNCTION(BlueprintCallable, Category="Door")
	void Open(const FDoorOpenParams& InParams);

	/** Starts closing right away from the current panel position. With bReportClose false the closing is not broadcast, used when a range restarts. */
	UFUNCTION(BlueprintCallable, Category="Door")
	void ForceClose(bool bReportClose = true);

	/** Registers a hit on the occupant without a hit zone. Returns true if the hit counted. */
	UFUNCTION(BlueprintCallable, Category="Door")
	bool RegisterHit();

	UFUNCTION(BlueprintPure, Category="Door")
	EDoorState GetDoorState() const { return State; }

	UFUNCTION(BlueprintPure, Category="Door")
	EDoorOccupant GetOccupant() const { return Params.Occupant; }

	/** True when the door is closed and can be opened again */
	UFUNCTION(BlueprintPure, Category="Door")
	bool IsAvailable() const { return State == EDoorState::Closed; }

	/** True while the occupant is up, armed where it has a weapon, and can be hit */
	UFUNCTION(BlueprintPure, Category="Door")
	bool IsDrawn() const { return bDrawn && !bHitRegistered && !bDisarmed && (State == EDoorState::Showing); }

	/** Draw bonus fraction: rises from 0 to 1 over the exposure window, then falls back to 0 while the panel closes */
	UFUNCTION(BlueprintPure, Category="Door")
	float GetExposureFraction() const;

	/** World position to aim at for the occupant, its chest */
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

	/** World position of a bone of the shown occupant, or of a hostage taker's hostage: the middle of its physics body. False without that bone. */
	bool GetOccupantBonePoint(FName Bone, bool bHostage, FVector& OutPoint) const;

	/** World position of the pistol of a hostile or a hostage taker while it holds it. False otherwise. */
	bool GetOccupantWeaponPoint(FVector& OutPoint) const;

	//~ Begin IDoorRangeTarget
	virtual bool NotifyShot(const FHitResult& Hit, const FVector& ShotDirection, AController* InstigatedBy) override;
	//~ End IDoorRangeTarget

protected:

	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

	/** Puts the look materials on the frame, panel and back wall */
	void ApplyLookMaterials();

	/** Anim instance and hit target collision of the bodies, once when play begins */
	void SetupTargets();

	/** Levels saved before the bodies became the hit targets can still carry the old hit volumes, which must never catch a shot */
	void DisableStaleHitVolumes();

	void SetState(EDoorState NewState);
	void ApplyPanelAlpha(float Alpha);
	void ShowOccupant(EDoorOccupant Occupant, UMaterialInterface* Material);
	void HideOccupant();

	/** The open fraction at which the panel covers everything the shown occupant reaches, from its bounds in the slot's space */
	float ComputeOccupantCoverAlpha() const;
	void ApplyBodyScale();

	/** Switches the bodies between shootable and not; a shootable body keeps its pose fresh off screen too */
	void SetBodiesShootable(bool bHostile, bool bFriendly);

	/** The pistol is a target while its holder can shoot: drawn, not down, not disarmed, the door not shut */
	void UpdateWeaponTarget();

	/** Holds the hostile at the first frame of the draw while the door opens */
	void PoseHostileReady();

	/** Starts the draw so it ends exactly when the telegraph does */
	void StartHostileDraw();

	void PlayBodyLoop(USkeletalMeshComponent* Body, UAnimSequenceBase* Animation, float BlendTime = 0.0f);
	USkeletalMeshComponent* GetShownBody() const;
	void ApplyHitReaction(float ReactionAlpha);
	void PlaySlotSound(USoundBase* Sound, float Pitch) const;
	bool IsOccupantComponent(const UPrimitiveComponent* Component) const;

	/** Registers a hit in a zone on a hostile or a friendly: it counts once and the body reacts to its zone */
	bool RegisterZoneHit(const FHitZoneResult& Zone);

	/** A shot on the pistol or the arm holding it: the pistol flies, the hostile stays up without it (D-050) */
	void DisarmHostile(const FHitZoneResult& Zone, const FHitResult& Hit, const FVector& ShotDirection);

	/** A shot on a hostage taker's door: the taker's showing strip frees the hostage, the hostage is the full penalty */
	bool NotifyHostageShot(const FHitResult& Hit, const FVector& ShotDirection, const FHitZoneResult& Zone, UStyleScoringComponent* Style);

	/** A hostile or a hostage taker goes down from a hit in this zone: a limb reacts first, then the body falls */
	void PlayDown(USkeletalMeshComponent* Body, const FHitZoneResult& Zone);

	/** A friendly or a hostage hit in this zone: it falls from a killing zone and only reacts to any other */
	void PlayInjured(USkeletalMeshComponent* Body, const FHitZoneResult& Zone);

	/** The fall from the side of the shot, else the door's own hit animation, else the whole occupant tips over */
	void PlayFallOn(USkeletalMeshComponent* Body, EHitDirection Direction);

	/** Plays the door's own hit animation on one body */
	void PlayHitOn(USkeletalMeshComponent* Body);

	/** Zone a follow-up shot reports to the style record: its own for another pellet of the shot that counted, none for a later shot */
	EHitZone GetFollowUpZone(const FHitZoneResult& Zone) const;

	/** Shuts the door once the controlled pair window after a hit has passed */
	void CloseAfterHit();

	/** Moves the hostile set behind the hostage for a hostage taker, or back to the front */
	void ApplyHostileLayout(bool bTaker);

	/** Tells the threat subsystem how close the occupant is to shooting: rising while a hostile draws, full while it can fire */
	void ReportThreat() const;

};
