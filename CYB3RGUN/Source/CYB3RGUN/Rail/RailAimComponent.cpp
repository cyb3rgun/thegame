// CYB3RGUN THEGAME. The one screen space aiming path (D-019).

#include "RailAimComponent.h"
#include "DoorRangeTarget.h"
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
	PrimaryComponentTick.bCanEverTick = false;
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

bool URailAimComponent::CanFire() const
{
	const UWorld* World = GetWorld();
	return World && !bFireBlocked && (World->GetTimeSeconds() - LastShotTime) >= RefireSeconds;
}

bool URailAimComponent::Fire()
{
	if (!CanFire())
	{
		return false;
	}

	LastShotTime = GetWorld()->GetTimeSeconds();
	++ShotsFired;

	FHitResult Hit;
	const bool bHit = ResolveAim(Hit);
	AActor* Damaged = nullptr;

	if (bHit && Hit.GetActor())
	{
		AActor* HitActor = Hit.GetActor();
		APawn* OwnerPawn = Cast<APawn>(GetOwner());
		AController* Instigator = OwnerPawn ? OwnerPawn->GetController() : nullptr;

		// scoring targets decide themselves whether the shot counts
		if (IDoorRangeTarget* Target = Cast<IDoorRangeTarget>(HitActor))
		{
			Target->NotifyShot(Hit.GetComponent(), Hit.ImpactPoint, Instigator);
		}

		// the engine damage path, which enemies route into their single entry point
		const FVector ShotDirection = (Hit.TraceEnd - Hit.TraceStart).GetSafeNormal();
		const float Applied = UGameplayStatics::ApplyPointDamage(HitActor, Damage, ShotDirection, Hit, Instigator, GetOwner(), DamageTypeClass);
		// walls and props accept engine damage too, only a pawn that took it counts as a hit
		if (Applied > 0.0f && HitActor->IsA<APawn>())
		{
			Damaged = HitActor;
			++ShotsHit;
		}
	}

	UE_LOG(LogRailAim, Verbose, TEXT("Shot %d at screen %.3f %.3f: %s%s"), ShotsFired, GetCrosshairNormalized().X, GetCrosshairNormalized().Y,
		bHit ? *GetNameSafe(Hit.GetActor()) : TEXT("nothing"), Damaged ? TEXT(", damaged") : TEXT(""));

	OnShotFired.Broadcast(bHit, Hit, Damaged);
	return true;
}
