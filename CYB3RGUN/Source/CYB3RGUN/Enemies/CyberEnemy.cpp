// CYB3RGUN THEGAME. Base actor for every enemy.

#include "CyberEnemy.h"
#include "CyberEnemyController.h"
#include "EnemyDefinition.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DamageEvents.h"
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

	// the skeletal mesh stays empty until real characters exist
	GetMesh()->SetVisibility(false);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

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

	BuildPlaceholder();
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

	// measure between capsule edges so wide enemies do not need to overlap the target
	float TargetRadius = 0.0f;
	if (const ACharacter* TargetCharacter = Cast<ACharacter>(Target))
	{
		TargetRadius = TargetCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius();
	}
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

	// placeholder lunge: a short forward scale punch through the flinch path
	Flinch();

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
	DeathPoseElapsed = 0.0f;
	SetActorTickEnabled(true);

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
	PlaceholderRoot->SetRelativeScale3D(FVector(1.15f, 1.15f, 0.92f));
	GetWorld()->GetTimerManager().SetTimer(FlinchTimer, this, &ACyberEnemy::ClearFlinch, 0.12f, false);
}

void ACyberEnemy::ClearFlinch()
{
	PlaceholderRoot->SetRelativeScale3D(FVector::OneVector);
}

void ACyberEnemy::PlayEnemySound(USoundBase* Sound) const
{
	if (Sound && Definition)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Sound, GetActorLocation(), 1.0f, Definition->Sounds.Pitch);
	}
}
