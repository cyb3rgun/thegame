// CYB3RGUN THEGAME. Base actor for every enemy.

#include "CyberEnemy.h"
#include "CyberEnemyController.h"
#include "EnemyDefinition.h"
#include "HitReactions.h"
#include "HitZoneSettings.h"
#include "TargetAnimInstance.h"
#include "StyleScoringComponent.h"
#include "StyleSettings.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DamageEvents.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogCyberEnemy, Log, All);

namespace CyberEnemyParts
{
	/** Bone the aim point sits on */
	const FName ChestBone(TEXT("spine_03"));
}

ACyberEnemy::ACyberEnemy()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	AIControllerClass = ACyberEnemyController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// the skeletal mesh shows only for definitions with a body; with a physics asset its bodies are what shots hit,
	// otherwise the capsule is, see ApplyHitCollision
	GetMesh()->SetVisibility(false);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;

	PlaceholderRoot = CreateDefaultSubobject<USceneComponent>(TEXT("PlaceholderRoot"));
	PlaceholderRoot->SetupAttachment(GetCapsuleComponent());

	GetCapsuleComponent()->SetCollisionProfileName(FName("Pawn"));
	// weapons aim with visibility traces, the capsule stops them until ApplyHitCollision hands them to a body
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->bOrientRotationToMovement = true;
	Movement->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
	Movement->bUseControllerDesiredRotation = false;
	bUseControllerRotationYaw = false;
}

void ACyberEnemy::Initialize(UEnemyDefinition* InDefinition)
{
	Definition = InDefinition;
	if (HasActorBegunPlay() || GetWorld())
	{
		ApplyDefinition();
	}
}

void ACyberEnemy::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	ApplyDefinition();
}

void ACyberEnemy::BeginPlay()
{
	Super::BeginPlay();

	if (Definition)
	{
		PlayEnemySound(Definition->Sounds.Spawn);
		if (HasBody())
		{
			GetWorld()->GetTimerManager().SetTimer(BodyAnimationTimer, this, &ACyberEnemy::UpdateBodyAnimation, 0.1f, true);
		}
	}
	else
	{
		UE_LOG(LogCyberEnemy, Warning, TEXT("%s has no enemy definition"), *GetName());
	}
}

void ACyberEnemy::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FlinchTimer);
		World->GetTimerManager().ClearTimer(LingerTimer);
		World->GetTimerManager().ClearTimer(BodyAnimationTimer);
		World->GetTimerManager().ClearTimer(MoveSpeedTimer);
	}
	Super::EndPlay(EndPlayReason);
}

void ACyberEnemy::ApplyDefinition()
{
	if (!Definition)
	{
		return;
	}

	Health = Definition->MaxHealth;

	GetCapsuleComponent()->SetCapsuleSize(Definition->CapsuleRadius, Definition->CapsuleHalfHeight);
	PlaceholderRoot->SetRelativeLocation(FVector(0.0f, 0.0f, -Definition->CapsuleHalfHeight));
	SetMoveSpeed(Definition->MoveSpeed);

	if (HasBody())
	{
		ClearPlaceholder();
		BuildBody();
	}
	else
	{
		GetMesh()->SetVisibility(false);
		BuildPlaceholder();
	}
	ApplyHitCollision();
}

bool ACyberEnemy::HasBody() const
{
	return Definition && Definition->BodyMesh;
}

bool ACyberEnemy::HasHitBody() const
{
	return HasBody() && GetMesh()->GetPhysicsAsset() != nullptr;
}

void ACyberEnemy::BuildBody()
{
	USkeletalMeshComponent* Body = GetMesh();
	Body->SetSkeletalMesh(Definition->BodyMesh);

	// feet on the bottom of the capsule; the mannequin faces +Y, the actor faces +X
	Body->SetRelativeLocationAndRotation(FVector(0.0f, 0.0f, -Definition->CapsuleHalfHeight), FRotator(0.0f, -90.0f, 0.0f));
	Body->SetRelativeScale3D(FVector(Definition->BodyScale));

	if (Definition->BodyMaterial)
	{
		for (int32 Index = 0; Index < Body->GetNumMaterials(); ++Index)
		{
			Body->SetMaterial(Index, Definition->BodyMaterial);
		}
	}

	// a native anim instance, so hit reactions blend over the idle, the walk or the attack
	UTargetAnimInstance::Ensure(Body);
	Body->SetVisibility(true);
	BodyLoop = nullptr;
	OneShotUntil = 0.0f;
	UpdateBodyAnimation();
}

