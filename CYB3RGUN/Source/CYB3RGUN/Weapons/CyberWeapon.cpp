// CYB3RGUN THEGAME. The one weapon actor: everything it is comes from its weapon definition (D-052).

#include "CyberWeapon.h"
#include "ShooterProjectile.h"
#include "ShooterWeaponHolder.h"
#include "ShotFeedback.h"
#include "WeaponStatus.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundBase.h"

DEFINE_LOG_CATEGORY_STATIC(LogCyberWeapon, Log, All);

ACyberWeapon::ACyberWeapon()
{
	// the definition's accuracy cone decides where a shot lands, not the template's target offset
	AimVariance = 0.0f;
}

ACyberWeapon* ACyberWeapon::SpawnFor(AActor* Holder, const UWeaponDefinition* InDefinition)
{
	UWorld* World = Holder ? Holder->GetWorld() : nullptr;
	if (!World || !InDefinition)
	{
		return nullptr;
	}

	// the definition is in place before BeginPlay, which builds the body from it
	const FTransform SpawnTransform = Holder->GetActorTransform();
	ACyberWeapon* Weapon = World->SpawnActorDeferred<ACyberWeapon>(ACyberWeapon::StaticClass(), SpawnTransform, Holder, Cast<APawn>(Holder),
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn, ESpawnActorScaleMethod::MultiplyWithRoot);
	if (Weapon)
	{
		Weapon->SetDefinition(const_cast<UWeaponDefinition*>(InDefinition));
		Weapon->FinishSpawning(SpawnTransform);
	}
	return Weapon;
}

void ACyberWeapon::BeginPlay()
{
	// the mesh and the animation classes come first, the base then attaches the meshes to the holder and prints the mark
	if (Definition)
	{
		GetFirstPersonMesh()->SetSkeletalMeshAsset(Definition->Mesh);
		GetThirdPersonMesh()->SetSkeletalMeshAsset(Definition->Mesh);
		FirstPersonAnimInstanceClass = Definition->FirstPersonAnimClass;
		ThirdPersonAnimInstanceClass = Definition->ThirdPersonAnimClass;
		if (Definition->ProjectileClass)
		{
			ProjectileClass = Definition->ProjectileClass;
		}
		MuzzleSocketName = Definition->Mounts.Muzzle.Socket;
	}
	else
	{
		UE_LOG(LogCyberWeapon, Warning, TEXT("%s has no weapon definition and cannot fire"), *GetName());
	}

	Super::BeginPlay();

	State.Init(Definition);
	BuildBody();
	UE_LOG(LogCyberWeapon, Log, TEXT("%s is %s, %d body parts"), *GetName(), *GetDisplayName().ToString(), BodyParts.Num());
}

void ACyberWeapon::BuildBody()
{
	if (!Definition)
	{
		return;
	}

	for (const bool bFirstPerson : { true, false })
	{
		USkeletalMeshComponent* Mesh = bFirstPerson ? GetFirstPersonMesh() : GetThirdPersonMesh();

		// the holder attached the body at its hand, the grip mount belongs there
		Mesh->SetRelativeTransform(GetMountLocal(EWeaponMount::Grip, Mesh).Inverse());

		for (const FWeaponBodyPart& Part : Definition->PlaceholderBody)
		{
			if (!Part.Mesh)
			{
				continue;
			}

			UStaticMeshComponent* Piece = NewObject<UStaticMeshComponent>(this);
			Piece->SetupAttachment(Mesh);
			Piece->SetRelativeTransform(Part.Transform * GetMountLocal(Part.Mount, Mesh));
			Piece->SetStaticMesh(Part.Mesh);
			if (Part.Material)
			{
				Piece->SetMaterial(0, Part.Material);
			}
			Piece->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Piece->SetGenerateOverlapEvents(false);

			// the same split as the meshes: the owner sees the first person body, everyone else and the shadows the other
			if (bFirstPerson)
			{
				Piece->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::FirstPerson);
				Piece->SetOnlyOwnerSee(true);
				Piece->SetCastShadow(false);
			}
			else
			{
				Piece->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::WorldSpaceRepresentation);
				Piece->SetOwnerNoSee(true);
			}
			Piece->RegisterComponent();
			BodyParts.Add(Piece);
		}
	}
}

