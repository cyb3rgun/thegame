// CYB3RGUN THEGAME. The one screen space aiming path (D-019).

#include "RailAimComponent.h"
#include "StyleScoringComponent.h"
#include "DoorRangeTarget.h"
#include "ShotFeedback.h"
#include "WeaponDefinition.h"
#include "CoreGlobals.h"
#include "Misc/App.h"
#include "Sound/SoundBase.h"
#include "CombatFeelSubsystem.h"
#include "Camera/CameraComponent.h"
#include "CollisionQueryParams.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogRailAim, Log, All);

URailAimComponent::URailAimComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	DamageTypeClass = UDamageType::StaticClass();
}

APlayerController* URailAimComponent::GetPlayerController() const
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	return Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
}

FVector2D URailAimComponent::ClampToScreen(FVector2D Normalized) const
{
	return FVector2D(
		FMath::Clamp(Normalized.X, EdgeMargin, 1.0 - EdgeMargin),
		FMath::Clamp(Normalized.Y, EdgeMargin, 1.0 - EdgeMargin));
}

void URailAimComponent::SetCrosshairNormalized(FVector2D Normalized)
{
	CrosshairNormalized = ClampToScreen(Normalized);
}

void URailAimComponent::SetCrosshairScreenPosition(FVector2D Pixels)
{
	int32 SizeX = 0;
	int32 SizeY = 0;
	if (const APlayerController* PC = GetPlayerController())
	{
		PC->GetViewportSize(SizeX, SizeY);
	}
	if (SizeX > 0 && SizeY > 0)
	{
		SetCrosshairNormalized(FVector2D(Pixels.X / SizeX, Pixels.Y / SizeY));
	}
}

void URailAimComponent::AddAimInput(FVector2D NormalizedDelta)
{
	SetCrosshairNormalized(CrosshairNormalized + NormalizedDelta);
}

FVector2D URailAimComponent::GetCrosshairNormalized() const
{
	switch (InputMode)
	{
	case ERailAimInputMode::ScreenCenter:
		return FVector2D(0.5, 0.5);

	case ERailAimInputMode::AbsoluteCursor:
		if (const APlayerController* PC = GetPlayerController())
		{
			double X = 0.0;
			double Y = 0.0;
			int32 SizeX = 0;
			int32 SizeY = 0;
			PC->GetViewportSize(SizeX, SizeY);
			if (SizeX > 0 && SizeY > 0 && PC->GetMousePosition(X, Y))
			{
				return ClampToScreen(FVector2D(X / SizeX, Y / SizeY));
			}
		}
		return CrosshairNormalized;

	case ERailAimInputMode::Relative:
	default:
		return CrosshairNormalized;
	}
}

bool URailAimComponent::GetCrosshairScreenPosition(FVector2D& OutPixels) const
{
	const APlayerController* PC = GetPlayerController();
	if (!PC)
	{
		return false;
	}

	int32 SizeX = 0;
	int32 SizeY = 0;
	PC->GetViewportSize(SizeX, SizeY);
	if (SizeX <= 0 || SizeY <= 0)
	{
		return false;
	}

	const FVector2D Normalized = GetCrosshairNormalized();
	OutPixels = FVector2D(Normalized.X * SizeX, Normalized.Y * SizeY);
	return true;
}

bool URailAimComponent::ResolveAim(FHitResult& OutHit) const
{
	const APlayerController* PC = GetPlayerController();
	FVector2D Pixels;
	if (!PC || !GetCrosshairScreenPosition(Pixels))
	{
		return false;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(RailAim), false, GetOwner());
	return PC->GetHitResultAtScreenPosition(Pixels, TraceChannel, QueryParams, OutHit);
}

FVector URailAimComponent::ResolveAimPoint() const
{
	FHitResult Hit;
	if (ResolveAim(Hit))
	{
		return Hit.ImpactPoint;
	}

	// nothing under the crosshair: a far point along the same ray, so a projectile still flies where the crosshair points
	const APlayerController* PC = GetPlayerController();
	FVector2D Pixels;
	FVector Origin;
	FVector Direction;
	if (PC && GetCrosshairScreenPosition(Pixels) && PC->DeprojectScreenPositionToWorld(Pixels.X, Pixels.Y, Origin, Direction))
	{
		return Origin + Direction * PC->HitResultTraceDistance;
	}

	return GetOwner() ? GetOwner()->GetActorLocation() + GetOwner()->GetActorForwardVector() * 10000.0f : FVector::ZeroVector;
}

