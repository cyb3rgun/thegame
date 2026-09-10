// CYB3RGUN THEGAME. Keeps the running game in line with the graphics settings.

#include "CyberSettingsSubsystem.h"
#include "CyberGameUserSettings.h"
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

	// a standalone game already applied everything at startup; play in the editor only applies the rendering side, never the window
	if (GIsEditor)
	{
		Settings->ApplyNonResolutionSettings();
	}
}

void UCyberSettingsSubsystem::Tick(float DeltaTime)
{
	if (const UCyberGameUserSettings* Settings = UCyberGameUserSettings::Get())
	{
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
