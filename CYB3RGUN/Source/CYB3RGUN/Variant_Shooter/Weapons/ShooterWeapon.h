// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShooterWeaponHolder.h"
#include "Animation/AnimInstance.h"
#include "ShooterWeapon.generated.h"

class IShooterWeaponHolder;
class AShooterProjectile;
class USkeletalMeshComponent;
class UAnimMontage;
class UAnimInstance;
class USoundBase;
class UWeaponDefinition;

/**
 *  Base class for a simple first person shooter weapon
 *  Provides both first person and third person perspective meshes
 *  Handles ammo and firing logic
 *  Interacts with the weapon owner through the ShooterWeaponHolder interface
 */
UCLASS(abstract)
class CYB3RGUN_API AShooterWeapon : public AActor
{
	GENERATED_BODY()
	
	/** First person perspective mesh */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* FirstPersonMesh;

	/** Third person perspective mesh */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* ThirdPersonMesh;

protected:

	/** Cast pointer to the weapon owner */
	IShooterWeaponHolder* WeaponOwner;

	/** Type of projectiles this weapon will shoot */
	UPROPERTY(EditAnywhere, Category="Ammo")
	TSubclassOf<AShooterProjectile> ProjectileClass;

	/** Number of bullets in a magazine */
	UPROPERTY(EditAnywhere, Category="Ammo", meta = (ClampMin = 0, ClampMax = 100))
	int32 MagazineSize = 10;

	/** Number of bullets in the current magazine */
	int32 CurrentBullets = 0;

	/** Handling shared with the rail: magazine, reload, refire, damage and pellets. Overrides the values here when set. */
	UPROPERTY(EditAnywhere, Category="Weapon")
	TObjectPtr<UWeaponDefinition> Definition;

	/** Seconds from starting a reload to a full magazine, used without a definition */
	UPROPERTY(EditAnywhere, Category="Ammo", meta = (ClampMin = 0, ClampMax = 10, Units = "s"))
	float ReloadSeconds = 1.4f;

	/** True while the magazine is being refilled, the weapon does not fire meanwhile */
	bool bReloading = false;

	/** Real seconds spent on the current reload */
	float ReloadElapsed = 0.0f;

	/** Real seconds until a weapon just switched to can fire */
	float EquipRemaining = 0.0f;

	/** Frames the reload and the equip started on. That frame's delta ran before they began and does not count. */
	uint64 ReloadStartFrame = 0;
	uint64 EquipStartFrame = 0;

	/** Wall clock at the start of the reload, the log reports the duration it measured */
	double ReloadStartSeconds = 0.0;

	/** Real time of the last shot and of the last trigger pull on an empty magazine */
	double LastShotRealTime = -1000.0;
	double LastDryFireTime = -1000.0;
	
	/** Animation montage to play when firing this weapon */
	UPROPERTY(EditAnywhere, Category="Animation")
	UAnimMontage* FiringMontage;

	/** AnimInstance class to set for the first person character mesh when this weapon is active */
	UPROPERTY(EditAnywhere, Category="Animation")
	TSubclassOf<UAnimInstance> FirstPersonAnimInstanceClass;

	/** AnimInstance class to set for the third person character mesh when this weapon is active */
	UPROPERTY(EditAnywhere, Category="Animation")
	TSubclassOf<UAnimInstance> ThirdPersonAnimInstanceClass;

	/** Cone half-angle for variance while aiming */
	UPROPERTY(EditAnywhere, Category="Aim", meta = (ClampMin = 0, ClampMax = 90, Units = "Degrees"))
	float AimVariance = 0.0f;

	/** Amount of firing recoil to apply to the owner */
	UPROPERTY(EditAnywhere, Category="Aim", meta = (ClampMin = 0, ClampMax = 100))
	float FiringRecoil = 0.0f;

	/** Name of the first person muzzle socket where projectiles will spawn */
	UPROPERTY(EditAnywhere, Category="Aim")
	FName MuzzleSocketName;

	/** Distance ahead of the muzzle that bullets will spawn at */
	UPROPERTY(EditAnywhere, Category="Aim", meta = (ClampMin = 0, ClampMax = 1000, Units = "cm"))
	float MuzzleOffset = 10.0f;

	/** Radius of the probe between the owner's view origin and the projectile spawn point. Anything it touches is point blank. */
	UPROPERTY(EditAnywhere, Category="Aim", meta = (ClampMin = 0, ClampMax = 50, Units = "cm"))
	float PointBlankProbeRadius = 4.0f;

	/** If true, this weapon will automatically fire at the refire rate */
	UPROPERTY(EditAnywhere, Category="Refire")
	bool bFullAuto = false;

