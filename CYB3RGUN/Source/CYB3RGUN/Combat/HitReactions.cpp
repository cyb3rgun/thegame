// CYB3RGUN THEGAME. Hit reactions of target bodies, and the held weapon as a hit target of its own.

#include "HitReactions.h"
#include "TargetAnimInstance.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequenceBase.h"
#include "CollisionShape.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/HitResult.h"
#include "PhysicsEngine/BodyInstance.h"

UAnimSequenceBase* FHitReactions::PickAnimation(EHitReaction Reaction, EHitZone Zone, EHitDirection Direction)
{
	const UHitZoneSettings* Settings = UHitZoneSettings::Get();
	const bool bArm = Zone == EHitZone::WeaponArm || Zone == EHitZone::OffArm;

	const FHitReactionAnims* Set = nullptr;
	switch (Reaction)
	{
	case EHitReaction::Kill:
		Set = &Settings->Death;
		break;
	case EHitReaction::Flinch:
		// an arm hit flinches the arm, anything else the torso
		Set = bArm ? &Settings->ArmFlinch : &Settings->Flinch;
		break;
	case EHitReaction::Stagger:
		Set = &Settings->Stagger;
		break;
	case EHitReaction::DropWeapon:
		// the arm jerks whether or not it has a weapon to let go of
		Set = &Settings->ArmFlinch;
		break;
	case EHitReaction::Disarm:
		Set = &Settings->Flinch;
		break;
	default:
		break;
	}
	return Set ? Set->Pick(Direction) : nullptr;
}

bool FHitReactions::PlayReaction(USkeletalMeshComponent* Body, EHitReaction Reaction, EHitZone Zone, EHitDirection Direction, TFunction<void()> OnBlendOut)
{
	if (Reaction == EHitReaction::Kill)
	{
		return PlayDeath(Body, Direction);
	}

	UTargetAnimInstance* Anim = UTargetAnimInstance::FromMesh(Body);
	UAnimSequenceBase* Animation = PickAnimation(Reaction, Zone, Direction);
	if (!Anim || !Animation)
	{
		return false;
	}

	const UHitZoneSettings* Settings = UHitZoneSettings::Get();
	UAnimMontage* Montage = Anim->PlayReaction(Animation, Settings->ReactionBlendIn, Settings->ReactionBlendOut);
	if (!Montage)
	{
		return false;
	}

	if (OnBlendOut)
	{
		FOnMontageBlendingOutStarted BlendingOut = FOnMontageBlendingOutStarted::CreateLambda([Callback = MoveTemp(OnBlendOut)](UAnimMontage* Ended, bool bInterrupted)
		{
			// a newer reaction or a stop cut this one off, whatever was to follow it no longer applies
			if (!bInterrupted)
			{
				Callback();
			}
		});
		Anim->Montage_SetBlendingOutDelegate(BlendingOut, Montage);
	}
	return true;
}

bool FHitReactions::PlayDeath(USkeletalMeshComponent* Body, EHitDirection Direction)
{
	return PlayFall(Body, UHitZoneSettings::Get()->Death.Pick(Direction));
}

bool FHitReactions::PlayFall(USkeletalMeshComponent* Body, UAnimSequenceBase* Fall, float PlayRate)
{
	UTargetAnimInstance* Anim = UTargetAnimInstance::FromMesh(Body);
	if (!Anim || !Fall)
	{
		return false;
	}

	// a fall lives on the base layer, a montage would hand the body back to its idle when it ends
	const float Blend = UHitZoneSettings::Get()->DeathBlend;
	Anim->StopAllMontages(Blend);
	Anim->PlayBase(Fall, false, PlayRate, 0.0f, Blend);
	return true;
}

bool FHitReactions::PlayDisarmedIdle(USkeletalMeshComponent* Body)
{
	const UHitZoneSettings* Settings = UHitZoneSettings::Get();
	UTargetAnimInstance* Anim = UTargetAnimInstance::FromMesh(Body);
	if (!Anim || !Settings->DisarmedIdle)
	{
		return false;
	}

	Anim->PlayBase(Settings->DisarmedIdle, true, 1.0f, 0.0f, Settings->DisarmedBlend);
	return true;
}

void FHitReactions::StopReactions(USkeletalMeshComponent* Body)
{
	if (UTargetAnimInstance* Anim = UTargetAnimInstance::FromMesh(Body))
	{
		Anim->StopAllMontages(0.0f);
	}
}

