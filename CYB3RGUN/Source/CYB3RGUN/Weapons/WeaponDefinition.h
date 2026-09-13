// CYB3RGUN THEGAME. Everything a weapon is, as data: name, body, mounts, fire mode, ballistics, feed, handling, sound (D-052).

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WeaponDefinition.generated.h"

class AShooterProjectile;
class UAnimInstance;
class UMaterialInterface;
class USkeletalMesh;
class USoundBase;
class UStaticMesh;
class UTexture2D;

/** How the trigger fires the weapon */
UENUM(BlueprintType)
enum class EWeaponFireMode : uint8
{
	/** One shot, then the action cycles before the next shot can fire */
	SingleShot,
	/** One shot per trigger pull, as fast as the refire time allows */
	SemiAuto,
	/** Kept free for later modes such as bursts; fires like semi auto until one is built */
	Reserved
};

/** What the weapon spends on a shot */
UENUM(BlueprintType)
enum class EWeaponFeed : uint8
{
	/** Cartridges from a magazine, a reload fills it */
	Magazine,
	/** Air from a reservoir: pressure is the ammunition, a refill from the carried supply restores it (D-055) */
	Pressure
};

/** The mount points of a weapon body, the contract any future mesh fulfils with sockets (D-054) */
UENUM(BlueprintType)
enum class EWeaponMount : uint8
{
	/** Where the hand holds the weapon */
	Grip,
	/** Where shots and the muzzle flash leave */
	Muzzle,
	/** Where the magazine or the air reservoir sits */
	Magazine,
	/** Where an optic mounts */
	Optic,
	/** Where the pressure gauge sits on a pressure fed weapon */
	PressureGauge
};

/** One mount point: a socket on the mesh, and where the placeholder body has it while the mesh lacks the socket */
USTRUCT(BlueprintType)
struct CYB3RGUN_API FWeaponMountPoint
{
	GENERATED_BODY()

	/** Socket of this name on the weapon mesh */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mount")
	FName Socket;

	/** The mount in the weapon's own space, used when the mesh has no socket of that name */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mount")
	FTransform Fallback;
};

/** The five mount points, named once for the whole weapon (D-054) */
USTRUCT(BlueprintType)
struct CYB3RGUN_API FWeaponMounts
{
	GENERATED_BODY()

	FWeaponMounts();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mount")
	FWeaponMountPoint Grip;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mount")
	FWeaponMountPoint Muzzle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mount")
	FWeaponMountPoint Magazine;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mount")
	FWeaponMountPoint Optic;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mount")
	FWeaponMountPoint PressureGauge;

	const FWeaponMountPoint& Get(EWeaponMount Mount) const;
};

/** One piece of placeholder geometry, placed relative to a mount point */
USTRUCT(BlueprintType)
struct CYB3RGUN_API FWeaponBodyPart
{
	GENERATED_BODY()

	/** An engine shape or any static mesh */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Body")
	TObjectPtr<UStaticMesh> Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Body")
	TObjectPtr<UMaterialInterface> Material;

	/** The mount the part sits on */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Body")
	EWeaponMount Mount = EWeaponMount::Grip;

	/** Placement relative to that mount */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Body")
	FTransform Transform;

	/** The needle of the pressure gauge: the part turns about its mount's X axis with the reservoir pressure, through the
	 *  definition's gauge sweep, empty at one end and full at the other */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Body")
	bool bPressureNeedle = false;
};

/**
 *  One weapon, completely, as data (D-052). The weapon actor ACyberWeapon and the rail aim both read it, so adding a
 *  weapon is a new definition and never code. The name and subtitle are our own text (D-053). The body is a mesh, or
 *  placeholder geometry until a model exists, and the mount points name the sockets a model must carry (D-054).
 */
UCLASS(BlueprintType)
class CYB3RGUN_API UWeaponDefinition : public UDataAsset
{
	GENERATED_BODY()

public:

	/** The weapon's name, shown on the HUD and in the loadout */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon")
	FText DisplayName;

	/** What kind of weapon it is, shown under the name */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon")
	FText Subtitle;

	/** The mark of the weapon's maker, beside its name in the HUD and the loadout, printed on the weapon (D-056). Our own
	 *  weapons carry the 3R house mark; the CYB3RGUN mark is the product's and never stands in for it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon")
	TObjectPtr<UTexture2D> MakerMark;

	/** The weapon's mesh. Without one the placeholder body stands in (D-054). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Body")
	TObjectPtr<USkeletalMesh> Mesh;

	/** Placeholder geometry, shown with the mesh or instead of it until a model exists */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Body")
	TArray<FWeaponBodyPart> PlaceholderBody;

