// Copyright Epic Games, Inc. All Rights Reserved.


#include "ShooterWeapon.h"
#include "StyleScoringComponent.h"
#include "StyleSettings.h"
#include "WeaponDefinition.h"
#include "CoreGlobals.h"
#include "DoorRangeTarget.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/App.h"
#include "Sound/SoundBase.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/World.h"
#include "ShooterProjectile.h"
#include "ShooterWeaponHolder.h"
#include "ShotFeedback.h"
#include "Components/SceneComponent.h"
#include "TimerManager.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "CollisionQueryParams.h"

DEFINE_LOG_CATEGORY_STATIC(LogShooterWeapon, Log, All);

AShooterWeapon::AShooterWeapon()
{
	PrimaryActorTick.bCanEverTick = true;

	// create the root
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	// create the first person mesh
	FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("First Person Mesh"));
	FirstPersonMesh->SetupAttachment(RootComponent);

	FirstPersonMesh->SetCollisionProfileName(FName("NoCollision"));
	FirstPersonMesh->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::FirstPerson);
	FirstPersonMesh->bOnlyOwnerSee = true;

	// create the third person mesh
	ThirdPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Third Person Mesh"));
	ThirdPersonMesh->SetupAttachment(RootComponent);

	ThirdPersonMesh->SetCollisionProfileName(FName("NoCollision"));
	ThirdPersonMesh->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::WorldSpaceRepresentation);
	ThirdPersonMesh->bOwnerNoSee = true;
}

void AShooterWeapon::BeginPlay()
{
	Super::BeginPlay();

	// subscribe to the owner's destroyed delegate
	GetOwner()->OnDestroyed.AddDynamic(this, &AShooterWeapon::OnOwnerDestroyed);

	// cast the weapon owner
	WeaponOwner = Cast<IShooterWeaponHolder>(GetOwner());
	PawnOwner = Cast<APawn>(GetOwner());

	// the definition decides the handling, the Blueprint values are the fallback
	if (Definition)
	{
		MagazineSize = Definition->MagazineSize;
		RefireRate = Definition->RefireSeconds;
		bFullAuto = Definition->bFullAuto;
		AimVariance = Definition->BulletAimVariance;
	}

	// fill the first ammo clip
	CurrentBullets = MagazineSize;

	// attach the meshes to the owner
	WeaponOwner->AttachWeaponMeshes(this);
}

void AShooterWeapon::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// clear the refire timer
	GetWorld()->GetTimerManager().ClearTimer(RefireTimer);
}

void AShooterWeapon::OnOwnerDestroyed(AActor* DestroyedActor)
{
	// ensure this weapon is destroyed when the owner is destroyed
	Destroy();
}

void AShooterWeapon::ActivateWeapon(const FName& OwnerTag)
{
	// save the owner tag for perception noise detection
	NoiseOwnerTag = OwnerTag;

	// unhide this weapon
	SetActorHiddenInGame(false);

	// a weapon switched to needs a moment before it can fire
	EquipRemaining = GetEquipDuration();
	EquipStartFrame = GFrameCounter;
	if (Definition)
	{
		PlayWeaponSound(Definition->EquipSound);
	}

	// notify the owner
	WeaponOwner->OnWeaponActivated(this);
}

void AShooterWeapon::DeactivateWeapon()
{
	// ensure we're no longer firing this weapon while deactivated
	StopFiring();

	// switching away leaves a reload unfinished
	CancelReload();

	// hide the weapon
	SetActorHiddenInGame(true);

	// notify the owner
	WeaponOwner->OnWeaponDeactivated(this);
}

void AShooterWeapon::StartFiring()
{
	// a weapon that is coming up or reloading does not fire, the trigger pull is lost
	if (bReloading || IsEquipping())
	{
		return;
	}

	// an empty magazine only clicks: reloading is the player's decision (D-045)
	if (CurrentBullets <= 0)
	{
		DryFire();
		return;
	}

	// raise the firing flag
	bIsFiring = true;

	// refire runs on real time, so the weapon keeps the player's pace while the world is slowed
	const double TimeSinceLastShot = GetWorld()->GetRealTimeSeconds() - LastShotRealTime;

	if (TimeSinceLastShot > RefireRate)
	{
		// fire the weapon right away
		Fire();

	} else {

		// if we're full auto, schedule the next shot once the refire time is up
		if (bFullAuto)
		{
			GetWorld()->GetTimerManager().SetTimer(RefireTimer, this, &AShooterWeapon::Fire, FMath::Max(RefireRate - static_cast<float>(TimeSinceLastShot), 0.01f), false);
		}

	}
}