FTransform ACyberWeapon::GetMountLocal(EWeaponMount Mount, const USkeletalMeshComponent* Mesh) const
{
	if (!Definition)
	{
		return FTransform::Identity;
	}

	// a mesh that carries the socket wins, the placeholder position stands in until one does (D-054)
	const FWeaponMountPoint& Point = Definition->Mounts.Get(Mount);
	if (Mesh && Mesh->GetSkeletalMeshAsset() && !Point.Socket.IsNone() && Mesh->DoesSocketExist(Point.Socket))
	{
		return Mesh->GetSocketTransform(Point.Socket, ERelativeTransformSpace::RTS_Component);
	}
	return Point.Fallback;
}

FTransform ACyberWeapon::GetMountTransform(EWeaponMount Mount, bool bFirstPerson) const
{
	const USkeletalMeshComponent* Mesh = bFirstPerson ? GetFirstPersonMesh() : GetThirdPersonMesh();
	return GetMountLocal(Mount, Mesh) * Mesh->GetComponentTransform();
}

FVector ACyberWeapon::GetMuzzleLocation() const
{
	return GetMountTransform(EWeaponMount::Muzzle, true).GetLocation();
}

void ACyberWeapon::ActivateWeapon(const FName& OwnerTag)
{
	Super::ActivateWeapon(OwnerTag);
	State.Draw();
	LastAction = State.GetAction();
}

void ACyberWeapon::StartFiring()
{
	bIsFiring = true;

	if (State.CanFire())
	{
		FireShot(State.Fire());
		return;
	}

	// a pull on a ready but empty weapon clicks; one while it is drawn, cycles or reloads is lost
	if (State.GetAction() == EWeaponAction::Ready && State.IsEmpty())
	{
		DryFire();
		State.NoteDryFire(LastDryFireTime);
	}
}

void ACyberWeapon::StopFiring()
{
	Super::StopFiring();
}

void ACyberWeapon::FireShot(const FWeaponShot& Shot)
{
	if (!Definition || !WeaponOwner)
	{
		return;
	}

	const FVector Target = WeaponOwner->GetWeaponTargetLocation();
	const bool bFirstPersonView = PawnOwner && PawnOwner->IsLocallyControlled() && PawnOwner->IsPlayerControlled();
	FTransform ShotTransform = CalculateProjectileSpawnTransform(Target);

	if (Shot.Pellets > 1)
	{
		FirePellets(Target);
	}
	else
	{
		// the shot leaves within the cone the weapon holds right now
		const FVector Direction = FMath::VRandCone(ShotTransform.GetRotation().Vector(), FMath::DegreesToRadians(Shot.ConeDegrees));
		ShotTransform.SetRotation(Direction.ToOrientationQuat());
		SpawnShotProjectile(ShotTransform, Shot.Damage, Shot.Speed, Shot.GravityScale);
	}

	// the flash leaves the muzzle mount, attached to the socket where the mesh has one
	USkeletalMeshComponent* MuzzleMesh = bFirstPersonView ? GetFirstPersonMesh() : GetThirdPersonMesh();
	const bool bSocket = MuzzleMesh->GetSkeletalMeshAsset() && !MuzzleSocketName.IsNone() && MuzzleMesh->DoesSocketExist(MuzzleSocketName);
	const FVector MuzzleLocation = GetMountTransform(EWeaponMount::Muzzle, bFirstPersonView).GetLocation();
	UShotFeedback::PlayMuzzleFlash(this, MuzzleMesh, bSocket ? MuzzleSocketName : NAME_None, MuzzleLocation, ShotTransform.Rotator(), bFirstPersonView);

	PlayWeaponSound(Definition->FireSound);
	if (State.GetAction() == EWeaponAction::Cycling)
	{
		PlayWeaponSound(Definition->CycleSound);
	}

	if (PawnOwner)
	{
		MakeNoise(ShotLoudness, PawnOwner, PawnOwner->GetActorLocation(), ShotNoiseRange, NoiseOwnerTag);
	}
	WeaponOwner->PlayFiringMontage(FiringMontage);
	ApplyRecoil();
	WeaponOwner->UpdateWeaponHUD(State.GetRounds(), Definition->MagazineSize);
}

