// CYB3RGUN THEGAME. A rail set piece: a hostile machine holding a hostage in front of it.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DoorRangeTarget.h"
#include "HitZoneSettings.h"
#include "HostageTaker.generated.h"

class USceneComponent;
class USkeletalMeshComponent;
class UAnimSequenceBase;
class UMaterialInterface;

/** How a hostage taker set piece ended */
UENUM(BlueprintType)
enum class EHostageOutcome : uint8
{
	/** The taker was hit or disarmed and the hostage is free */
	Rescued,
	/** The hostage was hit */
	HostageHit,
	/** Nobody fired in time and the taker backed away with the hostage */
	Escaped
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHostageResolvedDelegate, AHostageTaker*, Taker, EHostageOutcome, Outcome);

/**
 *  Waits hidden until a rail beat reveals it, then slides out from its hiding offset holding the hostage in front.
 *  Only a strip of the taker shows beside the hostage: any hit there frees the hostage, a hit on the hostage is the full
 *  style penalty. The physics asset bodies of both are what shots hit, and the taker's pistol is a target of its own:
 *  a shot on it, or on the arm holding it, disarms the taker and frees the hostage (D-049, D-050). Friend and foe read
 *  without colour, as on the door range: the taker is the dark gunmetal mannequin with a pistol, the hostage the
 *  lighter one with empty hands.
 */
UCLASS()
class CYB3RGUN_API AHostageTaker : public AActor, public IDoorRangeTarget
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USceneComponent* Root;

	/** Slides from the hiding offset to the root during the reveal, carries both bodies */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USceneComponent* Group;

	/** Carries the taker's body and its pistol, behind the hostage and scaled to its size */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USceneComponent* TakerRoot;

	/** The taker's body, its physics asset bodies are the hit target */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* TakerMesh;

	/** The taker's pistol, in its right hand. A target of its own while the taker can shoot. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* TakerWeapon;

	/** The hostage's body, its physics asset bodies are the hit target */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* HostageMesh;

protected:

	/** Where the taker stands against its hostage: behind it and to its gun side, so only a strip of it shows */
	UPROPERTY(EditAnywhere, Category="Hostage", meta = (Units = "cm"))
	FVector TakerOffset = FVector(-30.0f, 20.0f, 0.0f);

	/** Size of the taker's body, close to the hostage's so it can hide behind it */
	UPROPERTY(EditAnywhere, Category="Hostage", meta = (ClampMin = 0.5, ClampMax = 1.5))
	float TakerBodyScale = 0.88f;

	/** How far the taker's aim point sits out from its head bone, towards the side that shows past the hostage */
	UPROPERTY(EditAnywhere, Category="Hostage", meta = (ClampMin = 0.0, Units = "cm"))
	float TakerAimSideOffset = 8.0f;

	/** Where the pair waits before the reveal, in actor space, for example behind a wall edge */
	UPROPERTY(EditAnywhere, Category="Reveal", meta = (Units = "cm"))
	FVector HiddenOffset = FVector(0.0f, -160.0f, 0.0f);

	/** Seconds the pair takes to slide out, and to back away again when the taker gets away */
	UPROPERTY(EditAnywhere, Category="Reveal", meta = (ClampMin = 0.05, Units = "s"))
	float RevealSeconds = 0.6f;

	/** Seconds after the reveal before the taker backs away with its hostage */
	UPROPERTY(EditAnywhere, Category="Reveal", meta = (ClampMin = 0.5, Units = "s"))
	float TimeLimit = 6.0f;

	/** Seconds the outcome stays in view before the set piece reports it and the ride moves on */
	UPROPERTY(EditAnywhere, Category="Reveal", meta = (ClampMin = 0.0, Units = "s"))
	float OutcomeHoldSeconds = 1.2f;

	UPROPERTY(EditAnywhere, Category="Look")
	TObjectPtr<UMaterialInterface> TakerMaterial;

	UPROPERTY(EditAnywhere, Category="Look")
	TObjectPtr<UMaterialInterface> HostageMaterial;

	/** Looped by the taker, aiming past its hostage */
	UPROPERTY(EditAnywhere, Category="Animation")
	TObjectPtr<UAnimSequenceBase> TakerAimAnimation;

	/** Looped by the hostage, and by a disarmed taker when the hit zone settings have no disarmed idle */
	UPROPERTY(EditAnywhere, Category="Animation")
	TObjectPtr<UAnimSequenceBase> HostageIdleAnimation;

	/** Played on a body that falls when the hit zone settings have no fall for it */
	UPROPERTY(EditAnywhere, Category="Animation")
	TObjectPtr<UAnimSequenceBase> HitAnimation;

	UPROPERTY(EditAnywhere, Category="Animation", meta = (ClampMin = 0.1, ClampMax = 4.0))
	float HitAnimationRate = 1.5f;

	bool bActive = false;
	bool bResolved = false;

	/** The taker is down, or disarmed, and the hostage free */
	bool bTakerDown = false;
	bool bHostageDown = false;

	/** The taker lost its pistol to a shot and stands unarmed (D-050) */
	bool bTakerDisarmed = false;

	/** 0 hidden to 1 fully out, and the way it moves: 1 revealing, -1 backing away */
	float RevealAlpha = 0.0f;
	float RevealDirection = 1.0f;

	double TakerDownAt = 0.0;
	EHostageOutcome PendingOutcome = EHostageOutcome::Escaped;

	FTimerHandle TimeLimitTimer;
	FTimerHandle OutcomeTimer;

