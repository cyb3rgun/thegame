// CYB3RGUN THEGAME. Player facing graphics settings: presets, feature toggles, hardware detection.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "CyberSettingsTypes.h"
#include "CyberGameUserSettings.generated.h"

class UWorld;

/**
 *  Saved to GameUserSettings.ini and applied at startup (D-025). A preset drives the engine scalability
 *  groups and a set of feature defaults; every feature can then be switched on its own. Every console
 *  variable a setting drives is set here, by code, so project settings and scalability never fight it.
 *  The two experimental Nanite options are read only engine switches: they are staged into the engine
 *  config and take effect on the next launch.
 */
UCLASS(config=GameUserSettings, configdonotcheckdefaults)
class CYB3RGUN_API UCyberGameUserSettings : public UGameUserSettings
{
	GENERATED_BODY()

protected:

	UPROPERTY(Config)
	int32 CyberSettingsVersion = 0;

	UPROPERTY(Config)
	ECyberQualityPreset QualityPreset = ECyberQualityPreset::High;

	UPROPERTY(Config)
	FCyberFeatureSettings Features;

	UPROPERTY(Config)
	float FieldOfView = 90.0f;

	/** The weapon is not drawn in the player's hands unless this is on (D-094, D-095) */
	UPROPERTY(Config)
	bool bShowWeaponOnScreen = false;

	/** Multiplies the reticle speed where the mouse moves the crosshair (D-097) */
	UPROPERTY(Config)
	float AimSensitivity = 1.0f;

	UPROPERTY(Config)
	bool bExperimentalNaniteSkinnedMeshes = false;

	UPROPERTY(Config)
	bool bExperimentalNaniteFoliage = false;

	/** Set once the first run hardware detection has picked a preset */
	UPROPERTY(Config)
	bool bHardwareDetectionDone = false;

	UPROPERTY(Config)
	ECyberQualityPreset DetectedPreset = ECyberQualityPreset::High;

public:

	UCyberGameUserSettings(const FObjectInitializer& ObjectInitializer);

	static UCyberGameUserSettings* Get();

	//~ UGameUserSettings
	virtual void SetToDefaults() override;
	virtual void ValidateSettings() override;
	virtual void ApplySettings(bool bCheckForCommandLineOverrides) override;
	virtual void ApplyNonResolutionSettings() override;
	virtual void SaveSettings() override;

	/** Feature defaults for a preset */
	static FCyberFeatureSettings GetPresetFeatures(ECyberQualityPreset Preset);

	/** Engine scalability level for a preset: Low 0 to Epic 3, Ultra stays at 3, Cinematic is Cine (4) */
	static int32 GetPresetScalabilityLevel(ECyberQualityPreset Preset);

	static FString GetPresetName(ECyberQualityPreset Preset);

	/** Hardware detection stops at High on displays above 4 megapixels (D-031); the player can still raise it */
	static ECyberQualityPreset CapPresetForDisplay(ECyberQualityPreset Preset, const FIntPoint& Display);

	/** Where a fresh start and Reset to defaults begin: the capped detected preset, or High before any detection */
	ECyberQualityPreset GetStartingPreset() const;

	/** Selects a preset and resets every feature to its defaults. Leaves the experimental options off below Ultra and Cinematic. */
	void SetQualityPreset(ECyberQualityPreset Preset);

	ECyberQualityPreset GetQualityPreset() const { return QualityPreset; }
	const FCyberFeatureSettings& GetFeatures() const { return Features; }
	float GetFieldOfView() const { return FieldOfView; }

	/** True when the first person weapon and arms are drawn. Off by default (D-094). */
	bool IsWeaponShownOnScreen() const { return bShowWeaponOnScreen; }

	/** Multiplies the reticle speed of the mouse in the modes that move a crosshair (D-097) */
	float GetAimSensitivity() const { return AimSensitivity; }

	/** The multiplier the running game uses, 1 when there are no saved settings yet */
	static float GetAimSensitivityOrDefault();

	/** Applies the weapon visibility to every local player's pawn */
	static void ApplyWeaponVisibility(UWorld* World);

	/** True when a feature differs from the defaults of the selected preset */
	bool IsCustomized() const { return Features != GetPresetFeatures(QualityPreset); }

	/** Everything a menu shows, as one value */
	FCyberSettingsState GetState() const;

	/** Takes a whole state. bApplyAndSave applies everything and writes the config; otherwise only the rendering side is applied. */
	void SetState(const FCyberSettingsState& State, bool bApplyAndSave);

	/** The state Reset to defaults returns to: the detected preset when there is one, High otherwise */
	FCyberSettingsState GetDefaultState() const;

	/** True when a staged startup option differs from what the running engine uses */
	bool IsRestartRequired() const;

	/** Runs the engine hardware benchmark and maps its result to a preset. Does not apply anything. */
	ECyberQualityPreset RunHardwareDetection();

	bool HasRunHardwareDetection() const { return bHardwareDetectionDone; }
	ECyberQualityPreset GetDetectedPreset() const { return DetectedPreset; }

	/** Gives every local player's view target camera the configured field of view */
	void ApplyFieldOfView(UWorld* World) const;

	/** One line per option with the console variables it drives and their live values */
	TArray<FString> DescribeLiveState() const;

protected:

	void ApplyFeatureCVars();
	void StageStartupCVars();
};