FWeaponState* URailAimComponent::GetCurrentState()
{
	return States.IsValidIndex(WeaponIndex) ? &States[WeaponIndex] : nullptr;
}

const FWeaponState* URailAimComponent::GetCurrentState() const
{
	return States.IsValidIndex(WeaponIndex) ? &States[WeaponIndex] : nullptr;
}

bool URailAimComponent::CanFire() const
{
	const UWorld* World = GetWorld();
	if (!World || bFireBlocked)
	{
		return false;
	}

	// carried weapons follow their own rules, on the aim's clock, see TickComponent
	if (const FWeaponState* State = GetCurrentState())
	{
		return State->CanFire();
	}
	return (World->GetTimeSeconds() - LastShotTime) >= RefireSeconds;
}

bool URailAimComponent::TraceShot(float ConeDegrees, FHitResult& OutHit) const
{
	// a settled weapon hits exactly what the crosshair shows
	if (ConeDegrees <= 0.0f)
	{
		return ResolveAim(OutHit);
	}

	const APlayerController* PC = GetPlayerController();
	FVector2D Pixels;
	FVector Origin;
	FVector Direction;
	if (!PC || !GetCrosshairScreenPosition(Pixels) || !PC->DeprojectScreenPositionToWorld(Pixels.X, Pixels.Y, Origin, Direction))
	{
		return false;
	}

	// otherwise the shot leaves the crosshair ray within the cone the weapon holds right now
	const FVector ShotDirection = FMath::VRandCone(Direction, FMath::DegreesToRadians(ConeDegrees));
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(RailAim), false, GetOwner());
	return GetWorld()->LineTraceSingleByChannel(OutHit, Origin, Origin + ShotDirection * PC->HitResultTraceDistance, TraceChannel, QueryParams);
}

bool URailAimComponent::Fire()
{
	const UWeaponDefinition* Weapon = GetCurrentWeapon();
	FWeaponState* State = GetCurrentState();

	// an empty weapon clicks, unless cover or its own action holds the trigger anyway
	if (State && !bFireBlocked && State->GetAction() == EWeaponAction::Ready && State->IsEmpty())
	{
		DryFire();
		return false;
	}

	if (!CanFire())
	{
		return false;
	}

	UWorld* World = GetWorld();
	LastShotTime = World->GetTimeSeconds();
	++ShotsFired;

	FWeaponShot Shot;
	if (State)
	{
		Shot = State->Fire();
	}
	else
	{
		Shot.Damage = Damage;
	}

	// the style record resolves this shot right here: whatever it lands on reports a hit, anything else makes it a miss
	UStyleScoringComponent* Style = UStyleScoringComponent::Get(GetOwner());
	if (Style)
	{
		Style->RecordShotFired();
		Style->BeginShotResolution();
	}

	FHitResult Hit;
	bool bHit = false;
	AActor* Damaged = nullptr;

	if (Weapon && Weapon->Pellets > 1)
	{
		bHit = ResolveAim(Hit);
		Damaged = FirePellets(*Weapon);
	}
	else
	{
		bHit = TraceShot(Shot.ConeDegrees, Hit);
		if (bHit)
		{
			Damaged = ApplyShotHit(Hit, Shot.Damage * (Weapon ? Weapon->GetDamageScale(Hit.Distance) : 1.0f));
			UShotFeedback::PlayImpact(this, Hit.ImpactPoint, Hit.ImpactNormal);
			UShotFeedback::PlayImpactDecal(this, Hit);
		}
	}

	if (Damaged)
	{
		++ShotsHit;
	}

	if (Style)
	{
		Style->EndShotResolution();
	}

	// the rail has no weapon model: the flash sits low and right of the camera, where a held weapon would be
	if (const UCameraComponent* Camera = GetOwner() ? GetOwner()->FindComponentByClass<UCameraComponent>() : nullptr)
	{
		const FVector Muzzle = Camera->GetComponentTransform().TransformPosition(FVector(60.0f, 18.0f, -22.0f));
		const FVector AimTarget = bHit ? Hit.ImpactPoint : ResolveAimPoint();
		UShotFeedback::PlayMuzzleFlash(this, nullptr, NAME_None, Muzzle, (AimTarget - Muzzle).Rotation(), false);
	}

	if (Weapon)
	{
		PlayWeaponSound(Weapon->FireSound);
		if (State && State->GetAction() == EWeaponAction::Cycling)
		{
			PlayWeaponSound(Weapon->CycleSound);
		}
	}

	UE_LOG(LogRailAim, Verbose, TEXT("Shot %d at screen %.3f %.3f: %s%s"), ShotsFired, GetCrosshairNormalized().X, GetCrosshairNormalized().Y,
		bHit ? *GetNameSafe(Hit.GetActor()) : TEXT("nothing"), Damaged ? TEXT(", damaged") : TEXT(""));

	OnShotFired.Broadcast(bHit, Hit, Damaged);
	return true;
}

