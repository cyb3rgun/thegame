// CYB3RGUN THEGAME. The one weapon actor: everything it is comes from its weapon definition (D-052).

#pragma once

#include "CoreMinimal.h"
#include "ShooterWeapon.h"
#include "WeaponDefinition.h"
#include "WeaponState.h"
#include "CyberWeapon.generated.h"

class UStaticMeshComponent;

/**
 *  Every weapon a player carries is this one actor, told what it is by a UWeaponDefinition (D-052). It shows the
 *  definition's mesh or its placeholder body with the grip mount in the hand, and resolves every mount point against the
 *  mesh's sockets with the placeholder positions as fallback (D-054): the muzzle mount places the projectile spawn and the
 *  muzzle flash. It fires by the rules of FWeaponState, single shot with a cycling action or semi auto, after its draw
 *  time, with a recoil that recovers and an accuracy cone that blooms. A single shot flies as a projectile with the
 *  definition's ballistics, pellets as traces; both reach the style record, the hit zones and the disarm as before.
 *  A pressure fed weapon spends air (D-055), and its gauge needle shows the reservoir at the pressure gauge mount.
 */
UCLASS()
class CYB3RGUN_API ACyberWeapon : public AShooterWeapon
{
	GENERATED_BODY()

public:

	ACyberWeapon();

	/** Spawns the weapon a definition describes for a holder, attached and ready to be activated */
	static ACyberWeapon* SpawnFor(AActor* Holder, const UWeaponDefinition* InDefinition);

	/** World transform of a mount point on the first person or the world body (D-054) */
	FTransform GetMountTransform(EWeaponMount Mount, bool bFirstPerson) const;

	const FWeaponState& GetState() const { return State; }

	//~ Begin AShooterWeapon
	virtual void ActivateWeapon(const FName& OwnerTag) override;
	virtual void StartFiring() override;
	virtual void StopFiring() override;
	virtual bool StartReload() override;
	virtual void CancelReload() override;
	virtual void FillStatus(FWeaponStatus& OutStatus) const override;
	//~ End AShooterWeapon

protected:

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual FVector GetMuzzleLocation() const override;

	/** The rules of this weapon, advanced on the holder's clock */
	FWeaponState State;

	/** Placeholder geometry on the first person and the world body */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> BodyParts;

	/** A body part that reads the reservoir pressure, with the placement it turns from */
	struct FGaugeNeedle
	{
		TObjectPtr<UStaticMeshComponent> Component;
		FTransform PartTransform;
		FTransform MountTransform;
	};

	/** The pressure gauge needles on both bodies; the components are kept alive by BodyParts */
	TArray<FGaugeNeedle> Needles;

	/** Aim climb still to come back, degrees, and how fast it comes back */
	float RecoilToRecover = 0.0f;
	float RecoilRecoveryRate = 0.0f;

	/** The action last tick, to notice a finished reload */
	EWeaponAction LastAction = EWeaponAction::Ready;

	/** Shows the definition's placeholder parts on both bodies, with the grip mount in the hand */
	void BuildBody();

	/** A mount in the space of a mesh component: the mesh's socket, or the placeholder position */
	FTransform GetMountLocal(EWeaponMount Mount, const USkeletalMeshComponent* Mesh) const;

	/** Turns the gauge needles to the reservoir pressure */
	void UpdateGauge();

	/** Sends one shot on its way: a projectile within the cone, or the pellets, with its flash, sound and recoil */
	void FireShot(const FWeaponShot& Shot);

	/** Throws the aim up and to a side; the recovery brings back its share */
	void ApplyRecoil();
	void RecoverRecoil(float Delta);

	AController* GetHolderController() const;
};
