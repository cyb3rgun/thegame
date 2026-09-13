// Copyright Epic Games, Inc. All Rights Reserved.


#include "ShooterCharacter.h"
#include "ShooterWeapon.h"
#include "CyberWeapon.h"
#include "EnhancedInputComponent.h"
#include "Components/InputComponent.h"
#include "Components/PawnNoiseEmitterComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "Camera/CameraComponent.h"
#include "TimerManager.h"
#include "ShooterGameMode.h"
#include "RailAimComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "UObject/ConstructorHelpers.h"
#include "CombatFeelSubsystem.h"
#include "WeaponDefinition.h"

AShooterCharacter::AShooterCharacter()
{
	// create the noise emitter component
	PawnNoiseEmitter = CreateDefaultSubobject<UPawnNoiseEmitterComponent>(TEXT("Pawn Noise Emitter"));

	// create the screen space aim component, first person style with the crosshair in the centre
	AimComponent = CreateDefaultSubobject<URailAimComponent>(TEXT("Aim"));
	AimComponent->SetInputMode(ERailAimInputMode::ScreenCenter);

	// reload lives in the project's input folder, the Blueprint may still override it
	static ConstructorHelpers::FObjectFinder<UInputAction> ReloadInput(TEXT("/Game/CYB3RGUN/Core/Input/IA_Reload.IA_Reload"));
	ReloadAction = ReloadInput.Object;
	static ConstructorHelpers::FObjectFinder<UInputAction> OverclockInput(TEXT("/Game/CYB3RGUN/Core/Input/IA_Overclock.IA_Overclock"));
	OverclockAction = OverclockInput.Object;

	// configure movement
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 600.0f, 0.0f);
}

void AShooterCharacter::BeginPlay()
{
	Super::BeginPlay();

	// reset HP to max
	CurrentHP = MaxHP;

	// update the HUD
	OnDamaged.Broadcast(1.0f);
}

void AShooterCharacter::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// clear the respawn timer
	GetWorld()->GetTimerManager().ClearTimer(RespawnTimer);
}

void AShooterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// base class handles move, aim and jump inputs
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Firing
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &AShooterCharacter::DoStartFiring);
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &AShooterCharacter::DoStopFiring);

		// Switch weapon
		EnhancedInputComponent->BindAction(SwitchWeaponAction, ETriggerEvent::Triggered, this, &AShooterCharacter::DoSwitchWeapon);

		// Reload
		if (ReloadAction)
		{
			EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Started, this, &AShooterCharacter::DoReload);
		}

		// Overclock, held
		if (OverclockAction)
		{
			EnhancedInputComponent->BindAction(OverclockAction, ETriggerEvent::Started, this, &AShooterCharacter::OverclockPressed);
			EnhancedInputComponent->BindAction(OverclockAction, ETriggerEvent::Completed, this, &AShooterCharacter::OverclockReleased);
		}
	}

}

float AShooterCharacter::TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	// ignore if already dead
	if (CurrentHP <= 0.0f)
	{
		return 0.0f;
	}

	// Reduce HP
	CurrentHP -= Damage;

	// the projected HUD glitches for a moment (D-051)
	if (UCombatFeelSubsystem* Feel = UCombatFeelSubsystem::Get(this))
	{
		Feel->NotifyPlayerDamaged(Damage);
	}

	// Have we depleted HP?
	if (CurrentHP <= 0.0f)
	{
		Die();
	}

	// update the HUD
	OnDamaged.Broadcast(FMath::Max(0.0f, CurrentHP / MaxHP));

	return Damage;
}

void AShooterCharacter::DoAim(float Yaw, float Pitch)
{
	// only route inputs if the character is not dead
	if (!IsDead())
	{
		Super::DoAim(Yaw, Pitch);
	}
}

void AShooterCharacter::DoMove(float Right, float Forward)
{
	// only route inputs if the character is not dead
	if (!IsDead())
	{
		Super::DoMove(Right, Forward);
	}
}

void AShooterCharacter::DoJumpStart()
{
	// only route inputs if the character is not dead
	if (!IsDead())
	{
		Super::DoJumpStart();
	}
}

void AShooterCharacter::DoJumpEnd()
{
	// only route inputs if the character is not dead
	if (!IsDead())
	{
		Super::DoJumpEnd();
	}
}

void AShooterCharacter::DoStartFiring()
{
	// fire the current weapon
	if (CurrentWeapon && !IsDead())
	{
		CurrentWeapon->StartFiring();
	}
}

void AShooterCharacter::DoStopFiring()
{
	// stop firing the current weapon
	if (CurrentWeapon && !IsDead())
	{
		CurrentWeapon->StopFiring();
	}
}