AActor* URailAimComponent::ApplyShotHit(const FHitResult& Hit, float ShotDamage)
{
	AActor* HitActor = Hit.GetActor();
	if (!HitActor)
	{
		return nullptr;
	}

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	AController* Instigator = OwnerPawn ? OwnerPawn->GetController() : nullptr;

	// scoring targets decide themselves whether the shot counts and which zone it landed in
	const FVector ShotDirection = (Hit.TraceEnd - Hit.TraceStart).GetSafeNormal();
	if (IDoorRangeTarget* Target = Cast<IDoorRangeTarget>(HitActor))
	{
		Target->NotifyShot(Hit, ShotDirection, Instigator);
	}

	// the engine damage path, which enemies route into their single entry point
	const float Applied = UGameplayStatics::ApplyPointDamage(HitActor, ShotDamage, ShotDirection, Hit, Instigator, GetOwner(), DamageTypeClass);

	// walls and props accept engine damage too, only a pawn that took it counts as a hit
	return Applied > 0.0f && HitActor->IsA<APawn>() ? HitActor : nullptr;
}

AActor* URailAimComponent::FirePellets(const UWeaponDefinition& Weapon)
{
	const APlayerController* PC = GetPlayerController();
	FVector2D Pixels;
	FVector Origin;
	FVector Direction;
	if (!PC || !GetCrosshairScreenPosition(Pixels) || !PC->DeprojectScreenPositionToWorld(Pixels.X, Pixels.Y, Origin, Direction))
	{
		return nullptr;
	}

	// every pellet is its own trace through a cone around the crosshair ray, the shot still resolves once for style
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(RailPellet), false, GetOwner());
	const float HalfAngle = FMath::DegreesToRadians(Weapon.SpreadDegrees);
	AActor* FirstDamaged = nullptr;
	for (int32 Pellet = 0; Pellet < Weapon.Pellets; ++Pellet)
	{
		const FVector PelletDirection = FMath::VRandCone(Direction, HalfAngle);
		FHitResult PelletHit;
		if (!GetWorld()->LineTraceSingleByChannel(PelletHit, Origin, Origin + PelletDirection * Weapon.MaxRange, TraceChannel, QueryParams))
		{
			continue;
		}

		AActor* PelletDamaged = ApplyShotHit(PelletHit, Weapon.Damage * Weapon.GetDamageScale(PelletHit.Distance));
		FirstDamaged = FirstDamaged ? FirstDamaged : PelletDamaged;
		UShotFeedback::PlayImpact(this, PelletHit.ImpactPoint, PelletHit.ImpactNormal);
		UShotFeedback::PlayImpactDecal(this, PelletHit);
	}
	return FirstDamaged;
}

void URailAimComponent::BeginPlay()
{
	Super::BeginPlay();

	Weapons.RemoveAll([](const TObjectPtr<UWeaponDefinition>& Weapon) { return !Weapon; });
	States.Reset(Weapons.Num());
	for (const UWeaponDefinition* Weapon : Weapons)
	{
		States.AddDefaulted_GetRef().Init(Weapon);
	}
	WeaponIndex = 0;

	// only carried weapons have actions to time
	SetComponentTickEnabled(!Weapons.IsEmpty());
}

void URailAimComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// the aim runs on the rider's clock: the wall clock at the player's Overclock scale, so reloads, switches and refire
	// keep the player's pace while the world is slowed. A long gap, a pause or a hitch, counts as one short frame
	const double WallNow = FPlatformTime::Seconds();
	const float WallDelta = LastTickWallSeconds > 0.0 ? static_cast<float>(FMath::Min(WallNow - LastTickWallSeconds, 0.1)) : 0.0f;
	LastTickWallSeconds = WallNow;
	const float ClockDelta = WallDelta * UCombatFeelSubsystem::GetPlayerTimeScale(this);

	FWeaponState* State = GetCurrentState();
	const UWeaponDefinition* Weapon = GetCurrentWeapon();
	if (!State || !Weapon)
	{
		return;
	}

	const EWeaponAction Before = State->GetAction();
	State->Tick(ClockDelta);
	if (Before == EWeaponAction::Reloading && State->GetAction() == EWeaponAction::Ready)
	{
		PlayWeaponSound(Weapon->ReloadEndSound);
		UE_LOG(LogRailAim, Log, TEXT("%s reloaded in cover, %d rounds, %.2f s on the wall clock"), *Weapon->DisplayName.ToString(), State->GetRounds(), FPlatformTime::Seconds() - ReloadStartSeconds);
	}
}

