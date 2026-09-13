// CYB3RGUN THEGAME. Hit zones: which bone is head, torso, limb or weapon, and what a hit there does.

#include "HitZoneSettings.h"
#include "HitZoneProjectSettings.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkinnedMeshComponent.h"
#include "Engine/HitResult.h"
#include "GameFramework/Actor.h"
#include "HAL/IConsoleManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogHitZones, Log, All);

UAnimSequenceBase* FHitReactionAnims::Pick(EHitDirection Direction) const
{
	UAnimSequenceBase* Sided = nullptr;
	switch (Direction)
	{
	case EHitDirection::Back:
		Sided = Back.Get();
		break;
	case EHitDirection::Left:
		Sided = Left.Get();
		break;
	case EHitDirection::Right:
		Sided = Right.Get();
		break;
	default:
		Sided = Front.Get();
		break;
	}
	if (Sided)
	{
		return Sided;
	}

	// a set filled for one side still reacts from every side
	for (UAnimSequenceBase* Any : { Front.Get(), Back.Get(), Left.Get(), Right.Get() })
	{
		if (Any)
		{
			return Any;
		}
	}
	return nullptr;
}

UHitZoneSettings::UHitZoneSettings()
{
	auto MakeRule = [](EHitZone Zone, std::initializer_list<const TCHAR*> Bones, float DamageMultiplier, EHitReaction Reaction, int32 Score)
	{
		FHitZoneRule Rule;
		Rule.Zone = Zone;
		for (const TCHAR* Bone : Bones)
		{
			Rule.Bones.Add(FName(Bone));
		}
		Rule.DamageMultiplier = DamageMultiplier;
		Rule.Reaction = Reaction;
		Rule.Score = Score;
		return Rule;
	};

	// the bodies of the mannequin physics asset. The pistol sits in HandGrip_R, so the right arm is the weapon arm.
	// Scores run weapon, weapon arm, head, torso, then legs and the off arm: a disarm pays more than a kill (D-050)
	Rules.Add(MakeRule(EHitZone::Head, { TEXT("head"), TEXT("neck_02") }, 1.0f, EHitReaction::Kill, 60));
	Rules.Add(MakeRule(EHitZone::Torso, { TEXT("pelvis"), TEXT("spine_02"), TEXT("spine_03"), TEXT("spine_04"), TEXT("spine_05"), TEXT("neck_01"), TEXT("clavicle_l"), TEXT("clavicle_r") }, 1.0f, EHitReaction::Flinch, 20));
	Rules.Add(MakeRule(EHitZone::WeaponArm, { TEXT("upperarm_r"), TEXT("lowerarm_r"), TEXT("hand_r") }, 0.6f, EHitReaction::DropWeapon, 200));
	Rules.Add(MakeRule(EHitZone::OffArm, { TEXT("upperarm_l"), TEXT("lowerarm_l"), TEXT("hand_l") }, 0.6f, EHitReaction::Flinch, 10));
	Rules.Add(MakeRule(EHitZone::Leg, { TEXT("thigh_l"), TEXT("thigh_r"), TEXT("calf_l"), TEXT("calf_r"), TEXT("foot_l"), TEXT("foot_r") }, 0.75f, EHitReaction::Stagger, 10));
	Rules.Add(MakeRule(EHitZone::Weapon, {}, 0.0f, EHitReaction::Disarm, 300));

	RebuildBoneZones();
}

void UHitZoneSettings::PostLoad()
{
	Super::PostLoad();
	RebuildBoneZones();
}

#if WITH_EDITOR
void UHitZoneSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	RebuildBoneZones();
}
#endif

void UHitZoneSettings::RebuildBoneZones()
{
	BoneZones.Reset();
	for (const FHitZoneRule& Rule : Rules)
	{
		if (Rule.Zone == EHitZone::None)
		{
			continue;
		}
		for (const FName& Bone : Rule.Bones)
		{
			if (!Bone.IsNone())
			{
				BoneZones.Add(Bone, Rule.Zone);
			}
		}
	}
}