void ACyberEnemy::ApplyHitCollision()
{
	UCapsuleComponent* Capsule = GetCapsuleComponent();
	USkeletalMeshComponent* Body = GetMesh();
	if (HasHitBody() && !bDead)
	{
		// the physics asset bodies are the hit target: traces and projectiles pass the capsule and land on a bone (D-049).
		// The body keeps ignoring pawns, so it never takes part in movement
		Body->SetCollisionProfileName(FName("CharacterMesh"));
		Body->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		Body->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Capsule->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
		Capsule->SetCollisionResponseToChannel(FHitReactions::ProjectileChannel, ECR_Ignore);

		// a body that can be shot keeps its pose fresh off screen as well, so its physics bodies follow the animation
		Body->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	}
	else
	{
		// a placeholder, or a body without a physics asset: weapons aim with visibility traces, the capsule must stop them or
		// shots converge behind the enemy
		Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Capsule->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		Capsule->SetCollisionResponseToChannel(FHitReactions::ProjectileChannel, ECR_Block);
		Body->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
	}
}

void ACyberEnemy::UpdateBodyAnimation()
{
	if (bDead || !HasBody() || GetWorld()->GetTimeSeconds() < OneShotUntil)
	{
		return;
	}

	const float Speed = GetVelocity().Size2D();
	const bool bMoving = Speed > 20.0f && Definition->MoveAnimation;
	UAnimSequenceBase* Wanted = bMoving ? Definition->MoveAnimation.Get() : Definition->IdleAnimation.Get();
	UTargetAnimInstance* Anim = UTargetAnimInstance::FromMesh(GetMesh());
	if (!Wanted || !Anim)
	{
		return;
	}

	if (Wanted != BodyLoop || Anim->GetBaseSequence() != Wanted)
	{
		Anim->PlayBase(Wanted, true);
		BodyLoop = Wanted;
	}
	Anim->SetBasePlayRate(bMoving ? FMath::Clamp(Speed / Definition->MoveAnimationSpeed, 0.5f, 2.5f) : 1.0f);
}

void ACyberEnemy::PlayBodyOneShot(UAnimSequenceBase* Animation)
{
	if (!Animation || !HasBody())
	{
		return;
	}

	if (UTargetAnimInstance* Anim = UTargetAnimInstance::FromMesh(GetMesh()))
	{
		Anim->PlayBase(Animation, false);
	}
	BodyLoop = nullptr;
	OneShotUntil = GetWorld()->GetTimeSeconds() + Animation->GetPlayLength();
}

void ACyberEnemy::BuildPlaceholder()
{
	ClearPlaceholder();

	for (const FEnemyShapePart& Part : Definition->Parts)
	{
		if (!Part.Mesh)
		{
			continue;
		}

		UStaticMeshComponent* Shape = NewObject<UStaticMeshComponent>(this, NAME_None, RF_Transient);
		Shape->SetStaticMesh(Part.Mesh);
		Shape->SetupAttachment(PlaceholderRoot);
		Shape->SetRelativeLocation(Part.Location);
		Shape->SetRelativeRotation(Part.Rotation);
		Shape->SetRelativeScale3D(Part.Scale);
		Shape->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Shape->SetGenerateOverlapEvents(false);
		Shape->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
		if (Definition->Material)
		{
			Shape->SetMaterial(0, Definition->Material);
		}
		Shape->RegisterComponent();
		Parts.Add(Shape);
	}
}

void ACyberEnemy::ClearPlaceholder()
{
	for (UStaticMeshComponent* Shape : Parts)
	{
		if (Shape)
		{
			Shape->DestroyComponent();
		}
	}
	Parts.Reset();
}

void ACyberEnemy::SetMoveSpeed(float Speed)
{
	BaseMoveSpeed = FMath::Max(Speed, 0.0f);
	RefreshMoveSpeed();
}

