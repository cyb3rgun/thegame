// CYB3RGUN THEGAME. The one screen space aiming path (D-019): a crosshair position resolved to a world hit.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/HitResult.h"
#include "RailAimComponent.generated.h"

class APlayerController;
class UDamageType;

/** Where the crosshair position comes from */
UENUM(BlueprintType)
enum class ERailAimInputMode : uint8
{
	/** Look input moves the crosshair across the screen. Mouse and gamepad. */
	Relative,
	/** The crosshair sits on the OS cursor. Light gun replacements (Sinden, GUN4IR, AimTrak) present themselves this way. */
	AbsoluteCursor,
	/** Fixed at the screen centre, first person style */
	ScreenCenter
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FRailShotFiredDelegate, bool, bHit, const FHitResult&, Hit, AActor*, DamagedActor);

/**
 *  Resolves the crosshair to a world hit through APlayerController::GetHitResultAtScreenPosition and fires
 *  hitscan shots along it. Hits go through the engine damage path, which enemies route into their single
 *  damage entry point. Muzzles never decide a hit; they are cosmetic only.
 *  Works on any pawn: the rail pawn uses Relative input, a first person character uses ScreenCenter,
 *  hardware aiming calls SetCrosshairNormalized with its own screen point.
 */
UCLASS(ClassGroup=(CYB3RGUN), meta=(BlueprintSpawnableComponent))
class CYB3RGUN_API URailAimComponent : public UActorComponent
{
	GENERATED_BODY()

protected:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Aim")
	ERailAimInputMode InputMode = ERailAimInputMode::Relative;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Aim")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	/** Keeps the crosshair this far inside the screen edges, as a fraction of the screen */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Aim", meta = (ClampMin = 0.0, ClampMax = 0.4))
	float EdgeMargin = 0.02f;

	/** Crosshair position, 0 to 1 across the viewport. Used by Relative mode and by hardware. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Aim")
	FVector2D CrosshairNormalized = FVector2D(0.5, 0.5);

	/** Damage per hit. Matches the template pistol bullet until weapons become data. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shot", meta = (ClampMin = 0.0))
	float Damage = 25.0f;

	/** Seconds between two shots. Matches the template pistol until weapons become data. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shot", meta = (ClampMin = 0.0, Units = "s"))
	float RefireSeconds = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shot")
	TSubclassOf<UDamageType> DamageTypeClass;

	/** Set by the owner while it must not shoot, for example in cover */
	bool bFireBlocked = false;

	float LastShotTime = -1000.0f;
	int32 ShotsFired = 0;
	int32 ShotsHit = 0;

public:

	/** Every shot, hit or miss. DamagedActor is set when the hit applied damage. */
	UPROPERTY(BlueprintAssignable, Category="Shot")
	FRailShotFiredDelegate OnShotFired;

public:

	URailAimComponent();

	UFUNCTION(BlueprintCallable, Category="Aim")
	void SetInputMode(ERailAimInputMode NewMode) { InputMode = NewMode; }

	UFUNCTION(BlueprintPure, Category="Aim")
	ERailAimInputMode GetInputMode() const { return InputMode; }

	/** Places the crosshair directly, 0 to 1 across the viewport. The entry point for light guns and gyro aiming. */
	UFUNCTION(BlueprintCallable, Category="Aim")
	void SetCrosshairNormalized(FVector2D Normalized);

	/** Places the crosshair at a viewport pixel position */
	UFUNCTION(BlueprintCallable, Category="Aim")
	void SetCrosshairScreenPosition(FVector2D Pixels);

	/** Moves the crosshair by a delta in viewport fractions. Used by Relative mode. */
	UFUNCTION(BlueprintCallable, Category="Aim")
	void AddAimInput(FVector2D NormalizedDelta);

	/** Crosshair position for the current input mode, 0 to 1 across the viewport */
	UFUNCTION(BlueprintPure, Category="Aim")
	FVector2D GetCrosshairNormalized() const;

	/** Crosshair position in viewport pixels. False without a local player viewport. */
	UFUNCTION(BlueprintPure, Category="Aim")
	bool GetCrosshairScreenPosition(FVector2D& OutPixels) const;

	/** Traces from the crosshair into the world. True on a blocking hit. */
	UFUNCTION(BlueprintCallable, Category="Aim")
	bool ResolveAim(FHitResult& OutHit) const;

	/** The world point under the crosshair: the hit, or a far point along the crosshair ray */
	UFUNCTION(BlueprintCallable, Category="Aim")
	FVector ResolveAimPoint() const;

	/** Fires one hitscan shot through the crosshair. False when blocked or still in refire. */
	UFUNCTION(BlueprintCallable, Category="Shot")
	bool Fire();

	UFUNCTION(BlueprintPure, Category="Shot")
	bool CanFire() const;

	UFUNCTION(BlueprintCallable, Category="Shot")
	void SetFireBlocked(bool bBlocked) { bFireBlocked = bBlocked; }

	UFUNCTION(BlueprintPure, Category="Shot")
	bool IsFireBlocked() const { return bFireBlocked; }

	UFUNCTION(BlueprintPure, Category="Shot")
	int32 GetShotsFired() const { return ShotsFired; }

	UFUNCTION(BlueprintPure, Category="Shot")
	int32 GetShotsHit() const { return ShotsHit; }

protected:

	APlayerController* GetPlayerController() const;
	FVector2D ClampToScreen(FVector2D Normalized) const;
};
