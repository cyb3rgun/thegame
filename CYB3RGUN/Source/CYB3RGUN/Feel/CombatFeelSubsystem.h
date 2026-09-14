// CYB3RGUN THEGAME. Hit stop, camera kicks, the kill flash and Overclock for the local player.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "StyleScoringComponent.h"
#include "CombatFeelSubsystem.generated.h"

class APawn;
class APlayerController;
class UCombatFeelCameraModifier;
class UStyleSettings;
struct FStyleShake;

/**
 *  Turns the style record into feel (D-046): a short hit stop on a kill, a directional camera kick in three
 *  strengths, a brief screen effect on a kill, and Overclock. Overclock is a charge that fills on clean hits and
 *  drains while the held input keeps it running; it slows the world while the player keeps a separate, faster
 *  scale. It only runs where the scenario's style settings allow it, never in a precision scenario.
 *  Every value comes from UStyleSettings, and all timing is on the wall clock, so a slowed or stopped world
 *  never stretches its own effects.
 */
UCLASS()
class CYB3RGUN_API UCombatFeelSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:

	static UCombatFeelSubsystem* Get(const UObject* WorldContext);

	/** Speed of the player against the wall clock in the world of the context: 1 without Overclock */
	static float GetPlayerTimeScale(const UObject* WorldContext);

	/** The player holds or releases the Overclock input */
	void SetOverclockHeld(bool bHeld);

	/** Sets the charge directly, for tests */
	void SetOverclockCharge(float Charge);

	/** Charge from 0 to 1 */
	float GetOverclockFraction() const;

	float GetOverclockCharge() const { return OverclockCharge; }

	bool IsOverclockActive() const { return bOverclockActive; }

	/** True when the scenario allows Overclock */
	bool IsOverclockAllowed() const;

	/** True when there is enough charge to start */
	bool IsOverclockReady() const;

	/** The player took damage: the projected HUD glitches for a moment (D-051) */
	void NotifyPlayerDamaged(float Amount);

	/** 0 to 1, how strongly the projected HUD glitches right now, fading over DamageGlitchSeconds on the wall clock */
	float GetDamageGlitch() const;

	/** Speed of the player against the wall clock right now */
	float GetPlayerTimeScale() const { return PlayerTimeScale; }

	/** True while a hit stop holds the world */
	bool IsHitStopping() const;

	void LogStatus() const;

	//~ Begin UTickableWorldSubsystem
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void Deinitialize() override;
	//~ End UTickableWorldSubsystem

protected:

	UFUNCTION()
	void HandleStyleEvent(EStyleEvent Event, int32 Points, AActor* Target);

	/** Finds the local player, its style record and its camera, and binds to them once */
	void BindLocalPlayer();

	void StartHitStop(float Seconds);
	void Kick(const FStyleShake& Shake, AActor* Target);
	void UpdateOverclock(float RealDelta);
	void ApplyTime(float RealDelta);

	/** Gives the pawn that had the player scale its own time back */
	void ReleasePawn();

	const UStyleSettings* GetSettings() const;

	TWeakObjectPtr<APlayerController> LocalController;
	TWeakObjectPtr<UStyleScoringComponent> BoundStyle;
	TWeakObjectPtr<APawn> DilatedPawn;
	TWeakObjectPtr<UCombatFeelCameraModifier> CameraModifier;

	/** A hit stop waiting for the next tick, which starts its clock, so a slow rest of the frame cannot use it up */
	float PendingHitStopSeconds = 0.0f;

	/** Wall clock of the last damage the player took */
	double DamageGlitchAt = -1000.0;

	/** Wall clock until which the current hit stop holds, and when it began */
	double HitStopUntil = 0.0;
	double HitStopStarted = 0.0;
	bool bHitStopLogged = true;

	float OverclockCharge = 0.0f;
	float OverclockBlend = 0.0f;
	float PlayerTimeScale = 1.0f;

	/** World time dilation this subsystem last set */
	float AppliedWorldDilation = 1.0f;
	bool bOverclockHeld = false;
	bool bOverclockActive = false;
	double LastTickWallSeconds = 0.0;
};