void ACyberEnemy::RefreshMoveSpeed()
{
	// a stagger holds the enemy where it stands, a limp slows it, both run out on their own
	const float Scale = IsStaggered() ? 0.0f : (IsSlowed() ? UHitZoneSettings::Get()->LegSlowScale : 1.0f);
	GetCharacterMovement()->MaxWalkSpeed = BaseMoveSpeed * Scale;

	// back here when the stagger or the limp ends, whichever comes first
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	const double Now = World->GetTimeSeconds();
	const double NextChange = StaggerUntil > Now ? StaggerUntil : SlowUntil;
	if (NextChange > Now)
	{
		World->GetTimerManager().SetTimer(MoveSpeedTimer, this, &ACyberEnemy::RefreshMoveSpeed, static_cast<float>(NextChange - Now), false);
	}
}

bool ACyberEnemy::IsStaggered() const
{
	const UWorld* World = GetWorld();
	return !bDead && World && World->GetTimeSeconds() < StaggerUntil;
}

bool ACyberEnemy::IsSlowed() const
{
	const UWorld* World = GetWorld();
	return !bDead && World && World->GetTimeSeconds() < SlowUntil;
}

bool ACyberEnemy::ApplyLegHit()
{
	const UHitZoneSettings* Zones = UHitZoneSettings::Get();
	const double Now = GetWorld()->GetTimeSeconds();

	// every leg hit renews the limp; the stagger comes once per cooldown, so a spray of pellets is one stagger
	SlowUntil = FMath::Max(SlowUntil, Now + Zones->LegSlowSeconds);
	const bool bStagger = Now >= NextStaggerAt;
	if (bStagger)
	{
		StaggerUntil = Now + Zones->StaggerSeconds;
		NextStaggerAt = StaggerUntil + Zones->StaggerCooldown;
	}
	RefreshMoveSpeed();
	return bStagger;
}

float ACyberEnemy::GetHealthFraction() const
{
	return (Definition && Definition->MaxHealth > 0.0f) ? FMath::Clamp(Health / Definition->MaxHealth, 0.0f, 1.0f) : 0.0f;
}

float ACyberEnemy::GetDistanceToTarget() const
{
	return Target ? FVector::Dist2D(GetActorLocation(), Target->GetActorLocation()) : TNumericLimits<float>::Max();
}

bool ACyberEnemy::IsTargetInAttackRange(float RangeScale) const
{
	if (!Target || !Definition)
	{
		return false;
	}

	// measure between collision edges so wide enemies do not need to overlap the target, whatever pawn type it is
	const float TargetRadius = Target->GetSimpleCollisionRadius();
	const float Reach = Definition->AttackRange * RangeScale + Definition->CapsuleRadius + TargetRadius;
	return GetDistanceToTarget() <= Reach;
}

FVector ACyberEnemy::GetAimPoint() const
{
	// the chest of a skeletal body, where its physics body is; a point up the capsule of a placeholder
	FVector Chest;
	if (HasHitBody() && FHitReactions::GetBonePoint(GetMesh(), CyberEnemyParts::ChestBone, Chest))
	{
		return Chest;
	}
	const float Height = Definition ? Definition->CapsuleHalfHeight * 0.4f : 40.0f;
	return GetActorLocation() + FVector(0.0f, 0.0f, Height);
}

