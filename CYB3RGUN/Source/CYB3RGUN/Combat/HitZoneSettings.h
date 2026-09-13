// CYB3RGUN THEGAME. Hit zones: which bone is head, torso, limb or weapon, and what a hit there does (D-049, D-050).

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "HitZoneSettings.generated.h"

class UAnimSequenceBase;
class USceneComponent;
struct FHitResult;

/** Where a shot landed on a target */
UENUM(BlueprintType)
enum class EHitZone : uint8
{
	/** Not on a skeletal body, the target keeps its own test */
	None,
	Head,
	Torso,
	/** The arm that holds the weapon, the right one on the mannequin */
	WeaponArm,
	OffArm,
	Leg,
	/** The held weapon itself */
	Weapon
};

/** What a hit in a zone does to the target */
UENUM(BlueprintType)
enum class EHitReaction : uint8
{
	None,
	/** Ends it */
	Kill,
	Flinch,
	/** Knocks the target off its stride: it pauses, falls or slows */
	Stagger,
	/** The weapon arm lets go of the weapon */
	DropWeapon,
	/** The weapon is shot out of the hand, without damage */
	Disarm
};

/** Side of the target the shot came from */
UENUM(BlueprintType)
enum class EHitDirection : uint8
{
	Front,
	Back,
	Left,
	Right
};

/** One zone: the bones that belong to it and what a hit there does */
USTRUCT(BlueprintType)
struct FHitZoneRule
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EHitZone Zone = EHitZone::None;

	/** Physics asset bodies of this zone. A bone in no zone belongs to the zone of its nearest parent that is in one. The weapon zone needs none, the weapon tag marks it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FName> Bones;

	/** Damage against the weapon's own, the torso is the baseline */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = 0.0))
	float DamageMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EHitReaction Reaction = EHitReaction::None;

	/** Points on top of the hit: added to the door range score, and the zone bonus of the style record, paid once per target and shot. The head's is the headshot bonus. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 Score = 0;
};

/** One reaction from the four sides a shot can come from */
USTRUCT(BlueprintType)
struct FHitReactionAnims
{
	GENERATED_BODY()

	/** Shot from in front of the target */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimSequenceBase> Front;

	/** Shot from behind the target */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimSequenceBase> Back;

	/** Shot from the target's own left */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimSequenceBase> Left;

	/** Shot from the target's own right */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimSequenceBase> Right;

	/** The animation for a side. A side without one uses the front, a set without a front any it has. */
	UAnimSequenceBase* Pick(EHitDirection Direction) const;
};

/** What UHitZoneSettings::Resolve found for one hit */
struct FHitZoneResult
{
	EHitZone Zone = EHitZone::None;

	/** Rule of the zone, null when the zone has none */
	const FHitZoneRule* Rule = nullptr;

	EHitDirection Direction = EHitDirection::Front;

	/** Bone the hit reported */
	FName Bone;

	EHitReaction GetReaction() const { return Rule ? Rule->Reaction : EHitReaction::None; }

	/** Damage scale of the zone, unscaled without a rule */
	float GetDamageMultiplier() const { return Rule ? Rule->DamageMultiplier : 1.0f; }

	int32 GetScore() const { return Rule ? Rule->Score : 0; }

	/** The weapon, or the arm that holds it: on a target with a weapon this takes the weapon away instead of bringing the target down (D-050) */
	bool IsDisarm() const { return Zone == EHitZone::Weapon || Zone == EHitZone::WeaponArm; }
};

/**
 *  Hit zones of every target with a skeletal body (D-049): the bones of each zone, checked against the physics asset, and
 *  the damage multiplier, reaction and score of each. A hit on the weapon or the weapon arm disarms and scores above a kill
 *  (D-050). Reactions play as montages picked by the side the shot came from; the animations are set on the asset. The game
 *  uses the asset named under Project Settings, Game, Hit Zones, and these class defaults without one.
 */
UCLASS(BlueprintType)
class CYB3RGUN_API UHitZoneSettings : public UDataAsset
{
	GENERATED_BODY()

public:

	UHitZoneSettings();

	/** One rule per zone */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Zones")
	TArray<FHitZoneRule> Rules;

	/** Component tag of a held weapon. A blocking hit on a component with it is a weapon hit, whatever bone it names. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Zones")
	FName WeaponComponentTag = FName(TEXT("HitZone.Weapon"));

	/** Torso flinch, and the flinch of a target whose weapon was shot out of its hand */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Reactions")
	FHitReactionAnims Flinch;

