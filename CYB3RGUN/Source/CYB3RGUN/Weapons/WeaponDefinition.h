// CYB3RGUN THEGAME. How a weapon handles: magazine, reload, refire, damage and spread, as data.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WeaponDefinition.generated.h"

class UMaterialInterface;
class USoundBase;
class UStaticMesh;
class UTexture2D;

/**
 *  One weapon's handling, shared by every shot path: the door range and zombie weapons read it through
 *  AShooterWeapon, the rail reads it through its aim component. A weapon with more than one pellet fires
 *  hitscan pellets in a cone from the view, damage falls off with range.
 */
UCLASS(BlueprintType)
class CYB3RGUN_API UWeaponDefinition : public UDataAsset
{
	GENERATED_BODY()

public:

	/** Shown on the HUD */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Magazine", meta = (ClampMin = 1))
	int32 MagazineSize = 12;

	/** Seconds from starting a reload to a full magazine */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Magazine", meta = (ClampMin = 0.0, Units = "s"))
	float ReloadSeconds = 1.4f;

	/** Seconds between two shots */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Firing", meta = (ClampMin = 0.0, Units = "s"))
	float RefireSeconds = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Firing")
	bool bFullAuto = false;

	/** Seconds after a switch before the weapon can fire */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Firing", meta = (ClampMin = 0.0, Units = "s"))
	float EquipSeconds = 0.35f;

	/** Damage of one bullet, or of one pellet */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Damage", meta = (ClampMin = 0.0))
	float Damage = 34.0f;

	/** Pellets per shot. More than one fires hitscan pellets in a cone. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Damage", meta = (ClampMin = 1, ClampMax = 32))
	int32 Pellets = 1;

	/** Half angle of the pellet cone */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Damage", meta = (ClampMin = 0.0, ClampMax = 30.0, Units = "Degrees"))
	float SpreadDegrees = 0.0f;

	/** Farthest a hitscan pellet reaches */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Damage", meta = (ClampMin = 100.0, Units = "cm"))
	float MaxRange = 20000.0f;

	/** Full damage up to this distance */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Damage", meta = (ClampMin = 0.0, Units = "cm"))
	float FalloffStart = 20000.0f;

	/** At and beyond this distance only the minimum share of the damage lands */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Damage", meta = (ClampMin = 0.0, Units = "cm"))
	float FalloffEnd = 20000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Damage", meta = (ClampMin = 0.0, ClampMax = 1.0))
	float FalloffMinScale = 1.0f;

	/** Distance a single bullet lands off the aim point at the target. Zero goes exactly where it is aimed. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Bullet", meta = (ClampMin = 0.0, Units = "cm"))
	float BulletAimVariance = 0.0f;

	/** Radius of a single bullet's sweep. Small is precise; zero keeps the projectile's own radius. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Bullet", meta = (ClampMin = 0.0, Units = "cm"))
	float BulletRadius = 0.0f;

	/** Played when the trigger is pulled on an empty magazine */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sound")
	TObjectPtr<USoundBase> EmptySound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sound")
	TObjectPtr<USoundBase> ReloadStartSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sound")
	TObjectPtr<USoundBase> ReloadEndSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sound")
	TObjectPtr<USoundBase> EquipSound;

	/** The mark of the weapon's maker, beside its name in the HUD and the loadout, printed on the weapon (D-056). Our own
	 *  weapons carry the 3R house mark; the CYB3RGUN mark is the product's and never stands in for it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Maker")
	TObjectPtr<UTexture2D> MakerMark;

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
};