float ACyberEnemy::TakeDamage(float Damage, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	// engine damage from any source lands here, then flows through the single entry point
	EEnemyDamageSource Source = EEnemyDamageSource::Weapon;
	if (DamageEvent.IsOfType(FRadialDamageEvent::ClassID))
	{
		Source = EEnemyDamageSource::Explosion;
		// a blast throws the body away from its centre
		LastShotDirection = GetActorLocation() - static_cast<const FRadialDamageEvent&>(DamageEvent).Origin;
	}
	else if (DamageEvent.DamageTypeClass && DamageEvent.DamageTypeClass->GetName().Contains(TEXT("Fire")))
	{
		Source = EEnemyDamageSource::Fire;
	}

	// the zone a shot landed in scales the damage and picks the reaction, a shot on the head brings the enemy down at once
	bool bHeadshot = false;
	float Amount = Damage;
	FHitZoneResult Hit;
	if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
	{
		const FPointDamageEvent& PointEvent = static_cast<const FPointDamageEvent&>(DamageEvent);
		LastShotDirection = PointEvent.ShotDirection;
		if (HasHitBody())
		{
			Hit = UHitZoneSettings::Resolve(PointEvent.HitInfo, PointEvent.ShotDirection);
		}

		if (Hit.Zone != EHitZone::None)
		{
			bHeadshot = Hit.Zone == EHitZone::Head;
			Amount *= Hit.GetDamageMultiplier();
			if (Hit.GetReaction() == EHitReaction::Kill)
			{
				Amount = FMath::Max(Amount, Health);
			}
		}
		else
		{
			// a placeholder, or a shot the capsule caught: the head is a line test against the top of the enemy
			bHeadshot = IsHeadHit(PointEvent.HitInfo.ImpactPoint, PointEvent.ShotDirection);
			if (bHeadshot)
			{
				Amount = FMath::Max(Amount, Health);
			}
		}
	}

	const bool bWasAlive = !bDead;
	PendingHit = Hit;
	const float Applied = ApplyEnemyDamage(Amount, Source, EventInstigator, DamageCauser);
	PendingHit = FHitZoneResult();

	if (bWasAlive && Applied > 0.0f)
	{
		if (UStyleScoringComponent* Style = UStyleScoringComponent::ForController(EventInstigator))
		{
			// the zone of a skeletal body, the head test of a placeholder
			const EHitZone StyleZone = Hit.Zone != EHitZone::None ? Hit.Zone : (bHeadshot ? EHitZone::Head : EHitZone::None);
			Style->RecordTargetHit(this, StyleZone, bDead);
		}
	}
	return Applied;
}

bool ACyberEnemy::IsHeadHit(const FVector& Location, const FVector& Direction) const
{
	const UStyleSettings* Style = UStyleSettings::Get(this);
	static const FName HeadBone(TEXT("head"));

	// the head: its bone on a skeletal body, the top of the capsule on a placeholder
	FVector Head;
	if (HasBody() && GetMesh()->GetBoneIndex(HeadBone) != INDEX_NONE)
	{
		Head = GetMesh()->GetBoneLocation(HeadBone);
	}
	else
	{
		Head = GetActorLocation() + FVector(0.0f, 0.0f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight() - Style->PlaceholderHeadDepth);
	}

	// the shot enters the capsule at Location; it is a head hit when its line passes through the head
	const FVector ShotDirection = Direction.GetSafeNormal();
	if (ShotDirection.IsNearlyZero())
	{
		return FVector::Dist(Location, Head) <= Style->EnemyHeadRadius;
	}
	return FMath::PointDistToLine(Head, ShotDirection, Location) <= Style->EnemyHeadRadius;
}

float ACyberEnemy::ApplyEnemyDamage(float Amount, EEnemyDamageSource Source, AController* EventInstigator, AActor* DamageCauser)
{
	if (bDead || Amount <= 0.0f)
	{
		return 0.0f;
	}

	const float Applied = FMath::Min(Amount, Health);
	Health -= Applied;

	UE_LOG(LogCyberEnemy, Verbose, TEXT("%s takes %.0f damage from %s, health %.0f"), *GetName(), Applied, *GetNameSafe(DamageCauser), Health);

	OnEnemyDamaged.Broadcast(this, Applied, GetHealthFraction());

	if (Health <= 0.0f)
	{
		Die(EventInstigator);
	}
	else
	{
		ReactToHit();
		if (Definition)
		{
			PlayEnemySound(Definition->Sounds.Hurt);
		}
		BP_OnHitReaction(Applied, Source);
	}

	return Applied;
}

void ACyberEnemy::ReactToHit()
{
	// a skeletal body plays its zone's reaction over whatever it is doing, a leg hit also holds and slows it
	if (PendingHit.Zone != EHitZone::None && HasHitBody())
	{
		EHitReaction Reaction = PendingHit.GetReaction();
		if (Reaction == EHitReaction::Stagger && !ApplyLegHit())
		{
			// inside the stagger cooldown a leg hit only limps, a flinch shows it landed
			Reaction = EHitReaction::Flinch;
		}
		if (FHitReactions::PlayReaction(GetMesh(), Reaction, PendingHit.Zone, PendingHit.Direction))
		{
			return;
		}
	}

	// a placeholder, or a body without that reaction: the quick squash
	Flinch();
}

