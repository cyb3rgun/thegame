// CYB3RGUN THEGAME. Keeps the running game in line with the graphics settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "CyberSettingsSubsystem.generated.h"

/**
 *  Applies the saved settings when play starts inside the editor (a standalone game applies them at startup)
 *  and keeps the field of view on whatever camera the player currently looks through, across pawn changes.
 */
UCLASS()
class CYB3RGUN_API UCyberSettingsSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

	/** The engine was still initialising when the subsystem started, so the settings apply on the first tick */
	bool bApplyOnFirstTick = false;

	/** Applies the settings in full once the engine runs, and saves them in a standalone game */
	void ApplyWhenEngineReady(class UCyberGameUserSettings* Settings);

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	//~ FTickableGameObject
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual ETickableTickType GetTickableTickType() const override;
	virtual bool IsTickableWhenPaused() const override { return true; }
	virtual UWorld* GetTickableGameObjectWorld() const override;
};
