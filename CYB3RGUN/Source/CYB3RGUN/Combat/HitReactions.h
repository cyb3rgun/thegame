// CYB3RGUN THEGAME. Hit reactions of target bodies, and the held weapon as a hit target of its own.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "HitZoneSettings.h"

class UAnimSequenceBase;
class USkeletalMeshComponent;
struct FHitResult;

/**
 *  Shared by every target that shows hit zones (D-049, D-050). Picks the reaction for a zone and a side from the hit zone
 *  settings and plays it on the body's target anim instance, blended over what the body was doing. A call without an
 *  animation to play does nothing and returns false, so the caller keeps its own fallback. Also switches bodies and held
 *  weapons between shootable and not, drops and resets a weapon, and finds aim points on a body for the debug commands.
 */
struct CYB3RGUN_API FHitReactions
{
	/** Object channel of the player's projectiles, "Projectile" in DefaultEngine.ini */
	static constexpr ECollisionChannel ProjectileChannel = ECC_GameTraceChannel1;

	/** The animation a reaction plays from this side, null when the settings have none. A kill is a fall. */
	static UAnimSequenceBase* PickAnimation(EHitReaction Reaction, EHitZone Zone, EHitDirection Direction);

	/**
	 *  Plays a reaction on the body's DefaultSlot, a kill plays the fall instead. OnBlendOut runs once the reaction starts to hand
	 *  the body back, unless a newer reaction or a stop cut it off. False when there was nothing to play.
	 */
	static bool PlayReaction(USkeletalMeshComponent* Body, EHitReaction Reaction, EHitZone Zone, EHitDirection Direction, TFunction<void()> OnBlendOut = nullptr);

	/** Cross fades the body into the fall from this side and holds its last frame, running reactions stop */
	static bool PlayDeath(USkeletalMeshComponent* Body, EHitDirection Direction);

	/** Cross fades the body into a fall of the caller's own and holds its last frame, running reactions stop */
	static bool PlayFall(USkeletalMeshComponent* Body, UAnimSequenceBase* Fall, float PlayRate = 1.0f);

	/** Cross fades the body into the disarmed idle */
	static bool PlayDisarmedIdle(USkeletalMeshComponent* Body);

	/** Stops every reaction at once, for a body that is hidden or used again */
	static void StopReactions(USkeletalMeshComponent* Body);

	/** Makes the physics asset bodies a hit target: WorldStatic and blocking every channel, like the hit volumes they replaced. Off until SetBodyShootable. */
	static void SetupBodyTarget(USkeletalMeshComponent* Body);

	/** Query collision on while the body can be shot, off otherwise */
	static void SetBodyShootable(USkeletalMeshComponent* Body, bool bShootable);

	/** A held weapon as a hit target: tagged, WorldDynamic, blocking only projectiles and visibility traces, query only while its holder can shoot. A dropped weapon is left alone. */
	static void SetWeaponShootable(USkeletalMeshComponent* Weapon, bool bShootable);

	/** Lets the weapon go with physics along the shot: knocked away by a hit on it, falling away after a hit on the arm */
	static void DropWeapon(USkeletalMeshComponent* Weapon, const FVector& ShotDirection, const FVector& HitLocation, bool bKnockedAway);

	/** Physics off, back in the hand at the socket, not shootable, visible with the body */
	static void ResetWeapon(USkeletalMeshComponent* Weapon, USkeletalMeshComponent* Body, FName Socket);

	/** True once the weapon has left the hand */
	static bool IsWeaponDropped(const USkeletalMeshComponent* Weapon);

	/** True when a shot that landed at Hit, travelling along ShotDirection, grazed Body on the way in. OutGraze is the hit on Body. */
	static bool Grazes(USkeletalMeshComponent* Body, const FHitResult& Hit, const FVector& ShotDirection, FHitResult& OutGraze);

	/** Centre of the physics body of a bone, or the bone itself where it has no body. False without the bone. */
	static bool GetBonePoint(const USkeletalMeshComponent* Body, FName Bone, FVector& OutPoint);

	/** Centre of a held weapon's physics body, or of its bounds. False when it is not in the hand. */
	static bool GetWeaponPoint(const USkeletalMeshComponent* Weapon, FVector& OutPoint);
};
