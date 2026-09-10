// CYB3RGUN THEGAME. One registry of settings options shared by the menu and the console.

#pragma once

#include "CoreMinimal.h"
#include "CyberSettingsTypes.h"

/**
 *  Describes each option as a list of discrete values: how many there are, what they are called,
 *  which one a settings state currently holds and how to select another. Menus cycle through indices,
 *  console commands look values up by name. Neither touches a console variable; the settings object does.
 */
struct CYB3RGUN_API FCyberSettingsOptions
{
	/** Label shown for the option */
	static FText GetOptionLabel(ECyberSettingOption Option);

	/** Short name used by console commands */
	static FString GetOptionName(ECyberSettingOption Option);

	/** The console variables the option drives, for reports and the dump command */
	static FString GetDrivenCVars(ECyberSettingOption Option);

	static int32 GetValueCount(ECyberSettingOption Option);
	static FText GetValueLabel(ECyberSettingOption Option, int32 Index);
	static int32 GetValueIndex(ECyberSettingOption Option, const FCyberSettingsState& State);

	/** Selects a value. Selecting a preset resets every feature to that preset's defaults. */
	static void SetValueIndex(ECyberSettingOption Option, FCyberSettingsState& State, int32 Index);

	/** Experimental options are only offered on Ultra */
	static bool IsAvailable(ECyberSettingOption Option, const FCyberSettingsState& State);

	/** True when Nanite really runs: the switch is on and Virtual Shadow Maps are on. */
	static bool IsNaniteActive(const FCyberFeatureSettings& Features);

	static bool IsExperimental(ECyberSettingOption Option);

	/** True when the option only takes effect after a restart */
	static bool NeedsRestart(ECyberSettingOption Option);

	static bool FindOption(const FString& Name, ECyberSettingOption& OutOption);

	/** Accepts a value label with or without spaces, or a plain index. Numbers work for frame rate and field of view. */
	static bool ParseValue(ECyberSettingOption Option, const FString& Value, int32& OutIndex);

	/** Resolutions offered: the supported fullscreen modes plus the desktop resolution */
	static const TArray<FIntPoint>& GetResolutions();

	static const TArray<float>& GetFrameRateCaps();
	static const TArray<float>& GetFieldOfViews();
};