	/** Mount points: sockets on the mesh, with the placeholder positions as fallback (D-054) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Body")
	FWeaponMounts Mounts;

	/** Arms animation of the first person character while the weapon is in hand */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Body")
	TSubclassOf<UAnimInstance> FirstPersonAnimClass;

	/** Body animation of the third person character while the weapon is in hand */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Body")
	TSubclassOf<UAnimInstance> ThirdPersonAnimClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Firing")
	EWeaponFireMode FireMode = EWeaponFireMode::SemiAuto;

	/** Seconds between two shots of a semi auto weapon */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Firing", meta = (ClampMin = 0.0, Units = "s"))
	float RefireSeconds = 0.18f;

	/** Seconds the action of a single shot weapon takes to cycle before the next shot */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Firing", meta = (ClampMin = 0.0, Units = "s"))
	float CycleSeconds = 0.8f;

	/** Draw time: seconds from switching to the weapon until it can fire */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Firing", meta = (ClampMin = 0.0, Units = "s", DisplayName = "Draw Seconds"))
	float EquipSeconds = 0.35f;

	/** Cartridges from a magazine, or air from a reservoir (D-055) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Magazine")
	EWeaponFeed Feed = EWeaponFeed::Magazine;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Magazine", meta = (ClampMin = 1, EditCondition = "Feed == EWeaponFeed::Magazine"))
	int32 MagazineSize = 12;

	/** Seconds from starting a reload to a full magazine */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Magazine", meta = (ClampMin = 0.0, Units = "s", EditCondition = "Feed == EWeaponFeed::Magazine"))
	float ReloadSeconds = 1.4f;

	/** Reservoir pressure when full, in bar */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pressure", meta = (ClampMin = 1.0, EditCondition = "Feed == EWeaponFeed::Pressure"))
	float FillPressureBar = 250.0f;

	/** Below this pressure, in bar, the valve no longer opens and the trigger only clicks */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pressure", meta = (ClampMin = 0.0, EditCondition = "Feed == EWeaponFeed::Pressure"))
	float MinFirePressureBar = 120.0f;

	/** Share of the current pressure one shot uses. A thirstier valve empties the reservoir in fewer shots. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pressure", meta = (ClampMin = 0.0, ClampMax = 0.5, EditCondition = "Feed == EWeaponFeed::Pressure"))
	float PressureUsePerShot = 0.07f;

	/** Muzzle energy against pressure: points of pressure in bar (X) and the share of the muzzle energy (Y). Speed and
	 *  damage follow the energy. Linear between points, held flat beyond the ends. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pressure", meta = (EditCondition = "Feed == EWeaponFeed::Pressure"))
	TArray<FVector2D> EnergyCurve;

	/** Projectile drop against pressure: points of pressure in bar (X) and a multiplier on the drop scale (Y) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pressure", meta = (EditCondition = "Feed == EWeaponFeed::Pressure"))
	TArray<FVector2D> DropCurve;

	/** Accuracy against pressure: points of pressure in bar (X) and degrees added to the accuracy cone (Y) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pressure", meta = (EditCondition = "Feed == EWeaponFeed::Pressure"))
	TArray<FVector2D> ConeCurve;

	/** Seconds a refill from the carried supply takes */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pressure", meta = (ClampMin = 0.0, Units = "s", EditCondition = "Feed == EWeaponFeed::Pressure"))
	float RefillSeconds = 3.5f;

	/** Refills the supply carries into a scenario; each one fills the reservoir once */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pressure", meta = (ClampMin = 0, EditCondition = "Feed == EWeaponFeed::Pressure"))
	int32 RefillsCarried = 3;

	/** Degrees the gauge needle turns from an empty reservoir to a full one, centred on the needle's placement */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pressure", meta = (ClampMin = 0.0, ClampMax = 360.0, Units = "Degrees", EditCondition = "Feed == EWeaponFeed::Pressure"))
	float GaugeSweepDegrees = 270.0f;

	/** The projectile a single shot flies as. Several pellets fire traces instead. Empty keeps the holder's own. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ballistics")
	TSubclassOf<AShooterProjectile> ProjectileClass;

	/** Muzzle energy of one shot in joules. With the projectile mass it gives the muzzle velocity; zero keeps the projectile's own speed. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ballistics", meta = (ClampMin = 0.0))
	float MuzzleEnergy = 0.0f;

	/** Mass of one projectile */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ballistics", meta = (ClampMin = 0.1, Units = "Grams"))
	float ProjectileMassGrams = 8.0f;

	/** Gravity on the projectile in flight against the world's: 1 drops as it would, 0 flies straight */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ballistics", meta = (ClampMin = 0.0))
	float DropScale = 1.0f;