void AShooterWeapon::StopFiring()
{
	// lower the firing flag
	bIsFiring = false;

	// clear the refire timer
	GetWorld()->GetTimerManager().ClearTimer(RefireTimer);
}

void AShooterWeapon::Fire()
{
	// ensure the player still wants to fire. They may have let go of the trigger
	if (!bIsFiring)
	{
		return;
	}
	
	// a full auto weapon that ran dry stops with a click
	if (CurrentBullets <= 0)
	{
		DryFire();
		StopFiring();
		return;
	}

	// fire a projectile at the target
	FireProjectile(WeaponOwner->GetWeaponTargetLocation());

	// update the time of our last shot
	TimeOfLastShot = GetWorld()->GetTimeSeconds();
	LastShotRealTime = GetWorld()->GetRealTimeSeconds();

	// make noise so the AI perception system can hear us
	MakeNoise(ShotLoudness, PawnOwner, PawnOwner->GetActorLocation(), ShotNoiseRange, NoiseOwnerTag);

	// are we full auto?
	if (bFullAuto)
	{
		// schedule the next shot
		GetWorld()->GetTimerManager().SetTimer(RefireTimer, this, &AShooterWeapon::Fire, RefireRate, false);
	} else {

		// for semi-auto weapons, schedule the cooldown notification
		GetWorld()->GetTimerManager().SetTimer(RefireTimer, this, &AShooterWeapon::FireCooldownExpired, RefireRate, false);

	}
}

void AShooterWeapon::FireCooldownExpired()
{
	// notify the owner
	WeaponOwner->OnSemiWeaponRefire();
}

void AShooterWeapon::FireProjectile(const FVector& TargetLocation)
{
	// get the projectile transform
	FTransform ProjectileTransform = CalculateProjectileSpawnTransform(TargetLocation);
	
	// a weapon with several pellets fires them as hitscan traces from the view, every other weapon a projectile
	if (Definition && Definition->Pellets > 1)
	{
		FirePellets(TargetLocation);
	}
	else
	{
		SpawnShotProjectile(ProjectileTransform);
	}

	// muzzle flash and its light where the shot leaves; the local player sees it on the first person weapon
	const bool bFirstPersonView = PawnOwner && PawnOwner->IsLocallyControlled() && PawnOwner->IsPlayerControlled();
	USkeletalMeshComponent* MuzzleMesh = bFirstPersonView ? FirstPersonMesh : ThirdPersonMesh;
	UShotFeedback::PlayMuzzleFlash(this, MuzzleMesh, MuzzleSocketName, MuzzleMesh->GetSocketLocation(MuzzleSocketName), ProjectileTransform.Rotator(), bFirstPersonView);

	// play the firing montage
	WeaponOwner->PlayFiringMontage(FiringMontage);

	// add recoil
	WeaponOwner->AddWeaponRecoil(FiringRecoil);

	// consume a round. A player's magazine stays empty until the player reloads (D-045), other shooters refill at once
	--CurrentBullets;
	if (CurrentBullets <= 0 && !IsPlayerWeapon())
	{
		CurrentBullets = MagazineSize;
	}

	// update the weapon HUD
	WeaponOwner->UpdateWeaponHUD(CurrentBullets, MagazineSize);
}

