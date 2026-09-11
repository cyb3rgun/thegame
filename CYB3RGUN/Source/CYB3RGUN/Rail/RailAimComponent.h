// CYB3RGUN THEGAME. The one screen space aiming path (D-019): a crosshair position resolved to a world hit.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/HitResult.h"
#include "WeaponStatus.h"
#include "RailAimComponent.generated.h"

class APlayerController;
class UDamageType;
class UWeaponDefinition;
class USoundBase;

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

	/** Weapons the rider carries, the first is in hand at the start. Empty keeps the plain shot values above and an endless magazine. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapons")
	TArray<TObjectPtr<UWeaponDefinition>> Weapons;

	/** Rounds left in each carried weapon, kept across switches */
	TArray<int32> Rounds;

	int32 WeaponIndex = 0;

	/** Reload and switch timing, on real time. The frame each of them started on does not count. */
	bool bReloading = false;
	float ReloadElapsed = 0.0f;
	uint64 ReloadStartFrame = 0;
	float EquipRemaining = 0.0f;
	uint64 EquipStartFrame = 0;

	/** Wall clock at the start of the reload, the log reports the duration it measured */
	double ReloadStartSeconds = 0.0;

	/** Seconds on the aim's own clock, see TickComponent, and its reading at the last shot */
	double AimClock = 0.0;
	double LastShotClock = -1000.0;

	/** Wall clock at the previous tick, and real time of the last trigger pull on an empty magazine for the HUD cue */
	double LastTickWallSeconds = 0.0;
	double LastDryFireTime = -1000.0;

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

	/** Hands over the weapons to carry, the first comes up in hand with a full magazine when play starts */
	void SetWeapons(const TArray<UWeaponDefinition*>& InWeapons);

	/** The weapon in hand, null without carried weapons */
	UFUNCTION(BlueprintPure, Category="Weapons")
	const UWeaponDefinition* GetCurrentWeapon() const;

	/** Brings up the next carried weapon. False with fewer than two. */
	UFUNCTION(BlueprintCallable, Category="Weapons")
	bool SwitchWeapon();

	/** Starts refilling the weapon in hand. False when it is full, already reloading or nothing is carried. */
	UFUNCTION(BlueprintCallable, Category="Weapons")
	bool StartReload();

	/** Stops a reload before it finished, the magazine keeps what it had */
	UFUNCTION(BlueprintCallable, Category="Weapons")
	void CancelReload();

	UFUNCTION(BlueprintPure, Category="Weapons")
	bool IsReloading() const { return bReloading; }

	/** The weapon in hand for the HUD. False without carried weapons. */
	bool GetWeaponStatus(FWeaponStatus& OutStatus) const;

	/** Every carried weapon, in switching order */
	void GetLoadout(TArray<FWeaponStatus>& OutLoadout) const;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:

	APlayerController* GetPlayerController() const;
	FVector2D ClampToScreen(FVector2D Normalized) const;

	virtual void BeginPlay() override;

	/** Hands one hit to the target and to the damage path. Returns the pawn that took damage, or null. */
	AActor* ApplyShotHit(const FHitResult& Hit, float ShotDamage);

	/** Fires the weapon's pellets as traces in a cone around the crosshair ray. Returns the first pawn that took damage. */
	AActor* FirePellets(const UWeaponDefinition& Weapon);

	/** A trigger pull on an empty magazine: the click, and a time stamp for the HUD cue */
	void DryFire();

	void PlayWeaponSound(USoundBase* Sound) const;
};