public:

	/** Fired once when the set piece is over */
	UPROPERTY(BlueprintAssignable, Category="Hostage")
	FHostageResolvedDelegate OnResolved;

public:

	AHostageTaker();

	virtual void Tick(float DeltaSeconds) override;

	/** Reveals the pair and starts the time limit. Ignored while it is out, a resolved set piece can be revealed again. */
	UFUNCTION(BlueprintCallable, Category="Hostage")
	void Activate();

	/** True from the reveal until the outcome is reported */
	UFUNCTION(BlueprintPure, Category="Hostage")
	bool IsActive() const { return bActive && !bResolved; }

	/** World point on the part of the taker's head that shows beside the hostage */
	UFUNCTION(BlueprintPure, Category="Hostage")
	FVector GetTakerAimPoint() const;

	/** World point on the hostage's chest */
	UFUNCTION(BlueprintPure, Category="Hostage")
	FVector GetHostageAimPoint() const;

	/** World point on a bone of the taker, or of the hostage: the middle of its physics body. False without that bone. */
	bool GetBonePoint(FName Bone, bool bHostage, FVector& OutPoint) const;

	/** World point on the taker's pistol while it holds it. False once it is gone. */
	bool GetWeaponPoint(FVector& OutPoint) const;

	//~ Begin IDoorRangeTarget
	virtual bool NotifyShot(const FHitResult& Hit, const FVector& ShotDirection, AController* InstigatedBy) override;
	//~ End IDoorRangeTarget

protected:

	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

	/** Anim instance and hit target collision of the bodies, once when play begins */
	void SetupTargets();

	/** Set pieces saved before the bodies became the hit targets can still carry the old hit volumes, which must never catch a shot */
	void DisableStaleHitVolumes();

	void ApplyLayout();

	/** Shows or hides the pair; the pistol goes back into the hand either way */
	void SetShown(bool bShown);

	/** Hit target collision of both bodies, and of the pistol while the taker can shoot with it */
	void SetBodiesShootable(bool bShootable);

	void PlayLoop(USkeletalMeshComponent* Body, UAnimSequenceBase* Animation, float BlendTime = 0.0f);

	/** The taker goes down from a hit in this zone: a limb reacts first, then the body falls */
	void PlayDown(USkeletalMeshComponent* Body, const FHitZoneResult& Zone);

	/** The hostage is hit in this zone: it falls from a killing zone and only reacts to any other */
	void PlayInjured(USkeletalMeshComponent* Body, const FHitZoneResult& Zone);

	/** The fall from the side of the shot, else the set piece's own hit animation */
	void PlayFallOn(USkeletalMeshComponent* Body, EHitDirection Direction);

	void PlayHitOn(USkeletalMeshComponent* Body);

	/** Holds the outcome in view, then reports it. A hostage hit outweighs a rescue that came first. */
	void ScheduleOutcome(EHostageOutcome Outcome);
	void ReportOutcome();
	void HandleTimeLimit();
};
