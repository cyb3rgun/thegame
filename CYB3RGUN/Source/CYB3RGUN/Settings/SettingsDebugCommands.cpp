// CYB3RGUN THEGAME. Console access to the graphics settings for testing and automated verification.
// Every change goes through UCyberGameUserSettings, the same path the menu uses. Not compiled into shipping builds.

#include "CyberGameUserSettings.h"
#include "CyberSettingsOptions.h"
#include "HAL/IConsoleManager.h"

#if !UE_BUILD_SHIPPING

DEFINE_LOG_CATEGORY_STATIC(LogCyberSettingsDebug, Log, All);

namespace CyberSettingsDebug
{
	void Dump()
	{
		if (const UCyberGameUserSettings* Settings = UCyberGameUserSettings::Get())
		{
			for (const FString& Line : Settings->DescribeLiveState())
			{
				UE_LOG(LogCyberSettingsDebug, Log, TEXT("%s"), *Line);
			}
		}
	}

	bool Set(const FString& OptionName, const FString& Value)
	{
		UCyberGameUserSettings* Settings = UCyberGameUserSettings::Get();
		ECyberSettingOption Option;
		if (!Settings || !FCyberSettingsOptions::FindOption(OptionName, Option))
		{
			UE_LOG(LogCyberSettingsDebug, Warning, TEXT("Settings.Set: unknown option %s"), *OptionName);
			return false;
		}

		FCyberSettingsState State = Settings->GetState();
		if (!FCyberSettingsOptions::IsAvailable(Option, State))
		{
			UE_LOG(LogCyberSettingsDebug, Warning, TEXT("Settings.Set: %s is only available on the Ultra preset"), *OptionName);
			return false;
		}

		int32 Index = 0;
		if (!FCyberSettingsOptions::ParseValue(Option, Value, Index))
		{
			UE_LOG(LogCyberSettingsDebug, Warning, TEXT("Settings.Set: %s does not accept %s"), *OptionName, *Value);
			return false;
		}

		FCyberSettingsOptions::SetValueIndex(Option, State, Index);
		Settings->SetState(State, true);
		UE_LOG(LogCyberSettingsDebug, Log, TEXT("Settings.Set: %s = %s%s"), *FCyberSettingsOptions::GetOptionName(Option),
			*FCyberSettingsOptions::GetValueLabel(Option, Index).ToString(), FCyberSettingsOptions::NeedsRestart(Option) ? TEXT(", takes effect after a restart") : TEXT(""));
		return true;
	}
}

static FAutoConsoleCommand GSettingsDumpCommand(
	TEXT("Settings.Dump"),
	TEXT("Logs every graphics option, the console variables it drives and their live values."),
	FConsoleCommandDelegate::CreateStatic(&CyberSettingsDebug::Dump));

static FAutoConsoleCommand GSettingsSetCommand(
	TEXT("Settings.Set"),
	TEXT("Settings.Set <Option> <Value>. Options: Preset MegaLights GI VSM Fog AA Nanite Effects ViewDistance MotionBlur FrameCap VSync Resolution WindowMode FOV NaniteSkinned NaniteFoliage. Applies and saves."),
	FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
	{
		if (Args.Num() < 2)
		{
			UE_LOG(LogCyberSettingsDebug, Warning, TEXT("Settings.Set <Option> <Value>"));
			return;
		}
		// values with spaces such as "TSR Quality" arrive split, join them back
		CyberSettingsDebug::Set(Args[0], FString::Join(TArray<FString>(Args.GetData() + 1, Args.Num() - 1), TEXT(" ")));
	}));

static FAutoConsoleCommand GSettingsPresetCommand(
	TEXT("Settings.Preset"),
	TEXT("Settings.Preset <Low|Medium|High|Epic|Ultra>. Selects a preset, resets every feature to its defaults, applies and saves."),
	FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
	{
		if (Args.Num() > 0)
		{
			CyberSettingsDebug::Set(TEXT("Preset"), Args[0]);
		}
	}));

static FAutoConsoleCommand GSettingsResetCommand(
	TEXT("Settings.Reset"),
	TEXT("Returns every graphics option to its defaults, applies and saves."),
	FConsoleCommandDelegate::CreateLambda([]()
	{
		if (UCyberGameUserSettings* Settings = UCyberGameUserSettings::Get())
		{
			Settings->SetState(Settings->GetDefaultState(), true);
			UE_LOG(LogCyberSettingsDebug, Log, TEXT("Settings.Reset: preset %s"), *UCyberGameUserSettings::GetPresetName(Settings->GetQualityPreset()));
		}
	}));

static FAutoConsoleCommand GSettingsDetectCommand(
	TEXT("Settings.Detect"),
	TEXT("Runs the engine hardware benchmark, logs the preset it maps to and applies it."),
	FConsoleCommandDelegate::CreateLambda([]()
	{
		if (UCyberGameUserSettings* Settings = UCyberGameUserSettings::Get())
		{
			const ECyberQualityPreset Preset = Settings->RunHardwareDetection();
			FCyberSettingsState State = Settings->GetState();
			FCyberSettingsOptions::SetValueIndex(ECyberSettingOption::Preset, State, static_cast<int32>(Preset));
			Settings->SetState(State, true);
		}
	}));

#endif