const UHitZoneSettings* UHitZoneSettings::Get()
{
	const TSoftObjectPtr<UHitZoneSettings>& Asset = GetDefault<UHitZoneProjectSettings>()->DefaultHitZoneSettings;
	if (const UHitZoneSettings* Loaded = Asset.Get())
	{
		return Loaded;
	}

	// a missing asset is looked for once rather than on every shot, the class defaults stand in for it
	static FSoftObjectPath MissingAsset;
	if (!Asset.IsNull() && Asset.ToSoftObjectPath() != MissingAsset)
	{
		if (const UHitZoneSettings* Loaded = Asset.LoadSynchronous())
		{
			return Loaded;
		}
		MissingAsset = Asset.ToSoftObjectPath();
		UE_LOG(LogHitZones, Warning, TEXT("No hit zone asset at %s, the class defaults are used"), *MissingAsset.ToString());
	}
	return GetDefault<UHitZoneSettings>();
}

const FHitZoneRule* UHitZoneSettings::FindRule(EHitZone Zone) const
{
	if (Zone == EHitZone::None)
	{
		return nullptr;
	}
	return Rules.FindByPredicate([Zone](const FHitZoneRule& Rule) { return Rule.Zone == Zone; });
}

EHitZone UHitZoneSettings::FindBoneZone(FName Bone) const
{
	const EHitZone* Found = BoneZones.Find(Bone);
	return Found ? *Found : EHitZone::None;
}

FVector UHitZoneSettings::GetFacing(const USceneComponent* Component)
{
	if (!Component)
	{
		return FVector::ForwardVector;
	}
	if (Component->IsA<USkinnedMeshComponent>())
	{
		// the mannequin faces its component +Y, every target body is turned so that is the actor's forward
		return Component->GetRightVector();
	}
	const AActor* Owner = Component->GetOwner();
	return Owner ? Owner->GetActorForwardVector() : Component->GetForwardVector();
}

EHitDirection UHitZoneSettings::GetDirection(const FVector& Facing, const FVector& ShotDirection)
{
	const FVector Forward = FVector(Facing.X, Facing.Y, 0.0f).GetSafeNormal();
	const FVector ToShooter = FVector(-ShotDirection.X, -ShotDirection.Y, 0.0f).GetSafeNormal();
	if (Forward.IsNearlyZero() || ToShooter.IsNearlyZero())
	{
		return EHitDirection::Front;
	}

	// a quarter turn each: the shooter within 45 degrees of the facing is in front, of its back behind, the rest a side
	const float Along = FVector::DotProduct(ToShooter, Forward);
	if (Along >= UE_HALF_SQRT_2)
	{
		return EHitDirection::Front;
	}
	if (Along <= -UE_HALF_SQRT_2)
	{
		return EHitDirection::Back;
	}
	const FVector TargetRight = FVector::CrossProduct(FVector::UpVector, Forward);
	return FVector::DotProduct(ToShooter, TargetRight) > 0.0f ? EHitDirection::Right : EHitDirection::Left;
}

