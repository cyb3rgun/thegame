// CYB3RGUN THEGAME. Keeps the running game in line with the graphics settings.

#include "CyberSettingsSubsystem.h"
#include "CyberGameUserSettings.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogCyberSettingsSubsystem, Log, All);

void UCyberSettingsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UCyberGameUserSettings* Settings = UCyberGameUserSettings::Get();
	if (!Settings)
	{
		UE_LOG(LogCyberSettingsSubsystem, Warning, TEXT("GameUserSettingsClassName does not point at UCyberGameUserSettings, graphics settings are inactive"));
		return;
	}

	// The engine applies the scalability groups saved in GameUserSettings.ini during its own start, and UGameUserSettings
	// skips scalability while the engine initialises, so a preset changed by a settings migration or the first run
	// hardware detection would otherwise run on the old groups for a whole session. The settings are applied once the
	// engine is up; play in the editor only ever applies the rendering side, never the window.
	if (GEngine && GEngine->IsInitialized())
	{
		ApplyWhenEngineReady(Settings);
	}
	else
	{
		bApplyOnFirstTick = true;
	}
}

void UCyberSettingsSubsystem::ApplyWhenEngineReady(UCyberGameUserSettings* Settings)
{
	Settings->ApplyNonResolutionSettings();

	// a standalone game saves right away, so the saved groups, the preset and the settings version stay in step
	if (!GIsEditor)
	{
		Settings->SaveSettings();
	}
}

void UCyberSettingsSubsystem::Tick(float DeltaTime)
{
	if (UCyberGameUserSettings* Settings = UCyberGameUserSettings::Get())
	{
		if (bApplyOnFirstTick)
		{
			bApplyOnFirstTick = false;
			ApplyWhenEngineReady(Settings);
		}
		Settings->ApplyFieldOfView(GetTickableGameObjectWorld());
	}
}

TStatId UCyberSettingsSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UCyberSettingsSubsystem, STATGROUP_Tickables);
}

ETickableTickType UCyberSettingsSubsystem::GetTickableTickType() const
{
	return HasAnyFlags(RF_ClassDefaultObject) ? ETickableTickType::Never : ETickableTickType::Always;
}

UWorld* UCyberSettingsSubsystem::GetTickableGameObjectWorld() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetWorld() : nullptr;
}
