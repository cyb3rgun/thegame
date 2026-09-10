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

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	//~ FTickableGameObject
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual ETickableTickType GetTickableTickType() const override;
	virtual bool IsTickableWhenPaused() const override { return true; }
	virtual UWorld* GetTickableGameObjectWorld() const override;
};
