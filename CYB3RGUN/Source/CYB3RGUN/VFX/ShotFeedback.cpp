// CYB3RGUN THEGAME. Muzzle flashes and impacts with light, spawned by every weapon path.

#include "ShotFeedback.h"
#include "ShotFeedbackSettings.h"
#include "ShotFlashLight.h"
#include "Camera/CameraTypes.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/SceneComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

namespace
{
	UWorld* FeedbackWorld(const UObject* WorldContextObject)
	{
		UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
		// a dedicated server draws nothing
		return (World && World->GetNetMode() != NM_DedicatedServer) ? World : nullptr;
	}

	/**
	 *  Where a point on a first person primitive appears, for something drawn without the first person settings.
	 *  First person primitives are pulled towards the camera by the first person scale and drawn with their own
	 *  field of view; the returned point has the pulled in depth and lands on the same spot on screen.
	 */
	FVector ApparentFirstPersonLocation(UWorld* World, const FVector& WorldLocation)
	{
		const APlayerController* Player = World->GetFirstPlayerController();
		const APlayerCameraManager* CameraManager = Player ? Player->PlayerCameraManager.Get() : nullptr;
		if (!CameraManager)
		{
			return WorldLocation;
		}

		const FMinimalViewInfo& View = CameraManager->GetCameraCacheView();
		if (!View.bUseFirstPersonParameters || View.FirstPersonFOV <= 1.0f)
		{
			return WorldLocation;
		}

		const FTransform ViewTransform(View.Rotation, View.Location);
		FVector Local = ViewTransform.InverseTransformPosition(WorldLocation);
		const float Spread = FMath::Tan(FMath::DegreesToRadians(View.FOV * 0.5f)) / FMath::Tan(FMath::DegreesToRadians(View.FirstPersonFOV * 0.5f));
		Local.X *= View.FirstPersonScale;
		Local.Y *= View.FirstPersonScale * Spread;
		Local.Z *= View.FirstPersonScale * Spread;
		return ViewTransform.TransformPosition(Local);
	}
}

void UShotFeedback::PlayMuzzleFlash(const UObject* WorldContextObject, USceneComponent* Muzzle, FName Socket, FVector Location, FRotator Direction, bool bFirstPerson)
{
	const UShotFeedbackSettings* Settings = UShotFeedbackSettings::Get();
	UWorld* World = FeedbackWorld(WorldContextObject);
	if (!Settings->bEnabled || !World)
	{
		return;
	}

	if (UNiagaraSystem* System = Settings->MuzzleFlashSystem.LoadSynchronous())
	{
		if (bFirstPerson)
		{
			// drawn as a first person primitive the flash does not line up with the first person weapon, so it is
			// placed where that weapon's muzzle appears on screen and drawn as an ordinary world effect
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, System, ApparentFirstPersonLocation(World, Location), Direction);
		}
		else if (Muzzle)
		{
			// the flash keeps its world placement and aim at spawn, then rides along with the weapon
			UNiagaraFunctionLibrary::SpawnSystemAttached(System, Muzzle, Socket, Location, Direction, EAttachLocation::KeepWorldPosition, true);
		}
		else
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, System, Location, Direction);
		}
	}

	// the light sits at the real muzzle, lighting does not care how the weapon is drawn
	SpawnFlashLight(World, Location, Settings->MuzzleLightColor, Settings->MuzzleLightIntensity, Settings->MuzzleLightRadius, Settings->MuzzleLightDuration);
}

void UShotFeedback::PlayImpact(const UObject* WorldContextObject, FVector Location, FVector Normal)
{
	const UShotFeedbackSettings* Settings = UShotFeedbackSettings::Get();
	UWorld* World = FeedbackWorld(WorldContextObject);
	if (!Settings->bEnabled || !World)
	{
		return;
	}

	const FVector Outward = Normal.GetSafeNormal();
	if (UNiagaraSystem* System = Settings->ImpactSystem.LoadSynchronous())
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, System, Location, Outward.IsNearlyZero() ? FRotator::ZeroRotator : Outward.Rotation());
	}

	SpawnFlashLight(World, Location + Outward * Settings->ImpactLightOffset, Settings->ImpactLightColor, Settings->ImpactLightIntensity, Settings->ImpactLightRadius, Settings->ImpactLightDuration);
}

void UShotFeedback::SpawnFlashLight(UWorld* World, const FVector& Location, const FLinearColor& Color, float Lumens, float Radius, float Duration)
{
	if (Lumens <= 0.0f)
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (AShotFlashLight* FlashLight = World->SpawnActor<AShotFlashLight>(Location, FRotator::ZeroRotator, Params))
	{
		FlashLight->Flash(Color, Lumens, Radius, Duration);
	}
}