void AShooterCharacter::DoSwitchWeapon()
{
	// ensure we have at least two weapons two switch between
	if (OwnedWeapons.Num() > 1 && !IsDead())
	{
		// deactivate the old weapon
		CurrentWeapon->DeactivateWeapon();

		// find the index of the current weapon in the owned list
		int32 WeaponIndex = OwnedWeapons.Find(CurrentWeapon);

		// is this the last weapon?
		if (WeaponIndex == OwnedWeapons.Num() - 1)
		{
			// loop back to the beginning of the array
			WeaponIndex = 0;
		}
		else {
			// select the next weapon index
			++WeaponIndex;
		}

		// set the new weapon as current
		CurrentWeapon = OwnedWeapons[WeaponIndex];

		// activate the new weapon
		CurrentWeapon->ActivateWeapon(PlayerTag);
	}
}

void AShooterCharacter::AttachWeaponMeshes(AShooterWeapon* Weapon)
{
	const FAttachmentTransformRules AttachmentRule(EAttachmentRule::SnapToTarget, false);

	// attach the weapon actor
	Weapon->AttachToActor(this, AttachmentRule);

	// attach the weapon meshes
	Weapon->GetFirstPersonMesh()->AttachToComponent(GetFirstPersonMesh(), AttachmentRule, FirstPersonWeaponSocket);
	Weapon->GetThirdPersonMesh()->AttachToComponent(GetMesh(), AttachmentRule, FirstPersonWeaponSocket);
	
}

void AShooterCharacter::PlayFiringMontage(UAnimMontage* Montage)
{
	// stub
}

void AShooterCharacter::AddWeaponRecoil(float Recoil)
{
	// apply the recoil as pitch input
	AddControllerPitchInput(Recoil);
}

void AShooterCharacter::UpdateWeaponHUD(int32 CurrentAmmo, int32 MagazineSize)
{
	OnBulletCountUpdated.Broadcast(MagazineSize, CurrentAmmo);
}

FVector AShooterCharacter::GetWeaponTargetLocation()
{
	// players aim through the one screen space path (D-019), the projectile flies toward the point under the crosshair
	if (AimComponent && Cast<APlayerController>(GetController()))
	{
		return AimComponent->ResolveAimPoint();
	}

	// characters without a screen, such as AI shooters, keep the camera trace
	// trace ahead from the camera viewpoint
	FHitResult OutHit;

	const FVector Start = GetFirstPersonCameraComponent()->GetComponentLocation();
	const FVector End = Start + (GetFirstPersonCameraComponent()->GetForwardVector() * MaxAimDistance);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	GetWorld()->LineTraceSingleByChannel(OutHit, Start, End, ECC_Visibility, QueryParams);

	// return either the impact point or the trace end
	return OutHit.bBlockingHit ? OutHit.ImpactPoint : OutHit.TraceEnd;
}

void AShooterCharacter::AddWeaponClass(const TSubclassOf<AShooterWeapon>& WeaponClass)
{
	// do we already own this weapon?
	AShooterWeapon* OwnedWeapon = FindWeaponOfType(WeaponClass);

	if (!OwnedWeapon)
	{
		// spawn the new weapon
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParams.TransformScaleMethod = ESpawnActorScaleMethod::MultiplyWithRoot;

		AShooterWeapon* AddedWeapon = GetWorld()->SpawnActor<AShooterWeapon>(WeaponClass, GetActorTransform(), SpawnParams);

		if (AddedWeapon)
		{
			// add the weapon to the owned list
			OwnedWeapons.Add(AddedWeapon);

			// if we have an existing weapon, deactivate it
			if (CurrentWeapon)
			{
				CurrentWeapon->DeactivateWeapon();
			}

			// switch to the new weapon
			CurrentWeapon = AddedWeapon;
			CurrentWeapon->ActivateWeapon(PlayerTag);
		}
	}
}

void AShooterCharacter::AddWeaponDefinition(const UWeaponDefinition* Definition)
{
	if (!Definition)
	{
		return;
	}

	// one of each weapon
	for (const AShooterWeapon* Owned : OwnedWeapons)
	{
		if (Owned && Owned->GetDefinition() == Definition)
		{
			return;
		}
	}

	ACyberWeapon* AddedWeapon = ACyberWeapon::SpawnFor(this, Definition);
	if (!AddedWeapon)
	{
		return;
	}

	// weapons switch in the order they were handed out; the first comes up in hand, the others wait holstered
	OwnedWeapons.Add(AddedWeapon);
	if (CurrentWeapon)
	{
		AddedWeapon->DeactivateWeapon();
		return;
	}
	CurrentWeapon = AddedWeapon;
	CurrentWeapon->ActivateWeapon(PlayerTag);
}

void AShooterCharacter::OnWeaponActivated(AShooterWeapon* Weapon)
{
	// update the bullet counter
	FWeaponStatus Status;
	Weapon->FillStatus(Status);
	OnBulletCountUpdated.Broadcast(Status.MagazineSize, Status.Rounds);

	// set the character mesh AnimInstances
	GetFirstPersonMesh()->SetAnimInstanceClass(Weapon->GetFirstPersonAnimInstanceClass());
	GetMesh()->SetAnimInstanceClass(Weapon->GetThirdPersonAnimInstanceClass());
}

