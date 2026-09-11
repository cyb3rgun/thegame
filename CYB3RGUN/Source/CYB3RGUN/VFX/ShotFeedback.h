// CYB3RGUN THEGAME. Muzzle flashes and impacts with light, spawned by every weapon path.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Engine/HitResult.h"
#include "ShotFeedback.generated.h"

class USceneComponent;

/**
 *  One place that turns a shot into something visible. The projectile weapons and the rail aim both call in,
 *  the look lives in UShotFeedbackSettings.
 */
UCLASS()
class CYB3RGUN_API UShotFeedback : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/**
	 *  Muzzle flash and its light at Location, pointing along Direction. With a muzzle component the flash follows
	 *  it for its short life. bFirstPerson means Location is on a first person weapon of the local player: the
	 *  flash is then placed where that muzzle appears on screen, the light stays at the real muzzle.
	 */
	UFUNCTION(BlueprintCallable, Category="Shot Feedback", meta = (WorldContext = "WorldContextObject"))
	static void PlayMuzzleFlash(const UObject* WorldContextObject, USceneComponent* Muzzle, FName Socket, FVector Location, FRotator Direction, bool bFirstPerson);

	/** Sparks where a shot lands and a brief light just off the surface */
	UFUNCTION(BlueprintCallable, Category="Shot Feedback", meta = (WorldContext = "WorldContextObject"))
	static void PlayImpact(const UObject* WorldContextObject, FVector Location, FVector Normal);

	/** A mark on the surface where a shot lands. Pawns, skinned bodies and hidden hit volumes get none. */
	UFUNCTION(BlueprintCallable, Category="Shot Feedback", meta = (WorldContext = "WorldContextObject"))
	static void PlayImpactDecal(const UObject* WorldContextObject, const FHitResult& Hit);

private:

	static void SpawnFlashLight(UWorld* World, const FVector& Location, const FLinearColor& Color, float Lumens, float Radius, float Duration);
};