	/** Radius of a single projectile's sweep. Small is precise; zero keeps the projectile's own radius. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ballistics", meta = (ClampMin = 0.0, Units = "cm"))
	float BulletRadius = 0.0f;

	/** Damage of one projectile, or of one pellet */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Damage", meta = (ClampMin = 0.0))
	float Damage = 34.0f;

	/** Pellets per shot. More than one fires traces in a cone. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Damage", meta = (ClampMin = 1, ClampMax = 32))
	int32 Pellets = 1;

	/** Half angle of the pellet cone */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Damage", meta = (ClampMin = 0.0, ClampMax = 30.0, Units = "Degrees"))
	float SpreadDegrees = 0.0f;

	/** Farthest a pellet reaches */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Damage", meta = (ClampMin = 100.0, Units = "cm"))
	float MaxRange = 20000.0f;

	/** Effective range: full damage up to this distance */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Damage", meta = (ClampMin = 0.0, Units = "cm"))
	float FalloffStart = 20000.0f;

	/** At and beyond this distance only the minimum share of the damage lands */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Damage", meta = (ClampMin = 0.0, Units = "cm"))
	float FalloffEnd = 20000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Damage", meta = (ClampMin = 0.0, ClampMax = 1.0))
	float FalloffMinScale = 1.0f;

	/** Half angle of the cone a single shot lands in with the weapon settled */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Handling", meta = (ClampMin = 0.0, ClampMax = 20.0, Units = "Degrees"))
	float AccuracyConeDegrees = 0.0f;

	/** Cone each shot adds on top, recovering over time */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Handling", meta = (ClampMin = 0.0, ClampMax = 20.0, Units = "Degrees"))
	float BloomPerShotDegrees = 0.0f;

	/** Most the bloom can add */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Handling", meta = (ClampMin = 0.0, ClampMax = 20.0, Units = "Degrees"))
	float MaxBloomDegrees = 0.0f;

	/** How fast the bloom settles, degrees per second */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Handling", meta = (ClampMin = 0.0))
	float BloomRecoveryDegreesPerSecond = 2.0f;

	/** Recoil impulse: how far one shot throws the aim up */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Handling", meta = (ClampMin = 0.0, ClampMax = 30.0, Units = "Degrees"))
	float RecoilPitchDegrees = 0.0f;

	/** Sideways kick of one shot, up to this far to either side */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Handling", meta = (ClampMin = 0.0, ClampMax = 10.0, Units = "Degrees"))
	float RecoilYawDegrees = 0.0f;

	/** Recovery: seconds the aim takes to come back down after a shot */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Handling", meta = (ClampMin = 0.01, Units = "s"))
	float RecoilRecoverySeconds = 0.25f;

	/** Share of the climb the aim recovers, the rest stays where the recoil put it */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Handling", meta = (ClampMin = 0.0, ClampMax = 1.0))
	float RecoilRecoveryShare = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sound")
	TObjectPtr<USoundBase> FireSound;

	/** Played while a single shot weapon cycles its action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sound")
	TObjectPtr<USoundBase> CycleSound;

	/** Played when the trigger is pulled on an empty weapon */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sound")
	TObjectPtr<USoundBase> EmptySound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sound")
	TObjectPtr<USoundBase> ReloadStartSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sound")
	TObjectPtr<USoundBase> ReloadEndSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sound")
	TObjectPtr<USoundBase> EquipSound;

	/** The print on the first person weapon: a small plane with the mark, on a bone of the weapon mesh */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Maker")
	TObjectPtr<UStaticMesh> PrintMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Maker")
	TObjectPtr<UMaterialInterface> PrintMaterial;

	/** Bone or socket of the weapon mesh the print sits on */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Maker")
	FName PrintBone;

	/** Where the print sits, in the space of that bone */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Maker")
	FTransform PrintTransform;

	/** Damage share that lands at a distance, 1 up to the falloff start, down to the minimum at its end */
	float GetDamageScale(float Distance) const;

	/** Muzzle velocity from the energy and the projectile mass, zero without an energy */
	float GetMuzzleSpeed(float Energy) const;

	bool IsPressureFed() const { return Feed == EWeaponFeed::Pressure; }

	/** Share of the muzzle energy at a reservoir pressure, 1 without a curve */
	float GetEnergyShare(float PressureBar) const;

	/** Multiplier on the drop scale at a reservoir pressure, 1 without a curve */
	float GetDropMultiplier(float PressureBar) const;

	/** Degrees added to the accuracy cone at a reservoir pressure, 0 without a curve */
	float GetPressureCone(float PressureBar) const;

	/** Value of a piecewise linear curve of points at X, held flat beyond its ends; the fallback without points */
	static float EvaluateCurve(const TArray<FVector2D>& Points, float X, float Fallback);
};
