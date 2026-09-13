// Copyright Epic Games, Inc. All Rights Reserved.


#include "ShooterProjectile.h"
#include "Components/SphereComponent.h"
#include "Components/SkinnedMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "DoorRangeTarget.h"
#include "HitReactions.h"
#include "ShotFeedback.h"
#include "StyleScoringComponent.h"

AShooterProjectile::AShooterProjectile()
{
	PrimaryActorTick.bCanEverTick = true;

	// create the collision component and assign it as the root
	RootComponent = CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("Collision Component"));

	CollisionComponent->SetSphereRadius(16.0f);
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Block);
	CollisionComponent->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;

	// create the projectile movement component. No need to attach it because it's not a Scene Component
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Projectile Movement"));

	ProjectileMovement->InitialSpeed = 3000.0f;
	ProjectileMovement->MaxSpeed = 3000.0f;
	ProjectileMovement->bShouldBounce = true;

	// set the default damage type
	HitDamageType = UDamageType::StaticClass();
}

void AShooterProjectile::BeginPlay()
{
	Super::BeginPlay();
	
	// ignore the pawn that shot this projectile
	CollisionComponent->IgnoreActorWhenMoving(GetInstigator(), true);

	// the sweep uses the sphere's object type as its channel. On the Projectile channel it passes the capsule of an enemy
	// with a hit body and lands on a bone; everything else still blocks it, the channel's default response is Block
	CollisionComponent->SetCollisionObjectType(FHitReactions::ProjectileChannel);
}

void AShooterProjectile::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// clear the destruction timer
	GetWorld()->GetTimerManager().ClearTimer(DestructionTimer);
	GetWorld()->GetTimerManager().ClearTimer(StyleTimeoutTimer);

	// a shot that leaves the world without landing is a miss
	if (EndPlayReason == EEndPlayReason::Destroyed)
	{
		ResolveStyleMiss();
	}
}

void AShooterProjectile::NotifyHit(class UPrimitiveComponent* MyComp, AActor* Other, class UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit)
{
	// ignore if we've already hit something else
	if (bHit)
	{
		return;
	}

	bHit = true;

	// disable collision on the projectile
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// make AI perception noise
	MakeNoise(NoiseLoudness, GetInstigator(), GetActorLocation(), NoiseRange, NoiseTag);

	// a player's shot resolves here: whatever it landed on reports a hit, anything else makes it a miss
	UStyleScoringComponent* Style = nullptr;
	if (bStyleShot && !bStyleResolved)
	{
		bStyleResolved = true;
		GetWorld()->GetTimerManager().ClearTimer(StyleTimeoutTimer);
		Style = UStyleScoringComponent::Get(GetInstigator());
	}
	if (Style)
	{
		Style->BeginShotResolution();
	}

	if (bExplodeOnHit)
	{
		
		// apply explosion damage centered on the projectile
		ExplosionCheck(GetActorLocation());

	} else {

		// single hit projectile. Process the collided actor along the direction it travelled, with the real hit and its bone
		const FVector Travel = (Hit.TraceEnd - Hit.TraceStart).GetSafeNormal();
		const FVector Direction = Travel.IsNearlyZero() ? -Hit.ImpactNormal : Travel;
		ProcessHit(Other, RefineBoneHit(Hit, Direction), Direction);

	}

	if (Style)
	{
		Style->EndShotResolution();
	}

	// sparks, a brief light and a mark where the shot lands
	UShotFeedback::PlayImpact(this, Hit.ImpactPoint, Hit.ImpactNormal);
	UShotFeedback::PlayImpactDecal(this, Hit);

	// pass control to BP for any extra effects
	BP_OnProjectileHit(Hit);

	// check if we should schedule deferred destruction of the projectile
	if (DeferredDestructionTime > 0.0f)
	{
		GetWorld()->GetTimerManager().SetTimer(DestructionTimer, this, &AShooterProjectile::OnDeferredDestruction, DeferredDestructionTime, false);

	} else {

		// destroy the projectile right away
		Destroy();
	}
}