AController* ACyberWeapon::GetHolderController() const
{
	return PawnOwner ? PawnOwner->GetController() : nullptr;
}

void ACyberWeapon::ApplyRecoil()
{
	AController* Controller = GetHolderController();
	if (!Controller || !Definition || (Definition->RecoilPitchDegrees <= 0.0f && Definition->RecoilYawDegrees <= 0.0f))
	{
		return;
	}

	FRotator Aim = Controller->GetControlRotation();
	Aim.Pitch = FRotator::NormalizeAxis(Aim.Pitch + Definition->RecoilPitchDegrees);
	Aim.Yaw += FMath::FRandRange(-Definition->RecoilYawDegrees, Definition->RecoilYawDegrees);
	Controller->SetControlRotation(Aim);

	// the recovery brings back its share of every climb within the recovery time
	RecoilToRecover += Definition->RecoilPitchDegrees * Definition->RecoilRecoveryShare;
	RecoilRecoveryRate = RecoilToRecover / FMath::Max(Definition->RecoilRecoverySeconds, 0.01f);
}

void ACyberWeapon::RecoverRecoil(float Delta)
{
	if (RecoilToRecover <= 0.0f)
	{
		return;
	}

	AController* Controller = GetHolderController();
	if (!Controller)
	{
		RecoilToRecover = 0.0f;
		return;
	}

	const float Step = FMath::Min(RecoilToRecover, RecoilRecoveryRate * Delta);
	FRotator Aim = Controller->GetControlRotation();
	Aim.Pitch = FRotator::NormalizeAxis(Aim.Pitch - Step);
	Controller->SetControlRotation(Aim);
	RecoilToRecover -= Step;
}

bool ACyberWeapon::StartReload()
{
	if (!Definition || !State.StartReload())
	{
		return false;
	}

	bIsFiring = false;
	LastAction = State.GetAction();
	PlayWeaponSound(Definition->ReloadStartSound);
	UE_LOG(LogCyberWeapon, Log, TEXT("%s reloads, %d of %d rounds left, %.2f s"), *GetDisplayName().ToString(), State.GetRounds(), Definition->MagazineSize, Definition->ReloadSeconds);
	WeaponOwner->UpdateWeaponHUD(State.GetRounds(), Definition->MagazineSize);
	return true;
}

void ACyberWeapon::CancelReload()
{
	if (State.GetAction() == EWeaponAction::Reloading)
	{
		State.CancelReload();
		LastAction = State.GetAction();
		UE_LOG(LogCyberWeapon, Log, TEXT("%s reload cancelled with %d rounds"), *GetDisplayName().ToString(), State.GetRounds());
	}
}

void ACyberWeapon::FillStatus(FWeaponStatus& OutStatus) const
{
	State.FillStatus(OutStatus);
}

void ACyberWeapon::Tick(float DeltaSeconds)
{
	// the weapon's own rules replace the template's reload and equip timing, only its clock is shared
	AActor::Tick(DeltaSeconds);

	const float Delta = AdvanceClock(DeltaSeconds);
	State.Tick(Delta);
	RecoverRecoil(Delta);

	const EWeaponAction Action = State.GetAction();
	if (LastAction == EWeaponAction::Reloading && Action == EWeaponAction::Ready && Definition)
	{
		PlayWeaponSound(Definition->ReloadEndSound);
		UE_LOG(LogCyberWeapon, Log, TEXT("%s reloaded, %d rounds"), *GetDisplayName().ToString(), State.GetRounds());
		WeaponOwner->UpdateWeaponHUD(State.GetRounds(), Definition->MagazineSize);
	}
	LastAction = Action;
}