bool ACyberEnemy::PerformAttack()
{
	if (bDead || !Definition || !IsTargetInAttackRange())
	{
		return false;
	}

	PlayEnemySound(Definition->Sounds.Attack);

	if (HasBody())
	{
		PlayBodyOneShot(Definition->AttackAnimation);
	}
	else
	{
		// placeholder lunge: a short forward scale punch through the flinch path
		Flinch();
	}

	UGameplayStatics::ApplyDamage(Target, Definition->Damage, GetController(), this, UDamageType::StaticClass());
	UE_LOG(LogCyberEnemy, Verbose, TEXT("%s attacks %s for %.0f"), *GetName(), *GetNameSafe(Target), Definition->Damage);
	return true;
}

void ACyberEnemy::Die(AController* Killer)
{
	bDead = true;
	Health = 0.0f;

	UE_LOG(LogCyberEnemy, Log, TEXT("%s died, killed by %s"), *GetName(), *GetNameSafe(Killer));

	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// a corpse catches no more shots, they go on to whatever stands behind it
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;

	if (Definition)
	{
		PlayEnemySound(Definition->Sounds.Death);
	}

	if (ACyberEnemyController* AI = Cast<ACyberEnemyController>(GetController()))
	{
		AI->NotifyDied();
	}

	ClearFlinch();
	GetWorld()->GetTimerManager().ClearTimer(BodyAnimationTimer);
	GetWorld()->GetTimerManager().ClearTimer(MoveSpeedTimer);
	if (HasBody())
	{
		// the body falls away from the last shot and holds its last frame: the hit zone settings' fall for that side,
		// else one of the definition's at random
		const EHitDirection Direction = UHitZoneSettings::GetDirection(UHitZoneSettings::GetFacing(GetMesh()), LastShotDirection);
		if (!FHitReactions::PlayDeath(GetMesh(), Direction))
		{
			const TArray<TObjectPtr<UAnimSequenceBase>>& Deaths = Definition->DeathAnimations;
			if (Deaths.Num() > 0)
			{
				FHitReactions::PlayFall(GetMesh(), Deaths[FMath::RandRange(0, Deaths.Num() - 1)].Get());
			}
		}
	}
	else
	{
		DeathPoseElapsed = 0.0f;
		SetActorTickEnabled(true);
	}

	BP_OnDeath();
	OnEnemyDied.Broadcast(this, Killer);

	const float Linger = Definition ? Definition->DeathLinger : 2.0f;
	GetWorld()->GetTimerManager().SetTimer(LingerTimer, this, &ACyberEnemy::RemoveBody, FMath::Max(Linger, 0.05f), false);
}

void ACyberEnemy::RemoveBody()
{
	Destroy();
}

void ACyberEnemy::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// the only per tick work: the death tip over, and only while it runs
	if (bDead)
	{
		DeathPoseElapsed += DeltaSeconds;
		const float Alpha = FMath::Clamp(DeathPoseElapsed / DeathPoseSeconds, 0.0f, 1.0f);
		PlaceholderRoot->SetRelativeRotation(FRotator(-85.0f * Alpha, 0.0f, 0.0f));
		if (Alpha >= 1.0f)
		{
			SetActorTickEnabled(false);
		}
	}
}

void ACyberEnemy::Flinch()
{
	if (HasBody())
	{
		GetMesh()->SetRelativeScale3D(FVector(Definition->BodyScale) * FVector(1.06f, 1.06f, 0.95f));
	}
	else
	{
		PlaceholderRoot->SetRelativeScale3D(FVector(1.15f, 1.15f, 0.92f));
	}
	GetWorld()->GetTimerManager().SetTimer(FlinchTimer, this, &ACyberEnemy::ClearFlinch, 0.12f, false);
}

void ACyberEnemy::ClearFlinch()
{
	if (HasBody())
	{
		GetMesh()->SetRelativeScale3D(FVector(Definition->BodyScale));
	}
	PlaceholderRoot->SetRelativeScale3D(FVector::OneVector);
}

void ACyberEnemy::PlayEnemySound(USoundBase* Sound) const
{
	if (Sound && Definition)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Sound, GetActorLocation(), 1.0f, Definition->Sounds.Pitch);
	}
}
