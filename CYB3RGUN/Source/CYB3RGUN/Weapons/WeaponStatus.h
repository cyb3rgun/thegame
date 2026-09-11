// CYB3RGUN THEGAME. What a player's weapon is doing right now, for the HUD.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "WeaponStatus.generated.h"

/** The state of the weapon in hand */
USTRUCT(BlueprintType)
struct FWeaponStatus
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Weapon")
	FText WeaponName;

	UPROPERTY(BlueprintReadOnly, Category="Weapon")
	int32 Rounds = 0;

	UPROPERTY(BlueprintReadOnly, Category="Weapon")
	int32 MagazineSize = 0;

	UPROPERTY(BlueprintReadOnly, Category="Weapon")
	bool bReloading = false;

	/** 0 to 1 while reloading */
	UPROPERTY(BlueprintReadOnly, Category="Weapon")
	float ReloadProgress = 0.0f;

	/** True while a switched-to weapon comes up and cannot fire yet */
	UPROPERTY(BlueprintReadOnly, Category="Weapon")
	bool bSwitching = false;

	/** Real time of the last trigger pull on an empty magazine, for the empty cue */
	UPROPERTY(BlueprintReadOnly, Category="Weapon")
	double LastDryFireTime = -1000.0;

	/** How a reload starts here, shown with the empty cue */
	UPROPERTY(BlueprintReadOnly, Category="Weapon")
	FText ReloadHint;

	UPROPERTY(BlueprintReadOnly, Category="Weapon")
	int32 WeaponIndex = 0;

	UPROPERTY(BlueprintReadOnly, Category="Weapon")
	int32 WeaponCount = 0;
};

UINTERFACE(MinimalAPI)
class UWeaponStatusSource : public UInterface
{
	GENERATED_BODY()
};

/** Implemented by player pawns, so one HUD reads the weapon of every scenario */
class CYB3RGUN_API IWeaponStatusSource
{
	GENERATED_BODY()

public:

	/** False while the pawn holds no weapon */
	virtual bool GetWeaponStatus(FWeaponStatus& OutStatus) const = 0;
};