void AShooterCharacter::OnWeaponDeactivated(AShooterWeapon* Weapon)
{
	// unused
}

void AShooterCharacter::OnSemiWeaponRefire()
{
	// unused
}

AShooterWeapon* AShooterCharacter::FindWeaponOfType(TSubclassOf<AShooterWeapon> WeaponClass) const
{
	// check each owned weapon
	for (AShooterWeapon* Weapon : OwnedWeapons)
	{
		if (Weapon->IsA(WeaponClass))
		{
			return Weapon;
		}
	}

	// weapon not found
	return nullptr;

}

void AShooterCharacter::Die()
{
	// deactivate the weapon
	if (IsValid(CurrentWeapon))
	{
		CurrentWeapon->DeactivateWeapon();
	}

	// increment the team score
	if (AShooterGameMode* GM = Cast<AShooterGameMode>(GetWorld()->GetAuthGameMode()))
	{
		GM->IncrementTeamScore(TeamByte);
	}

	// grant the death tag to the character
	Tags.Add(DeathTag);
		
	// stop character movement
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();

	// disable collision
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// disable controls
	DisableInput(nullptr);

	// reset the bullet counter UI
	OnBulletCountUpdated.Broadcast(0, 0);

	// call the BP handler
	BP_OnDeath();
	OnDied.Broadcast(this);

	// schedule character respawn
	GetWorld()->GetTimerManager().SetTimer(RespawnTimer, this, &AShooterCharacter::OnRespawn, RespawnTime, false);
}

void AShooterCharacter::OnRespawn()
{
	// destroy the character to force the PC to respawn
	Destroy();
}

bool AShooterCharacter::IsDead() const
{
	// the character is dead if their current HP drops to zero
	return CurrentHP <= 0.0f;
}

void AShooterCharacter::SetTeam(uint8 Team)
{
	TeamByte = Team;
}

void AShooterCharacter::DoReload()
{
	if (CurrentWeapon && !IsDead())
	{
		CurrentWeapon->StartReload();
	}
}

void AShooterCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();

	const APlayerController* PC = Cast<APlayerController>(GetController());
	UEnhancedInputLocalPlayerSubsystem* Subsystem = PC ? ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()) : nullptr;
	if (!Subsystem)
	{
		return;
	}

	// reload on R and the gamepad's left face button, switching also on Q and the mouse wheel, Overclock on E and the left shoulder
	if (!CombatMappingContext)
	{
		CombatMappingContext = NewObject<UInputMappingContext>(this, TEXT("CombatMappingContext"));
		if (ReloadAction)
		{
			CombatMappingContext->MapKey(ReloadAction, EKeys::R);
			CombatMappingContext->MapKey(ReloadAction, EKeys::Gamepad_FaceButton_Left);
		}
		if (SwitchWeaponAction)
		{
			CombatMappingContext->MapKey(SwitchWeaponAction, EKeys::Q);
			CombatMappingContext->MapKey(SwitchWeaponAction, EKeys::MouseScrollUp);
			CombatMappingContext->MapKey(SwitchWeaponAction, EKeys::MouseScrollDown);
		}
		if (OverclockAction)
		{
			CombatMappingContext->MapKey(OverclockAction, EKeys::E);
			CombatMappingContext->MapKey(OverclockAction, EKeys::Gamepad_LeftShoulder);
		}
	}
	Subsystem->AddMappingContext(CombatMappingContext, 1);
}

bool AShooterCharacter::GetWeaponStatus(FWeaponStatus& OutStatus) const
{
	if (!CurrentWeapon)
	{
		return false;
	}

	CurrentWeapon->FillStatus(OutStatus);
	OutStatus.ReloadHint = NSLOCTEXT("ShooterCharacter", "ReloadHint", "R OR X TO RELOAD");
	OutStatus.WeaponIndex = OwnedWeapons.Find(CurrentWeapon.Get());
	OutStatus.WeaponCount = OwnedWeapons.Num();
	return true;
}

void AShooterCharacter::GetLoadout(TArray<FWeaponStatus>& OutLoadout) const
{
	for (int32 Index = 0; Index < OwnedWeapons.Num(); ++Index)
	{
		const AShooterWeapon* Weapon = OwnedWeapons[Index];
		if (!Weapon)
		{
			continue;
		}
		FWeaponStatus& Entry = OutLoadout.AddDefaulted_GetRef();
		Weapon->FillStatus(Entry);
		Entry.WeaponIndex = Index;
		Entry.WeaponCount = OwnedWeapons.Num();
	}
}

void AShooterCharacter::OverclockPressed()
{
	if (UCombatFeelSubsystem* Feel = UCombatFeelSubsystem::Get(this))
	{
		Feel->SetOverclockHeld(true);
	}
}

void AShooterCharacter::OverclockReleased()
{
	if (UCombatFeelSubsystem* Feel = UCombatFeelSubsystem::Get(this))
	{
		Feel->SetOverclockHeld(false);
	}
}