	/** Arm flinch: an arm hit, and the weapon arm letting go of the weapon */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Reactions")
	FHitReactionAnims ArmFlinch;

	/** Leg hit: the target is knocked off its stride */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Reactions")
	FHitReactionAnims Stagger;

	/** Falls, played on the body itself rather than over it, and held on the last frame */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Reactions")
	FHitReactionAnims Death;

	/** Looped by a disarmed target, empty handed */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Reactions")
	TObjectPtr<UAnimSequenceBase> DisarmedIdle;

	/** Seconds a reaction takes to blend in over what the body was doing */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Timing", meta = (ClampMin = 0.0, ClampMax = 1.0, Units = "s"))
	float ReactionBlendIn = 0.06f;

	/** Seconds a reaction takes to hand the body back */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Timing", meta = (ClampMin = 0.0, ClampMax = 1.0, Units = "s"))
	float ReactionBlendOut = 0.2f;

	/** Seconds the body takes to cross fade into a fall */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Timing", meta = (ClampMin = 0.0, ClampMax = 1.0, Units = "s"))
	float DeathBlend = 0.12f;

	/** Seconds a disarmed target takes to settle into the disarmed idle */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Timing", meta = (ClampMin = 0.0, ClampMax = 2.0, Units = "s"))
	float DisarmedBlend = 0.3f;

	/** Seconds a door stays open after a hit or a disarm before its panel swings shut, so the reaction reads. Never shorter than the controlled pair window. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Timing", meta = (ClampMin = 0.0, ClampMax = 3.0, Units = "s"))
	float DoorHoldAfterHit = 0.8f;

	/** Seconds a leg hit holds a walking target: its approach and its attack pause this long */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement", meta = (ClampMin = 0.0, ClampMax = 5.0, Units = "s"))
	float StaggerSeconds = 0.7f;

	/** Seconds after a stagger ends before a leg hit can stagger again, so a spray of pellets is one stagger */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement", meta = (ClampMin = 0.0, Units = "s"))
	float StaggerCooldown = 2.0f;

	/** Walking speed after a leg hit, against the target's own */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement", meta = (ClampMin = 0.05, ClampMax = 1.0))
	float LegSlowScale = 0.55f;

	/** Seconds the leg slow lasts, renewed by every leg hit */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement", meta = (ClampMin = 0.0, Units = "s"))
	float LegSlowSeconds = 3.0f;

	/** Speed a weapon leaves the hand with when a shot hits it, along the shot */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon Drop", meta = (ClampMin = 0.0, Units = "cm/s"))
	float WeaponDropSpeed = 450.0f;

	/** Speed a weapon falls away with when the arm holding it is hit, along the shot */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon Drop", meta = (ClampMin = 0.0, Units = "cm/s"))
	float ArmDropSpeed = 120.0f;

	/** Spin of a dropped weapon, degrees per second */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon Drop", meta = (ClampMin = 0.0))
	float WeaponDropSpin = 720.0f;

	/** Radius of the sweep that follows a shot on a hostage taker back past its hostage: a shot that grazes the hostage on the way counts on the hostage */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hostage", meta = (ClampMin = 0.0, ClampMax = 20.0, Units = "cm"))
	float HostageGrazeRadius = 4.0f;

	/** The values in use: the asset named in the project settings, else the class defaults */
	static const UHitZoneSettings* Get();

	/**
	 *  The zone a hit landed in and the side of the target the shot came from. The weapon tag wins only on a real blocking hit;
	 *  otherwise the bone walks up its parents until one is in a zone. A skeletal body without a match is torso, anything else
	 *  is None, so the caller keeps its own test.
	 */
	static FHitZoneResult Resolve(const FHitResult& Hit, const FVector& ShotDirection);

	/** Side a shot travelling along ShotDirection comes from, for a target that faces Facing */
	static EHitDirection GetDirection(const FVector& Facing, const FVector& ShotDirection);

	/** Where a target faces: a skeletal body its +Y, the way the mannequin faces, anything else its owner's forward */
	static FVector GetFacing(const USceneComponent* Component);

	/** Rule of a zone, null when it has none */
	const FHitZoneRule* FindRule(EHitZone Zone) const;

	/** Zone a bone is listed in, None when it is in none */
	EHitZone FindBoneZone(FName Bone) const;

	/** Writes the rules, the reaction animations and the timings to the log */
	void LogRules() const;

	virtual void PostLoad() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:

	void RebuildBoneZones();

	/** Bone to zone, rebuilt from the rules and never saved */
	TMap<FName, EHitZone> BoneZones;
};
