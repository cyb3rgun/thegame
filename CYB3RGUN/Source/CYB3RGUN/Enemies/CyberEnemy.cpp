// CYB3RGUN THEGAME. Base actor for every enemy.

#include "CyberEnemy.h"
#include "CyberEnemyController.h"
#include "EnemyDefinition.h"
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

ACyberEnemy::ACyberEnemy()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	AIControllerClass = ACyberEnemyController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// the skeletal mesh shows only for definitions with a body; shots hit the capsule, never the mesh
	GetMesh()->SetVisibility(false);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;

	PlaceholderRoot = CreateDefaultSubobject<USceneComponent>(TEXT("PlaceholderRoot"));
	PlaceholderRoot->SetupAttachment(GetCapsuleComponent());

	GetCapsuleComponent()->SetCollisionProfileName(FName("Pawn"));
	// weapons aim with visibility traces, the capsule must stop them or shots converge behind the enemy
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
}

bool ACyberEnemy::HasBody() const
{
	return Definition && Definition->BodyMesh;
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

	Body->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	Body->SetVisibility(true);
	BodyLoop = nullptr;
	OneShotUntil = 0.0f;
	UpdateBodyAnimation();
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
	if (!Wanted)
	{
		return;
	}

	USkeletalMeshComponent* Body = GetMesh();
	if (Wanted != BodyLoop || !Body->IsPlaying())
	{
		Body->PlayAnimation(Wanted, true);
		BodyLoop = Wanted;
	}
	Body->SetPlayRate(bMoving ? FMath::Clamp(Speed / Definition->MoveAnimationSpeed, 0.5f, 2.5f) : 1.0f);
}

void ACyberEnemy::PlayBodyOneShot(UAnimSequenceBase* Animation)
{
	if (!Animation || !HasBody())
	{
		return;
	}

	GetMesh()->PlayAnimation(Animation, false);
	GetMesh()->SetPlayRate(1.0f);
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
	GetCharacterMovement()->MaxWalkSpeed = FMath::Max(Speed, 0.0f);
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
	}
	else if (DamageEvent.DamageTypeClass && DamageEvent.DamageTypeClass->GetName().Contains(TEXT("Fire")))
	{
		Source = EEnemyDamageSource::Fire;
	}

	return ApplyEnemyDamage(Damage, Source, EventInstigator, DamageCauser);
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
		Flinch();
		if (Definition)
		{
			PlayEnemySound(Definition->Sounds.Hurt);
		}
		BP_OnHitReaction(Applied, Source);
	}

	return Applied;
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
	if (HasBody())
	{
		// the body falls with a death animation and holds its last frame
		const TArray<TObjectPtr<UAnimSequenceBase>>& Deaths = Definition->DeathAnimations;
		if (Deaths.Num() > 0)
		{
			if (UAnimSequenceBase* Death = Deaths[FMath::RandRange(0, Deaths.Num() - 1)])
			{
				GetMesh()->PlayAnimation(Death, false);
				GetMesh()->SetPlayRate(1.0f);
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