void FHitReactions::SetupBodyTarget(USkeletalMeshComponent* Body)
{
	if (!Body)
	{
		return;
	}

	// every physics asset body takes the component's object type and responses. WorldStatic blocking everything, like the hit
	// volumes they replace: the point blank sweep still finds them, explosions, which look for pawns and dynamic objects, do not
	Body->SetCollisionObjectType(ECC_WorldStatic);
	Body->SetCollisionResponseToAllChannels(ECR_Block);
	Body->SetGenerateOverlapEvents(false);
	Body->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;

	// the bodies are built once, not again on every reveal
	Body->bAlwaysCreatePhysicsState = true;
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void FHitReactions::SetBodyShootable(USkeletalMeshComponent* Body, bool bShootable)
{
	if (Body)
	{
		Body->SetCollisionEnabled(bShootable ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	}
}

void FHitReactions::SetWeaponShootable(USkeletalMeshComponent* Weapon, bool bShootable)
{
	if (!Weapon || IsWeaponDropped(Weapon))
	{
		return;
	}

	const FName Tag = UHitZoneSettings::Get()->WeaponComponentTag;
	if (!Tag.IsNone())
	{
		Weapon->ComponentTags.AddUnique(Tag);
	}

	// only what a shot uses finds the pistol, projectile sweeps and visibility traces, and it never pushes the body holding it
	Weapon->SetCollisionObjectType(ECC_WorldDynamic);
	Weapon->SetCollisionResponseToAllChannels(ECR_Ignore);
	Weapon->SetCollisionResponseToChannel(ProjectileChannel, ECR_Block);
	Weapon->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	Weapon->SetGenerateOverlapEvents(false);
	Weapon->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
	Weapon->SetCollisionEnabled(bShootable ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
}

void FHitReactions::DropWeapon(USkeletalMeshComponent* Weapon, const FVector& ShotDirection, const FVector& HitLocation, bool bKnockedAway)
{
	if (!Weapon || IsWeaponDropped(Weapon))
	{
		return;
	}

	const UHitZoneSettings* Settings = UHitZoneSettings::Get();
	const FVector Along = ShotDirection.GetSafeNormal();

	// out of the hand; it keeps its owner, so it must leave every query or a stray shot on the floor would still reach the target
	Weapon->DetachFromComponent(FDetachmentTransformRules(EDetachmentRule::KeepWorld, false));
	Weapon->SetCollisionObjectType(ECC_PhysicsBody);
	Weapon->SetCollisionResponseToAllChannels(ECR_Ignore);
	Weapon->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	Weapon->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);

	// physics collision has to be on before it can simulate
	Weapon->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	Weapon->SetSimulatePhysics(true);

	const float Speed = bKnockedAway ? Settings->WeaponDropSpeed : Settings->ArmDropSpeed;
	Weapon->AddImpulse(Along * Speed, NAME_None, true);

	// it turns about the axis the shot pushes it around, or tumbles over the line of the shot when hit dead centre
	FVector Axis = FVector::CrossProduct(HitLocation - Weapon->GetComponentLocation(), Along).GetSafeNormal();
	if (Axis.IsNearlyZero())
	{
		Axis = FVector::CrossProduct(FVector::UpVector, Along).GetSafeNormal();
	}
	Weapon->AddAngularImpulseInDegrees(Axis * Settings->WeaponDropSpin, NAME_None, true);
}

void FHitReactions::ResetWeapon(USkeletalMeshComponent* Weapon, USkeletalMeshComponent* Body, FName Socket)
{
	if (!Weapon || !Body)
	{
		return;
	}

	// simulation off first, it also stops the weapon; attached while it still simulates it would be torn loose again
	if (Weapon->IsSimulatingPhysics())
	{
		Weapon->SetSimulatePhysics(false);
	}
	if (Weapon->GetAttachParent() != Body || Weapon->GetAttachSocketName() != Socket)
	{
		Weapon->AttachToComponent(Body, FAttachmentTransformRules::SnapToTargetIncludingScale, Socket);
	}
	SetWeaponShootable(Weapon, false);
	Weapon->SetVisibility(Body->GetVisibleFlag());
}

bool FHitReactions::IsWeaponDropped(const USkeletalMeshComponent* Weapon)
{
	return Weapon && (!Weapon->GetAttachParent() || Weapon->IsSimulatingPhysics());
}

bool FHitReactions::Grazes(USkeletalMeshComponent* Body, const FHitResult& Hit, const FVector& ShotDirection, FHitResult& OutGraze)
{
	const FVector Along = ShotDirection.GetSafeNormal();
	if (!Body || !Body->IsCollisionEnabled() || Along.IsNearlyZero())
	{
		return false;
	}

	// back from the impact along the shot, far enough to cross the whole body wherever it stands on that line
	const FVector End = Hit.ImpactPoint;
	const float Reach = FVector::Dist(End, Body->Bounds.Origin) + Body->Bounds.SphereRadius;
	const FVector Start = End - Along * Reach;
	const FCollisionShape Probe = FCollisionShape::MakeSphere(UHitZoneSettings::Get()->HostageGrazeRadius);
	if (!Body->SweepComponent(OutGraze, Start, End, FQuat::Identity, Probe))
	{
		return false;
	}

	OutGraze.Component = Body;
	OutGraze.bBlockingHit = true;
	return true;
}

bool FHitReactions::GetBonePoint(const USkeletalMeshComponent* Body, FName Bone, FVector& OutPoint)
{
	if (!Body || Bone.IsNone() || Body->GetBoneIndex(Bone) == INDEX_NONE)
	{
		return false;
	}

	// the middle of the bone's physics body is where a shot surely lands on it, the joint itself can sit in the body of its parent
	if (const FBodyInstance* BodyInstance = Body->GetBodyInstance(Bone))
	{
		if (BodyInstance->IsValidBodyInstance())
		{
			OutPoint = BodyInstance->GetBodyBounds().GetCenter();
			return true;
		}
	}
	OutPoint = Body->GetBoneLocation(Bone);
	return true;
}

bool FHitReactions::GetWeaponPoint(const USkeletalMeshComponent* Weapon, FVector& OutPoint)
{
	if (!Weapon || IsWeaponDropped(Weapon))
	{
		return false;
	}

	const FBodyInstance* BodyInstance = Weapon->GetBodyInstance();
	OutPoint = (BodyInstance && BodyInstance->IsValidBodyInstance()) ? BodyInstance->GetBodyBounds().GetCenter() : Weapon->Bounds.Origin;
	return true;
}