void AShooterProjectile::ExplosionCheck(const FVector& ExplosionCenter)
{
	// do a sphere overlap check look for nearby actors to damage
	TArray<FOverlapResult> Overlaps;

	FCollisionShape OverlapShape;
	OverlapShape.SetSphere(ExplosionRadius);

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectParams.AddObjectTypesToQuery(ECC_PhysicsBody);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	if (!bDamageOwner)
	{
		QueryParams.AddIgnoredActor(GetInstigator());
	}

	GetWorld()->OverlapMultiByObjectType(Overlaps, ExplosionCenter, FQuat::Identity, ObjectParams, OverlapShape, QueryParams);

	TArray<AActor*> DamagedActors;

	// process the overlap results
	for (const FOverlapResult& CurrentOverlap : Overlaps)
	{
		// overlaps may return the same actor multiple times per each component overlapped
		// ensure we only damage each actor once by adding it to a damaged list
		if (DamagedActors.Find(CurrentOverlap.GetActor()) == INDEX_NONE)
		{
			DamagedActors.Add(CurrentOverlap.GetActor());

			// apply physics force away from the explosion
			const FVector& ExplosionDir = CurrentOverlap.GetActor()->GetActorLocation() - GetActorLocation();

			// push and/or damage the overlapped actor. The made up hit names no bone and is no blocking hit, so it never disarms
			const FVector Direction = ExplosionDir.GetSafeNormal();
			const FHitResult BlastHit(CurrentOverlap.GetActor(), CurrentOverlap.GetComponent(), GetActorLocation(), -Direction);
			ProcessHit(CurrentOverlap.GetActor(), BlastHit, Direction);
		}
			
	}
}

void AShooterProjectile::ProcessHit(AActor* HitActor, const FHitResult& Hit, const FVector& HitDirection)
{
	AController* InstigatorController = GetInstigator() ? GetInstigator()->GetController() : nullptr;

	// have we hit a scoring target? Let it decide whether the shot counts and which zone it landed in
	if (IDoorRangeTarget* Target = Cast<IDoorRangeTarget>(HitActor))
	{
		Target->NotifyShot(Hit, HitDirection, InstigatorController);
	}

	// have we hit a character?
	if (ACharacter* HitCharacter = Cast<ACharacter>(HitActor))
	{
		// ignore the owner of this projectile
		if (HitCharacter != GetOwner() || bDamageOwner)
		{
			// point damage carries the real hit and its bone, so the target can tell which zone the shot landed in
			UGameplayStatics::ApplyPointDamage(HitCharacter, HitDamage, HitDirection, Hit, InstigatorController, this, HitDamageType);
		}
	}

	// have we hit a physics object?
	UPrimitiveComponent* HitComp = Hit.GetComponent();
	if (HitComp && HitComp->IsSimulatingPhysics(Hit.BoneName))
	{
		// give some physics impulse to the object
		HitComp->AddImpulseAtLocation(HitDirection * PhysicsForce, Hit.ImpactPoint, Hit.BoneName);
	}
}

FHitResult AShooterProjectile::RefineBoneHit(const FHitResult& Hit, const FVector& Direction) const
{
	UPrimitiveComponent* Component = Hit.GetComponent();
	if (!Component || !Component->IsA<USkinnedMeshComponent>() || Hit.BoneName.IsNone())
	{
		return Hit;
	}

	// the line the centre of the sphere flew along, carried a sphere's width past the contact
	const FVector End = Hit.ImpactPoint + Direction * (2.0f * CollisionComponent->GetScaledSphereRadius());
	FHitResult Refined;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ShooterProjectileBone), false);
	if (!Component->LineTraceComponent(Refined, Hit.TraceStart, End, QueryParams) || Refined.BoneName.IsNone())
	{
		return Hit;
	}

	FHitResult Result = Hit;
	Result.BoneName = Refined.BoneName;
	Result.Item = Refined.Item;
	return Result;
}

void AShooterProjectile::OnDeferredDestruction()
{
	// destroy this actor
	Destroy();
}

void AShooterProjectile::SetNoiseTag(const FName& Tag)
{
	NoiseTag = Tag;
}

void AShooterProjectile::MarkAsStyleShot(float MissTimeout)
{
	bStyleShot = true;

	// a shot into the open never lands, it counts as a miss once it has flown this long
	GetWorld()->GetTimerManager().SetTimer(StyleTimeoutTimer, this, &AShooterProjectile::ResolveStyleMiss, FMath::Max(MissTimeout, 0.1f), false);
}

void AShooterProjectile::ResolveStyleMiss()
{
	if (!bStyleShot || bStyleResolved)
	{
		return;
	}
	bStyleResolved = true;

	if (UStyleScoringComponent* Style = UStyleScoringComponent::Get(GetInstigator()))
	{
		Style->BeginShotResolution();
		Style->EndShotResolution();
	}
}

void AShooterProjectile::SetCollisionRadius(float Radius)
{
	CollisionComponent->SetSphereRadius(Radius);
}

void AShooterProjectile::SetBallistics(float Speed, float GravityScale)
{
	if (Speed > 0.0f)
	{
		// no cap, gravity may add to the speed on the way down
		ProjectileMovement->InitialSpeed = Speed;
		ProjectileMovement->MaxSpeed = 0.0f;
		if (!ProjectileMovement->Velocity.IsNearlyZero())
		{
			ProjectileMovement->Velocity = ProjectileMovement->Velocity.GetSafeNormal() * Speed;
		}
	}
	if (GravityScale >= 0.0f)
	{
		ProjectileMovement->ProjectileGravityScale = GravityScale;
	}
}