FTransform AShooterWeapon::CalculateProjectileSpawnTransform(const FVector& TargetLocation) const
{
	// find the muzzle location
	const FVector MuzzleLoc = FirstPersonMesh->GetSocketLocation(MuzzleSocketName);

	// calculate the spawn location ahead of the muzzle
	FVector SpawnLoc = MuzzleLoc + ((TargetLocation - MuzzleLoc).GetSafeNormal() * MuzzleOffset);

	// point blank: when something already sits between the owner and the spawn point, a projectile spawned there
	// starts inside it and its first sweep never reports the hit. Start from the owner's view origin instead so
	// the sweep enters the obstacle from the front (D-021)
	if (PawnOwner)
	{
		const FVector SafeOrigin = PawnOwner->GetPawnViewLocation();

		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ShooterWeaponPointBlank), false, this);
		QueryParams.AddIgnoredActor(PawnOwner);

		FHitResult Blocker;
		if (GetWorld()->SweepSingleByChannel(Blocker, SafeOrigin, SpawnLoc, FQuat::Identity, ECC_WorldDynamic, FCollisionShape::MakeSphere(PointBlankProbeRadius), QueryParams))
		{
			UE_LOG(LogShooterWeapon, Verbose, TEXT("Point blank: %s blocks the muzzle path, projectile starts at the view origin"), *GetNameSafe(Blocker.GetActor()));
			SpawnLoc = SafeOrigin;
		}
	}

	// find the aim rotation vector while applying some variance to the target 
	const FRotator AimRot = UKismetMathLibrary::FindLookAtRotation(SpawnLoc, TargetLocation + (UKismetMathLibrary::RandomUnitVector() * AimVariance));

	// return the built transform
	return FTransform(AimRot, SpawnLoc, FVector::OneVector);
}

const TSubclassOf<UAnimInstance>& AShooterWeapon::GetFirstPersonAnimInstanceClass() const
{
	return FirstPersonAnimInstanceClass;
}

const TSubclassOf<UAnimInstance>& AShooterWeapon::GetThirdPersonAnimInstanceClass() const
{
	return ThirdPersonAnimInstanceClass;
}

void AShooterWeapon::SpawnShotProjectile(const FTransform& ProjectileTransform)
{
	// spawn the projectile
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.TransformScaleMethod = ESpawnActorScaleMethod::OverrideRootScale;
	SpawnParams.Owner = GetOwner();
	SpawnParams.Instigator = PawnOwner;

	AShooterProjectile* Projectile = GetWorld()->SpawnActor<AShooterProjectile>(ProjectileClass, ProjectileTransform, SpawnParams);

	// set the noise tag on the projectile, and the definition's damage
	if (Projectile)
	{
		Projectile->SetNoiseTag(NoiseOwnerTag);
		if (Definition)
		{
			Projectile->SetHitDamage(Definition->Damage);
			if (Definition->BulletRadius > 0.0f)
			{
				Projectile->SetCollisionRadius(Definition->BulletRadius);
			}
		}
	}

	// a player's shot counts for style: fired now, resolved where it lands or when it times out
	if (UStyleScoringComponent* Style = UStyleScoringComponent::Get(PawnOwner.Get()))
	{
		Style->RecordShotFired();
		if (Projectile)
		{
			Projectile->MarkAsStyleShot(UStyleSettings::Get(this)->ShotMissTimeout);
		}
	}
}

void AShooterWeapon::FirePellets(const FVector& TargetLocation)
{
	UWorld* World = GetWorld();
	const FVector Origin = PawnOwner ? PawnOwner->GetPawnViewLocation() : FirstPersonMesh->GetSocketLocation(MuzzleSocketName);
	const FVector Aim = (TargetLocation - Origin).GetSafeNormal();
	const float HalfAngle = FMath::DegreesToRadians(Definition->SpreadDegrees);
	AController* InstigatorController = PawnOwner ? PawnOwner->GetController() : nullptr;

	// one trigger pull is one shot for the style record, however many pellets land
	UStyleScoringComponent* Style = UStyleScoringComponent::Get(PawnOwner.Get());
	if (Style)
	{
		Style->RecordShotFired();
		Style->BeginShotResolution();
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ShooterWeaponPellet), true, this);
	QueryParams.AddIgnoredActor(PawnOwner);

	int32 ImpactsShown = 0;
	for (int32 Pellet = 0; Pellet < Definition->Pellets; ++Pellet)
	{
		const FVector Direction = FMath::VRandCone(Aim, HalfAngle);
		FHitResult Hit;
		if (!World->LineTraceSingleByChannel(Hit, Origin, Origin + Direction * Definition->MaxRange, ECC_Visibility, QueryParams))
		{
			continue;
		}

		AActor* HitActor = Hit.GetActor();
		if (IDoorRangeTarget* Target = Cast<IDoorRangeTarget>(HitActor))
		{
			Target->NotifyShot(Hit.GetComponent(), Hit.ImpactPoint, InstigatorController);
		}
		if (HitActor && HitActor != PawnOwner)
		{
			const float Damage = Definition->Damage * Definition->GetDamageScale(Hit.Distance);
			UGameplayStatics::ApplyPointDamage(HitActor, Damage, Direction, Hit, InstigatorController, this, UDamageType::StaticClass());
		}

		// a few impacts are enough to read the spread, a light for every pellet would only cost frames
		if (ImpactsShown < 3)
		{
			UShotFeedback::PlayImpact(this, Hit.ImpactPoint, Hit.ImpactNormal);
			++ImpactsShown;
		}
	}

	if (Style)
	{
		Style->EndShotResolution();
	}
}