	/** Time between shots for this weapon. Affects both full auto and semi auto modes */
	UPROPERTY(EditAnywhere, Category="Refire", meta = (ClampMin = 0, ClampMax = 5, Units = "s"))
	float RefireRate = 0.5f;

	/** Game time of last shot fired, used to enforce refire rate on semi auto */
	float TimeOfLastShot = 0.0f;

	/** If true, the weapon is currently firing */
	bool bIsFiring = false;

	/** Timer to handle full auto refiring */
	FTimerHandle RefireTimer;

	/** Cast pawn pointer to the owner for AI perception system interactions */
	TObjectPtr<APawn> PawnOwner;

	/** Loudness of the shot for AI perception system interactions */
	UPROPERTY(EditAnywhere, Category="Perception", meta = (ClampMin = 0, ClampMax = 100))
	float ShotLoudness = 1.0f;

	/** Max range of shot AI perception noise */
	UPROPERTY(EditAnywhere, Category="Perception", meta = (ClampMin = 0, ClampMax = 100000, Units = "cm"))
	float ShotNoiseRange = 300.0f;

	/** Tag to apply to noise generated by shooting this weapon */
	UPROPERTY(EditAnywhere, Category="Perception")
	FName NoiseOwnerTag = FName("Shot");

public:	

	/** Constructor */
	AShooterWeapon();

protected:
	
	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Gameplay Cleanup */
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

	/** Runs reloads and weapon switches on real time */
	virtual void Tick(float DeltaSeconds) override;

protected:

	/** Called when the weapon's owner is destroyed */
	UFUNCTION()
	void OnOwnerDestroyed(AActor* DestroyedActor);

public:

	/** Activates this weapon and gets it ready to fire */
	void ActivateWeapon(const FName& OwnerTag);

	/** Deactivates this weapon */
	void DeactivateWeapon();

	/** Start firing this weapon */
	void StartFiring();

	/** Stop firing this weapon */
	void StopFiring();

protected:

	/** Fire the weapon */
	virtual void Fire();

	/** Called when the refire rate time has passed while shooting semi auto weapons */
	void FireCooldownExpired();

	/** Fire a projectile towards the target location */
	virtual void FireProjectile(const FVector& TargetLocation);

	/** Calculates the spawn transform for projectiles shot by this weapon */
	FTransform CalculateProjectileSpawnTransform(const FVector& TargetLocation) const;

	/** Spawns the projectile of a single bullet shot */
	void SpawnShotProjectile(const FTransform& ProjectileTransform);

	/** Fires the definition's pellets as hitscan traces in a cone from the view */
	void FirePellets(const FVector& TargetLocation);

	/** A trigger pull on an empty magazine: the click, and a time stamp for the HUD cue */
	void DryFire();

	void PlayWeaponSound(USoundBase* Sound) const;
	float GetReloadDuration() const;
	float GetEquipDuration() const;

	/** True when a player holds this weapon. Only a player's magazine runs dry, other shooters refill at once. */
	bool IsPlayerWeapon() const;

public:

	/** Returns the first person mesh */
	UFUNCTION(BlueprintPure, Category="Weapon")
	USkeletalMeshComponent* GetFirstPersonMesh() const { return FirstPersonMesh; };

	/** Returns the third person mesh */
	UFUNCTION(BlueprintPure, Category="Weapon")
	USkeletalMeshComponent* GetThirdPersonMesh() const { return ThirdPersonMesh; };

	/** Returns the first person anim instance class */
	const TSubclassOf<UAnimInstance>& GetFirstPersonAnimInstanceClass() const;

	/** Returns the third person anim instance class */
	const TSubclassOf<UAnimInstance>& GetThirdPersonAnimInstanceClass() const;

	/** Returns the magazine size */
	int32 GetMagazineSize() const { return MagazineSize; };

	/** Returns the current bullet count */
	int32 GetBulletCount() const { return CurrentBullets; }

	/** Starts refilling the magazine. False when it is full, already reloading or the weapon is still coming up. */
	bool StartReload();

	/** Stops a reload before it finished, the magazine keeps what it had */
	void CancelReload();

	bool IsReloading() const { return bReloading; }

	/** 0 to 1 while reloading */
	float GetReloadProgress() const;

	/** True while a weapon just switched to comes up and cannot fire */
	bool IsEquipping() const { return EquipRemaining > 0.0f; }

	double GetLastDryFireTime() const { return LastDryFireTime; }

	const UWeaponDefinition* GetDefinition() const { return Definition; }

	FText GetDisplayName() const;
};