void URailAimComponent::SetWeapons(const TArray<UWeaponDefinition*>& InWeapons)
{
	Weapons.Reset(InWeapons.Num());
	for (UWeaponDefinition* Weapon : InWeapons)
	{
		Weapons.Add(Weapon);
	}
}

const UWeaponDefinition* URailAimComponent::GetCurrentWeapon() const
{
	return Weapons.IsValidIndex(WeaponIndex) ? Weapons[WeaponIndex].Get() : nullptr;
}

bool URailAimComponent::SwitchWeapon()
{
	if (Weapons.Num() < 2 || States.Num() != Weapons.Num())
	{
		return false;
	}

	CancelReload();
	WeaponIndex = (WeaponIndex + 1) % Weapons.Num();

	const UWeaponDefinition* Weapon = GetCurrentWeapon();
	FWeaponState* State = GetCurrentState();
	State->Draw();
	PlayWeaponSound(Weapon->EquipSound);
	UE_LOG(LogRailAim, Log, TEXT("Switched to %s, %d of %d rounds"), *Weapon->DisplayName.ToString(), State->GetRounds(), Weapon->MagazineSize);
	return true;
}

bool URailAimComponent::StartReload()
{
	const UWeaponDefinition* Weapon = GetCurrentWeapon();
	FWeaponState* State = GetCurrentState();
	if (!Weapon || !State || !State->StartReload())
	{
		return false;
	}

	ReloadStartSeconds = FPlatformTime::Seconds();
	PlayWeaponSound(Weapon->ReloadStartSound);
	UE_LOG(LogRailAim, Log, TEXT("%s reloads in cover, %d of %d rounds left, %.2f s"), *Weapon->DisplayName.ToString(), State->GetRounds(), Weapon->MagazineSize, Weapon->ReloadSeconds);
	return true;
}

void URailAimComponent::CancelReload()
{
	FWeaponState* State = GetCurrentState();
	if (!State || State->GetAction() != EWeaponAction::Reloading)
	{
		return;
	}

	State->CancelReload();
	const UWeaponDefinition* Weapon = GetCurrentWeapon();
	UE_LOG(LogRailAim, Log, TEXT("%s reload left unfinished with %d of %d rounds"), Weapon ? *Weapon->DisplayName.ToString() : TEXT("Weapon"),
		State->GetRounds(), Weapon ? Weapon->MagazineSize : 0);
}

bool URailAimComponent::IsReloading() const
{
	const FWeaponState* State = GetCurrentState();
	return State && State->GetAction() == EWeaponAction::Reloading;
}

bool URailAimComponent::GetWeaponStatus(FWeaponStatus& OutStatus) const
{
	const FWeaponState* State = GetCurrentState();
	if (!State)
	{
		return false;
	}

	State->FillStatus(OutStatus);
	OutStatus.WeaponIndex = WeaponIndex;
	OutStatus.WeaponCount = Weapons.Num();
	return true;
}

void URailAimComponent::GetLoadout(TArray<FWeaponStatus>& OutLoadout) const
{
	for (int32 Index = 0; Index < States.Num(); ++Index)
	{
		FWeaponStatus& Entry = OutLoadout.AddDefaulted_GetRef();
		States[Index].FillStatus(Entry);
		Entry.WeaponIndex = Index;
		Entry.WeaponCount = States.Num();
	}
}

void URailAimComponent::DryFire()
{
	const UWeaponDefinition* Weapon = GetCurrentWeapon();
	if (FWeaponState* State = GetCurrentState())
	{
		State->NoteDryFire(GetWorld()->GetRealTimeSeconds());
	}
	if (Weapon)
	{
		PlayWeaponSound(Weapon->EmptySound);
	}
	UE_LOG(LogRailAim, Log, TEXT("%s clicks empty"), Weapon ? *Weapon->DisplayName.ToString() : TEXT("Weapon"));
}

void URailAimComponent::PlayWeaponSound(USoundBase* Sound) const
{
	// the rider hears their own weapon in the head, not placed in the world
	if (Sound)
	{
		UGameplayStatics::PlaySound2D(this, Sound);
	}
}