bool AShooterWeapon::StartReload()
{
	if (bReloading || IsEquipping() || CurrentBullets >= MagazineSize)
	{
		return false;
	}

	StopFiring();
	bReloading = true;
	ReloadElapsed = 0.0f;
	ReloadStartFrame = GFrameCounter;
	ReloadStartSeconds = FPlatformTime::Seconds();
	if (Definition)
	{
		PlayWeaponSound(Definition->ReloadStartSound);
	}
	UE_LOG(LogShooterWeapon, Log, TEXT("%s reloads, %d of %d rounds left, %.2f s"), *GetName(), CurrentBullets, MagazineSize, GetReloadDuration());
	WeaponOwner->UpdateWeaponHUD(CurrentBullets, MagazineSize);
	return true;
}

void AShooterWeapon::CancelReload()
{
	if (bReloading)
	{
		bReloading = false;
		ReloadElapsed = 0.0f;
		UE_LOG(LogShooterWeapon, Log, TEXT("%s reload cancelled with %d of %d rounds"), *GetName(), CurrentBullets, MagazineSize);
	}
}

float AShooterWeapon::GetReloadProgress() const
{
	const float Duration = GetReloadDuration();
	return bReloading && Duration > 0.0f ? FMath::Clamp(ReloadElapsed / Duration, 0.0f, 1.0f) : 0.0f;
}

void AShooterWeapon::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// reloads and switches run on real time, so they keep the player's pace while the world is slowed
	const float RealDelta = static_cast<float>(FApp::GetDeltaTime());

	if (EquipRemaining > 0.0f && GFrameCounter != EquipStartFrame)
	{
		EquipRemaining = FMath::Max(EquipRemaining - RealDelta, 0.0f);
	}

	if (bReloading && GFrameCounter != ReloadStartFrame)
	{
		ReloadElapsed += RealDelta;
		if (ReloadElapsed >= GetReloadDuration())
		{
			bReloading = false;
			ReloadElapsed = 0.0f;
			CurrentBullets = MagazineSize;
			if (Definition)
			{
				PlayWeaponSound(Definition->ReloadEndSound);
			}
			UE_LOG(LogShooterWeapon, Log, TEXT("%s reloaded, %d rounds, %.2f s on the wall clock"), *GetName(), CurrentBullets, FPlatformTime::Seconds() - ReloadStartSeconds);
			WeaponOwner->UpdateWeaponHUD(CurrentBullets, MagazineSize);
		}
	}
}

void AShooterWeapon::DryFire()
{
	LastDryFireTime = GetWorld()->GetRealTimeSeconds();
	if (Definition)
	{
		PlayWeaponSound(Definition->EmptySound);
	}
	UE_LOG(LogShooterWeapon, Log, TEXT("%s clicks empty"), *GetName());
}

void AShooterWeapon::PlayWeaponSound(USoundBase* Sound) const
{
	// the player hears their own weapon in the head, not placed in the world
	if (Sound && IsPlayerWeapon())
	{
		UGameplayStatics::PlaySound2D(this, Sound);
	}
}

float AShooterWeapon::GetReloadDuration() const
{
	return Definition ? Definition->ReloadSeconds : ReloadSeconds;
}

float AShooterWeapon::GetEquipDuration() const
{
	return Definition ? Definition->EquipSeconds : 0.25f;
}

bool AShooterWeapon::IsPlayerWeapon() const
{
	return PawnOwner && PawnOwner->IsPlayerControlled();
}

FText AShooterWeapon::GetDisplayName() const
{
	return Definition && !Definition->DisplayName.IsEmpty() ? Definition->DisplayName : FText::FromString(GetClass()->GetName());
}