FHitZoneResult UHitZoneSettings::Resolve(const FHitResult& Hit, const FVector& ShotDirection)
{
	const UHitZoneSettings* Settings = Get();
	FHitZoneResult Result;
	Result.Bone = Hit.BoneName;

	const UPrimitiveComponent* Component = Hit.GetComponent();
	if (!Component)
	{
		return Result;
	}

	// a held weapon turns with the body that holds it
	const bool bWeapon = !Settings->WeaponComponentTag.IsNone() && Component->ComponentHasTag(Settings->WeaponComponentTag);
	const USceneComponent* Facing = (bWeapon && Component->GetAttachParent()) ? Component->GetAttachParent() : Component;
	Result.Direction = GetDirection(GetFacing(Facing), ShotDirection);

	if (bWeapon && Hit.bBlockingHit)
	{
		// only a real shot disarms, never the made up hit of an explosion
		Result.Zone = EHitZone::Weapon;
	}
	else if (const USkinnedMeshComponent* Skinned = Cast<USkinnedMeshComponent>(Component))
	{
		// the hit names a physics body; a bone that is in no zone takes the zone of its nearest parent that is
		Result.Zone = EHitZone::Torso;
		for (FName Bone = bWeapon ? NAME_None : Hit.BoneName; !Bone.IsNone(); Bone = Skinned->GetParentBone(Bone))
		{
			const EHitZone Found = Settings->FindBoneZone(Bone);
			if (Found != EHitZone::None)
			{
				Result.Zone = Found;
				break;
			}
		}
	}

	Result.Rule = Settings->FindRule(Result.Zone);
	return Result;
}

void UHitZoneSettings::LogRules() const
{
	const UEnum* ZoneEnum = StaticEnum<EHitZone>();
	const UEnum* ReactionEnum = StaticEnum<EHitReaction>();

	UE_LOG(LogHitZones, Display, TEXT("Hit zones from %s, weapon tag %s, %d bones mapped"), *GetPathName(), *WeaponComponentTag.ToString(), BoneZones.Num());
	for (const FHitZoneRule& Rule : Rules)
	{
		FString Bones;
		for (const FName& Bone : Rule.Bones)
		{
			Bones += Bones.IsEmpty() ? Bone.ToString() : FString(TEXT(" ")) + Bone.ToString();
		}
		UE_LOG(LogHitZones, Display, TEXT("  %s: damage x%.2f, %s, score %d, bones %s"),
			*ZoneEnum->GetNameStringByValue(static_cast<int64>(Rule.Zone)), Rule.DamageMultiplier,
			*ReactionEnum->GetNameStringByValue(static_cast<int64>(Rule.Reaction)), Rule.Score, Bones.IsEmpty() ? TEXT("none") : *Bones);
	}

	auto LogSet = [](const TCHAR* Name, const FHitReactionAnims& Set)
	{
		UE_LOG(LogHitZones, Display, TEXT("  %s: front %s, back %s, left %s, right %s"), Name,
			*GetNameSafe(Set.Front.Get()), *GetNameSafe(Set.Back.Get()), *GetNameSafe(Set.Left.Get()), *GetNameSafe(Set.Right.Get()));
	};
	LogSet(TEXT("Flinch"), Flinch);
	LogSet(TEXT("Arm flinch"), ArmFlinch);
	LogSet(TEXT("Stagger"), Stagger);
	LogSet(TEXT("Death"), Death);
	UE_LOG(LogHitZones, Display, TEXT("  Disarmed idle %s"), *GetNameSafe(DisarmedIdle.Get()));

	UE_LOG(LogHitZones, Display, TEXT("  Blend in %.2f s, out %.2f s, death %.2f s, disarmed %.2f s; stagger %.2f s, cooldown %.2f s; leg slow x%.2f for %.1f s; weapon drop %.0f cm/s, arm drop %.0f cm/s, spin %.0f deg/s; hostage graze %.1f cm"),
		ReactionBlendIn, ReactionBlendOut, DeathBlend, DisarmedBlend, StaggerSeconds, StaggerCooldown, LegSlowScale, LegSlowSeconds,
		WeaponDropSpeed, ArmDropSpeed, WeaponDropSpin, HostageGrazeRadius);
}

#if !UE_BUILD_SHIPPING

static FAutoConsoleCommand GHitZonesStatusCommand(
	TEXT("HitZones.Status"),
	TEXT("Logs the hit zone rules in use: bones, damage multipliers, reactions, scores, reaction animations and timings."),
	FConsoleCommandDelegate::CreateLambda([]()
	{
		UHitZoneSettings::Get()->LogRules();
	}));

#endif
