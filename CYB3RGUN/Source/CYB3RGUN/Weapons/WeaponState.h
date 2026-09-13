// CYB3RGUN THEGAME. The rules of one carried weapon: rounds, the action, draw, reload and bloom, for every holder (D-052).

#pragma once

#include "CoreMinimal.h"

class UWeaponDefinition;
struct FWeaponStatus;

/** What the weapon's action is busy with */
enum class EWeaponAction : uint8
{
	Ready,
	/** Coming up after a switch, the draw time */
	Drawing,
	/** A single shot weapon working its action before the next shot */
	Cycling,
	Reloading
};

/** One shot as the rules let it leave the weapon */
struct FWeaponShot
{
	float Damage = 0.0f;

	/** Muzzle velocity in centimetres per second, zero keeps the projectile's own */
	float Speed = 0.0f;

	/** Gravity on the projectile in flight against the world's */
	float GravityScale = 1.0f;

	/** Half angle the shot lands within: the settled cone plus the bloom */
	float ConeDegrees = 0.0f;

	int32 Pellets = 1;

	/** Share of the full muzzle energy, below 1 once a pressure fed weapon's pressure has fallen */
	float EnergyShare = 1.0f;

	/** Reservoir pressure the shot left with, in bar, zero for a magazine fed weapon */
	float PressureBar = 0.0f;
};

/**
 *  The rules of one carried weapon, one set for the weapon actor and the rail aim alike (D-052). Semi auto weapons wait
 *  their refire time, single shot weapons cycle their action after every shot; a switch draws the weapon, a reload fills
 *  it, and every shot blooms the accuracy cone, which settles again. The holder advances it on its own clock and turns
 *  the shots it hands out into projectiles or traces.
 *
 *  A pressure fed weapon spends air instead of rounds (D-055): every shot uses a share of the reservoir pressure, and the
 *  pressure sets the shot's muzzle energy, drop and cone through the definition's curves. Below the firing pressure the
 *  trigger clicks; its reload is a refill from the carried supply, which holds a limited number of them.
 */
struct CYB3RGUN_API FWeaponState
{
	/** Takes the weapon's handling and fills it */
	void Init(const UWeaponDefinition* InDefinition);

	/** Advances the action, the refire wait and the bloom on the holder's clock */
	void Tick(float Delta);

	/** Brings the weapon up: it cannot fire for its draw time. A reload is lost, a cycle finished while holstered. */
	void Draw();

	/** True when a trigger pull fires now */
	bool CanFire() const;

	/** True when a trigger pull on a ready weapon only clicks: no rounds, or pressure below the firing pressure */
	bool IsEmpty() const;

	/** Takes one shot: uses the round or the air, starts the cycle or the refire wait and blooms the cone. Check CanFire first. */
	FWeaponShot Fire();

	/** Starts filling the weapon, a reload or a refill. False when it is full, busy or has nothing to fill from. */
	bool StartReload();

	/** Stops a reload, the weapon keeps what it had */
	void CancelReload();

	/** Marks a trigger pull on an empty weapon for the HUD cue, real seconds */
	void NoteDryFire(double RealSeconds) { LastDryFireTime = RealSeconds; }

	EWeaponAction GetAction() const { return Action; }

	/** 0 to 1 through the current action */
	float GetActionProgress() const;

	int32 GetRounds() const { return Rounds; }
	float GetPressure() const { return Pressure; }
	int32 GetRefillsLeft() const { return RefillsLeft; }
	float GetBloom() const { return Bloom; }

	/** Seconds the current reload or refill takes */
	float GetReloadSeconds() const;

	/** Short account of what the weapon holds, rounds or bar and refills, for logs */
	FString DescribeAmmo() const;
	const UWeaponDefinition* GetDefinition() const { return Definition; }

	/** Fills every HUD field the rules know; the holder adds its index, count and hints */
	void FillStatus(FWeaponStatus& OutStatus) const;

private:

	void StartAction(EWeaponAction NewAction, float Duration);
	void FinishAction();

	const UWeaponDefinition* Definition = nullptr;
	int32 Rounds = 0;

	/** Reservoir pressure in bar and the refills the supply still holds, for a pressure fed weapon */
	float Pressure = 0.0f;
	int32 RefillsLeft = 0;

	EWeaponAction Action = EWeaponAction::Ready;
	float ActionElapsed = 0.0f;
	float ActionDuration = 0.0f;

	/** The frame an action started on; that frame's delta ran before it began and does not count */
	uint64 ActionStartFrame = 0;

	/** Seconds until a semi auto weapon may fire again */
	float RefireRemaining = 0.0f;

	float Bloom = 0.0f;
	double LastDryFireTime = -1000.0;
};
