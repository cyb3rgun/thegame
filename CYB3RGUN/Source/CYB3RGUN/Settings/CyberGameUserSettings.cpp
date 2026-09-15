// CYB3RGUN THEGAME. Player facing graphics settings: presets, feature toggles, hardware detection.

#include "CyberGameUserSettings.h"
#include "CyberSettingsOptions.h"
#include "Camera/CameraComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Misc/ConfigCacheIni.h"
#include "Scalability.h"

DEFINE_LOG_CATEGORY_STATIC(LogCyberSettings, Log, All);

namespace CyberSettings
{
	/** Bump when the saved layout or the preset table changes; older saves reset to the preset defaults. 2: G03-B03 presets,
	 *  3: Nanite on in High and Epic (D-086) */
	constexpr int32 CurrentVersion = 3;

	/** Above this many desktop pixels hardware detection stops at High (D-031) */
	constexpr int64 LargeDisplayPixels = 4000000;

	/** Startup only engine switches live in this engine config section */
	const TCHAR* StartupSection = TEXT("ConsoleVariables");

	/**
	 *  Sets a console variable by code. Code outranks project settings and scalability, so a preset change
	 *  cannot undo a player's toggle. Unchanged values are left alone, some variables rebuild render state.
	 */
	void SetInt(const TCHAR* Name, int32 Value)
	{
		IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(Name);
		if (!CVar)
		{
			UE_LOG(LogCyberSettings, Warning, TEXT("Console variable %s does not exist in this engine"), Name);
			return;
		}
		if (CVar->GetInt() != Value)
		{
			CVar->Set(Value, ECVF_SetByCode);
		}
	}

	void SetFloat(const TCHAR* Name, float Value)
	{
		IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(Name);
		if (!CVar)
		{
			UE_LOG(LogCyberSettings, Warning, TEXT("Console variable %s does not exist in this engine"), Name);
			return;
		}
		if (!FMath::IsNearlyEqual(CVar->GetFloat(), Value))
		{
			CVar->Set(Value, ECVF_SetByCode);
		}
	}

	int32 GetInt(const TCHAR* Name)
	{
		const IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(Name);
		return CVar ? CVar->GetInt() : -1;
	}

	FString GetString(const TCHAR* Name)
	{
		const IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(Name);
		return CVar ? CVar->GetString() : TEXT("missing");
	}
}

UCyberGameUserSettings::UCyberGameUserSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UCyberGameUserSettings* UCyberGameUserSettings::Get()
{
	return GEngine ? Cast<UCyberGameUserSettings>(GEngine->GetGameUserSettings()) : nullptr;
}

FCyberFeatureSettings UCyberGameUserSettings::GetPresetFeatures(ECyberQualityPreset Preset)
{
	FCyberFeatureSettings F;
	switch (Preset)
	{
	case ECyberQualityPreset::Low:
		F.bMegaLights = false;
		F.GlobalIllumination = ECyberGIMode::Off;
		F.VirtualShadowMaps = ECyberShadowQuality::Off;
		F.VolumetricFog = ECyberFogQuality::Off;
		F.AntiAliasing = ECyberAntiAliasing::TSRPerformance;
		// Nanite needs Virtual Shadow Maps, which Low leaves off
		F.bNanite = false;
		F.EffectsQuality = 0;
		F.ViewDistanceQuality = 0;
		F.bMotionBlur = false;
		F.ResolutionScale = 100;
		break;
	case ECyberQualityPreset::Medium:
		F.bMegaLights = false;
		F.GlobalIllumination = ECyberGIMode::LumenLite;
		F.VirtualShadowMaps = ECyberShadowQuality::Low;
		F.VolumetricFog = ECyberFogQuality::Off;
		F.AntiAliasing = ECyberAntiAliasing::TSRBalanced;
		// Nanite stays off in Medium (D-086): its plants draw their fallback meshes, which keep their leaves
		F.bNanite = false;
		F.EffectsQuality = 1;
		F.ViewDistanceQuality = 1;
		F.bMotionBlur = false;
		F.ResolutionScale = 100;
		break;
	case ECyberQualityPreset::High:
		// the default preset (D-030, run 4 in docs/benchmark.md): MegaLights improves the 1 percent low, TSR Quality
		// is cheaper than native without anti aliasing, effects and view distance Epic cost nothing measurable
		F.bMegaLights = true;
		F.GlobalIllumination = ECyberGIMode::Lumen;
		F.VirtualShadowMaps = ECyberShadowQuality::High;
		F.VolumetricFog = ECyberFogQuality::Low;
		F.AntiAliasing = ECyberAntiAliasing::TSRQuality;
		// production geometry exists now, the imported plants among it: Nanite runs here, Virtual Shadow Maps are on (D-086)
		F.bNanite = true;
		F.EffectsQuality = 3;
		F.ViewDistanceQuality = 3;
		F.bMotionBlur = false;
		F.ResolutionScale = 100;
		break;
	case ECyberQualityPreset::Epic:
		F.bMegaLights = true;
		F.GlobalIllumination = ECyberGIMode::Lumen;
		F.VirtualShadowMaps = ECyberShadowQuality::Epic;
		F.VolumetricFog = ECyberFogQuality::Medium;
		// TSR Native costs 6.5 ms more than Quality at 5120 x 1440; native resolution is left to Ultra
		F.AntiAliasing = ECyberAntiAliasing::TSRQuality;
		// Nanite with Virtual Shadow Maps Epic, as in High (D-086)
		F.bNanite = true;
		F.EffectsQuality = 3;
		F.ViewDistanceQuality = 3;
		F.bMotionBlur = false;
		F.ResolutionScale = 100;
		break;
	case ECyberQualityPreset::Ultra:
		// maximum game quality (D-028): everything on, native resolution without upscaling, Nanite with Virtual
		// Shadow Maps Epic, fog and effects at their highest playable setting; the Cine level stays out
		F.bMegaLights = true;
		F.GlobalIllumination = ECyberGIMode::Lumen;
		F.VirtualShadowMaps = ECyberShadowQuality::Epic;
		F.VolumetricFog = ECyberFogQuality::Medium;
		F.AntiAliasing = ECyberAntiAliasing::TSRNative;
		F.bNanite = true;
		F.EffectsQuality = 3;
		F.ViewDistanceQuality = 3;
		F.bMotionBlur = false;
		F.ResolutionScale = 100;
		break;
	case ECyberQualityPreset::Cinematic:
	default:
		// screenshots and video capture (D-029): the engine's Cine level, fog at its finest grid, motion blur for footage
		F.bMegaLights = true;
		F.GlobalIllumination = ECyberGIMode::Lumen;
		F.VirtualShadowMaps = ECyberShadowQuality::Epic;
		F.VolumetricFog = ECyberFogQuality::High;
		F.AntiAliasing = ECyberAntiAliasing::TSRNative;
		F.bNanite = true;
		F.EffectsQuality = 3;
		F.ViewDistanceQuality = 3;
		F.bMotionBlur = true;
		F.ResolutionScale = 100;
		break;
	}
	return F;
}

int32 UCyberGameUserSettings::GetPresetScalabilityLevel(ECyberQualityPreset Preset)
{
	// Ultra is a game preset and stays on the Epic level; only Cinematic uses the engine's Cine level
	switch (Preset)
	{
	case ECyberQualityPreset::Low: return 0;
	case ECyberQualityPreset::Medium: return 1;
	case ECyberQualityPreset::High: return 2;
	case ECyberQualityPreset::Epic: return 3;
	case ECyberQualityPreset::Ultra: return 3;
	case ECyberQualityPreset::Cinematic: return 4;
	default: return 2;
	}
}

ECyberQualityPreset UCyberGameUserSettings::CapPresetForDisplay(ECyberQualityPreset Preset, const FIntPoint& Display)
{
	// the engine benchmark rates the GPU and knows nothing about the display: on the 7.4 megapixel development
	// display it picked Epic, which ran at 36 fps while High ran at 74 (D-031)
	const int64 Pixels = static_cast<int64>(Display.X) * static_cast<int64>(Display.Y);
	if (Pixels > CyberSettings::LargeDisplayPixels && static_cast<int32>(Preset) > static_cast<int32>(ECyberQualityPreset::High))
	{
		return ECyberQualityPreset::High;
	}
	return Preset;
}

ECyberQualityPreset UCyberGameUserSettings::GetStartingPreset() const
{
	return bHardwareDetectionDone ? CapPresetForDisplay(DetectedPreset, GetDesktopResolution()) : ECyberQualityPreset::High;
}

FString UCyberGameUserSettings::GetPresetName(ECyberQualityPreset Preset)
{
	return FCyberSettingsOptions::GetValueLabel(ECyberSettingOption::Preset, static_cast<int32>(Preset)).ToString();
}

void UCyberGameUserSettings::SetToDefaults()
{
	Super::SetToDefaults();

	QualityPreset = GetStartingPreset();
	Features = GetPresetFeatures(QualityPreset);
	FieldOfView = 90.0f;
	bExperimentalNaniteSkinnedMeshes = false;
	bExperimentalNaniteFoliage = false;
	FullscreenMode = EWindowMode::WindowedFullscreen;
	CyberSettingsVersion = CyberSettings::CurrentVersion;
}

void UCyberGameUserSettings::ValidateSettings()
{
	Super::ValidateSettings();

	if (CyberSettingsVersion != CyberSettings::CurrentVersion)
	{
		// a detection taken before the display cap existed is capped now, so the new defaults start where they should
		if (bHardwareDetectionDone)
		{
			DetectedPreset = CapPresetForDisplay(DetectedPreset, GetDesktopResolution());
		}
		QualityPreset = GetStartingPreset();
		UE_LOG(LogCyberSettings, Log, TEXT("Settings version %d is outdated, resetting CYB3RGUN settings to the %s preset defaults"), CyberSettingsVersion, *GetPresetName(QualityPreset));
		Features = GetPresetFeatures(QualityPreset);
		FieldOfView = 90.0f;
		bExperimentalNaniteSkinnedMeshes = false;
		bExperimentalNaniteFoliage = false;
		CyberSettingsVersion = CyberSettings::CurrentVersion;
	}

	FieldOfView = FMath::Clamp(FieldOfView, 60.0f, 120.0f);
	Features.EffectsQuality = FMath::Clamp(Features.EffectsQuality, 0, 3);
	Features.ViewDistanceQuality = FMath::Clamp(Features.ViewDistanceQuality, 0, 3);
	Features.ResolutionScale = FMath::Clamp(Features.ResolutionScale, 100, 200);
	if (!FCyberSettingsOptions::IsUltraOrAbove(QualityPreset))
	{
		Features.ResolutionScale = 100;
		bExperimentalNaniteSkinnedMeshes = false;
		bExperimentalNaniteFoliage = false;
	}
}

void UCyberGameUserSettings::ApplySettings(bool bCheckForCommandLineOverrides)
{
	// first launch outside the editor: let the engine benchmark pick the starting preset, the player can override it later
	if (!GIsEditor && !bHardwareDetectionDone)
	{
		SetQualityPreset(RunHardwareDetection());
	}

	Super::ApplySettings(bCheckForCommandLineOverrides);
}

void UCyberGameUserSettings::SetQualityPreset(ECyberQualityPreset Preset)
{
	QualityPreset = Preset;
	Features = GetPresetFeatures(Preset);
	if (!FCyberSettingsOptions::IsUltraOrAbove(Preset))
	{
		bExperimentalNaniteSkinnedMeshes = false;
		bExperimentalNaniteFoliage = false;
	}
}

void UCyberGameUserSettings::ApplyNonResolutionSettings()
{
	// the preset fills every scalability group, the density and distance toggles then override their own groups
	ScalabilityQuality.SetFromSingleQualityLevel(GetPresetScalabilityLevel(QualityPreset));
	ScalabilityQuality.EffectsQuality = FMath::Clamp(Features.EffectsQuality, 0, 3);
	ScalabilityQuality.ViewDistanceQuality = FMath::Clamp(Features.ViewDistanceQuality, 0, 3);

	Super::ApplyNonResolutionSettings();
	ApplyFeatureCVars();

	UE_LOG(LogCyberSettings, Log, TEXT("Settings applied: preset %s%s, restart required %d"), *GetPresetName(QualityPreset), IsCustomized() ? TEXT(" (custom)") : TEXT(""), IsRestartRequired() ? 1 : 0);
}

void UCyberGameUserSettings::ApplyFeatureCVars()
{
	using namespace CyberSettings;
	const FCyberFeatureSettings& F = Features;

	// MegaLights: the project default the renderer reads every frame, per light and post process overrides still apply
	SetInt(TEXT("r.MegaLights.EnableForProject"), F.bMegaLights ? 1 : 0);

	// global illumination and the reflections that belong to it
	switch (F.GlobalIllumination)
	{
	case ECyberGIMode::Off:
		SetInt(TEXT("r.DynamicGlobalIlluminationMethod"), 0);
		SetInt(TEXT("r.ReflectionMethod"), 2);
		break;
	case ECyberGIMode::LumenLite:
		// Irradiance Field Gather, the engine's faster gather targeted at mid range hardware, with screen space reflections
		SetInt(TEXT("r.DynamicGlobalIlluminationMethod"), 1);
		SetInt(TEXT("r.Lumen.DiffuseIndirect.Allow"), 1);
		SetInt(TEXT("r.Lumen.FinalGatherMethod"), 0);
		SetInt(TEXT("r.ReflectionMethod"), 2);
		break;
	case ECyberGIMode::Lumen:
	default:
		SetInt(TEXT("r.DynamicGlobalIlluminationMethod"), 1);
		SetInt(TEXT("r.Lumen.DiffuseIndirect.Allow"), 1);
		SetInt(TEXT("r.Lumen.FinalGatherMethod"), 1);
		SetInt(TEXT("r.ReflectionMethod"), 1);
		SetInt(TEXT("r.Lumen.Reflections.Allow"), 1);
		break;
	}

	// Virtual Shadow Maps, off falls back to conventional shadow maps
	if (F.VirtualShadowMaps == ECyberShadowQuality::Off)
	{
		SetInt(TEXT("r.Shadow.Virtual.Enable"), 0);
	}
	else
	{
		struct FVsmLevel { float BiasDirectional; float BiasLocal; int32 RaysDirectional; int32 RaysLocal; int32 Pages; };
		static const FVsmLevel Levels[] = {
			{ 1.0f, 2.0f, 0, 0, 1024 },  // Low
			{ 0.5f, 1.0f, 4, 4, 2048 },  // Medium
			{ 0.0f, 0.0f, 8, 4, 2048 },  // High, the engine's High shadow group
			{ -1.5f, 0.0f, 8, 8, 4096 }, // Epic, the engine's Epic shadow group
		};
		const FVsmLevel& L = Levels[FMath::Clamp(static_cast<int32>(F.VirtualShadowMaps) - 1, 0, 3)];
		SetInt(TEXT("r.Shadow.Virtual.Enable"), 1);
		SetFloat(TEXT("r.Shadow.Virtual.ResolutionLodBiasDirectional"), L.BiasDirectional);
		SetFloat(TEXT("r.Shadow.Virtual.ResolutionLodBiasDirectionalMoving"), L.BiasDirectional);
		SetFloat(TEXT("r.Shadow.Virtual.ResolutionLodBiasLocal"), L.BiasLocal);
		SetFloat(TEXT("r.Shadow.Virtual.ResolutionLodBiasLocalMoving"), L.BiasLocal + 1.0f);
		SetInt(TEXT("r.Shadow.Virtual.SMRT.RayCountDirectional"), L.RaysDirectional);
		SetInt(TEXT("r.Shadow.Virtual.SMRT.RayCountLocal"), L.RaysLocal);
		SetInt(TEXT("r.Shadow.Virtual.MaxPhysicalPages"), L.Pages);
	}

	// volumetric fog, quality is the froxel grid resolution
	if (F.VolumetricFog == ECyberFogQuality::Off)
	{
		SetInt(TEXT("r.VolumetricFog"), 0);
	}
	else
	{
		static const int32 PixelSize[] = { 16, 12, 8 };
		static const int32 SizeZ[] = { 64, 96, 128 };
		const int32 Level = FMath::Clamp(static_cast<int32>(F.VolumetricFog) - 1, 0, 2);
		SetInt(TEXT("r.VolumetricFog"), 1);
		SetInt(TEXT("r.VolumetricFog.GridPixelSize"), PixelSize[Level]);
		SetInt(TEXT("r.VolumetricFog.GridSizeZ"), SizeZ[Level]);
	}

	// anti aliasing and upscaling: TSR at an internal resolution, or nothing at all. The resolution scale multiplies
	// that percentage, so Ultra at TSR Native and 200 percent renders twice the display resolution per axis
	static const float ScreenPercentage[] = { 100.0f, 100.0f, 66.7f, 58.0f, 50.0f };
	const float Scale = FMath::Clamp(F.ResolutionScale, 100, 200) / 100.0f;
	SetInt(TEXT("r.AntiAliasingMethod"), F.AntiAliasing == ECyberAntiAliasing::Off ? 0 : 4);
	SetFloat(TEXT("r.ScreenPercentage"), FMath::Min(ScreenPercentage[FMath::Clamp(static_cast<int32>(F.AntiAliasing), 0, 4)] * Scale, 200.0f));

	// Nanite only runs together with Virtual Shadow Maps, see FCyberSettingsOptions::IsNaniteActive
	SetInt(TEXT("r.Nanite"), FCyberSettingsOptions::IsNaniteActive(F) ? 1 : 0);
	SetInt(TEXT("r.MotionBlurQuality"), F.bMotionBlur ? 4 : 0);
}

void UCyberGameUserSettings::SaveSettings()
{
	Super::SaveSettings();
	StageStartupCVars();
}

void UCyberGameUserSettings::StageStartupCVars()
{
	// read only engine switches: the engine reads this section of its config at startup, before rendering begins
	if (!GConfig)
	{
		return;
	}
	GConfig->SetString(CyberSettings::StartupSection, TEXT("r.Nanite.AllowSkinnedMeshes"), bExperimentalNaniteSkinnedMeshes ? TEXT("1") : TEXT("0"), GEngineIni);
	GConfig->SetString(CyberSettings::StartupSection, TEXT("r.Nanite.Foliage"), bExperimentalNaniteFoliage ? TEXT("1") : TEXT("0"), GEngineIni);
	GConfig->SetString(CyberSettings::StartupSection, TEXT("r.Nanite.AllowAssemblies"), bExperimentalNaniteFoliage ? TEXT("1") : TEXT("0"), GEngineIni);
	GConfig->Flush(false, GEngineIni);
}

bool UCyberGameUserSettings::IsRestartRequired() const
{
	using namespace CyberSettings;
	return GetInt(TEXT("r.Nanite.AllowSkinnedMeshes")) != (bExperimentalNaniteSkinnedMeshes ? 1 : 0)
		|| GetInt(TEXT("r.Nanite.Foliage")) != (bExperimentalNaniteFoliage ? 1 : 0);
}

FCyberSettingsState UCyberGameUserSettings::GetState() const
{
	FCyberSettingsState State;
	State.Preset = QualityPreset;
	State.Features = Features;
	State.FrameRateLimit = GetFrameRateLimit();
	State.bVSync = IsVSyncEnabled();
	State.Resolution = GetScreenResolution();
	State.WindowMode = static_cast<int32>(GetFullscreenMode());
	State.FieldOfView = FieldOfView;
	State.bExperimentalNaniteSkinnedMeshes = bExperimentalNaniteSkinnedMeshes;
	State.bExperimentalNaniteFoliage = bExperimentalNaniteFoliage;
	return State;
}

void UCyberGameUserSettings::SetState(const FCyberSettingsState& State, bool bApplyAndSave)
{
	QualityPreset = State.Preset;
	Features = State.Features;
	FieldOfView = FMath::Clamp(State.FieldOfView, 60.0f, 120.0f);
	const bool bUltra = FCyberSettingsOptions::IsUltraOrAbove(QualityPreset);
	Features.ResolutionScale = bUltra ? FMath::Clamp(Features.ResolutionScale, 100, 200) : 100;
	bExperimentalNaniteSkinnedMeshes = bUltra && State.bExperimentalNaniteSkinnedMeshes;
	bExperimentalNaniteFoliage = bUltra && State.bExperimentalNaniteFoliage;

	SetFrameRateLimit(State.FrameRateLimit);
	SetVSyncEnabled(State.bVSync);
	if (State.Resolution.X > 0 && State.Resolution.Y > 0)
	{
		SetScreenResolution(State.Resolution);
	}
	SetFullscreenMode(static_cast<EWindowMode::Type>(FMath::Clamp(State.WindowMode, 0, 2)));

	if (bApplyAndSave)
	{
		ApplySettings(false);
	}
	else
	{
		ApplyNonResolutionSettings();
	}
}

FCyberSettingsState UCyberGameUserSettings::GetDefaultState() const
{
	FCyberSettingsState State = GetState();
	State.Preset = GetStartingPreset();
	State.Features = GetPresetFeatures(State.Preset);
	State.FrameRateLimit = 0.0f;
	State.bVSync = false;
	State.Resolution = GetDesktopResolution();
	State.WindowMode = static_cast<int32>(EWindowMode::WindowedFullscreen);
	State.FieldOfView = 90.0f;
	State.bExperimentalNaniteSkinnedMeshes = false;
	State.bExperimentalNaniteFoliage = false;
	return State;
}

ECyberQualityPreset UCyberGameUserSettings::RunHardwareDetection()
{
	RunHardwareBenchmark();

	// the engine picks a level per scalability group, the preset is their rounded average
	const Scalability::FQualityLevels& Q = ScalabilityQuality;
	const int32 Groups[] = { Q.ViewDistanceQuality, Q.AntiAliasingQuality, Q.ShadowQuality, Q.GlobalIlluminationQuality, Q.ReflectionQuality,
		Q.PostProcessQuality, Q.TextureQuality, Q.EffectsQuality, Q.FoliageQuality, Q.ShadingQuality };
	int32 Sum = 0;
	for (const int32 Level : Groups)
	{
		Sum += Level;
	}
	const int32 Level = FMath::Clamp(FMath::RoundToInt(static_cast<float>(Sum) / UE_ARRAY_COUNT(Groups)), 0, 3);

	const ECyberQualityPreset Rated = static_cast<ECyberQualityPreset>(Level);
	const FIntPoint Display = GetDesktopResolution();
	DetectedPreset = CapPresetForDisplay(Rated, Display);
	bHardwareDetectionDone = true;

	UE_LOG(LogCyberSettings, Log, TEXT("Hardware detection: CPU index %.1f, GPU index %.1f, groups view %d aa %d shadow %d gi %d reflection %d post %d texture %d effects %d foliage %d shading %d, rated %s, display %d x %d, preset %s%s"),
		LastCPUBenchmarkResult, LastGPUBenchmarkResult, Q.ViewDistanceQuality, Q.AntiAliasingQuality, Q.ShadowQuality, Q.GlobalIlluminationQuality,
		Q.ReflectionQuality, Q.PostProcessQuality, Q.TextureQuality, Q.EffectsQuality, Q.FoliageQuality, Q.ShadingQuality, *GetPresetName(Rated),
		Display.X, Display.Y, *GetPresetName(DetectedPreset), DetectedPreset != Rated ? TEXT(" (capped for a display above 4 megapixels)") : TEXT(""));

	return DetectedPreset;
}

void UCyberGameUserSettings::ApplyFieldOfView(UWorld* World) const
{
	if (!World)
	{
		return;
	}

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* PC = It->Get();
		if (!PC || !PC->IsLocalController())
		{
			continue;
		}
		const AActor* ViewTarget = PC->GetViewTarget();
		UCameraComponent* Camera = ViewTarget ? ViewTarget->FindComponentByClass<UCameraComponent>() : nullptr;
		if (Camera && !FMath::IsNearlyEqual(Camera->FieldOfView, FieldOfView))
		{
			Camera->SetFieldOfView(FieldOfView);
		}
	}
}

TArray<FString> UCyberGameUserSettings::DescribeLiveState() const
{
	using namespace CyberSettings;
	const FCyberSettingsState State = GetState();
	TArray<FString> Lines;
	Lines.Add(FString::Printf(TEXT("Preset %s%s, detected %s, restart required %d, overall scalability %d"),
		*GetPresetName(QualityPreset), IsCustomized() ? TEXT(" (custom)") : TEXT(""),
		bHardwareDetectionDone ? *GetPresetName(DetectedPreset) : TEXT("not run"), IsRestartRequired() ? 1 : 0, GetOverallScalabilityLevel()));

	for (int32 i = 0; i < static_cast<int32>(ECyberSettingOption::Count); ++i)
	{
		const ECyberSettingOption Option = static_cast<ECyberSettingOption>(i);
		Lines.Add(FString::Printf(TEXT("  %-14s = %-16s drives %s"), *FCyberSettingsOptions::GetOptionName(Option),
			*FCyberSettingsOptions::GetValueLabel(Option, FCyberSettingsOptions::GetValueIndex(Option, State)).ToString(),
			*FCyberSettingsOptions::GetDrivenCVars(Option)));
	}

	const TCHAR* Watched[] = {
		TEXT("r.MegaLights.EnableForProject"), TEXT("r.DynamicGlobalIlluminationMethod"), TEXT("r.Lumen.FinalGatherMethod"), TEXT("r.ReflectionMethod"),
		TEXT("r.Shadow.Virtual.Enable"), TEXT("r.Shadow.Virtual.ResolutionLodBiasDirectional"), TEXT("r.VolumetricFog"), TEXT("r.VolumetricFog.GridPixelSize"),
		TEXT("r.AntiAliasingMethod"), TEXT("r.ScreenPercentage"), TEXT("r.Nanite"), TEXT("sg.EffectsQuality"), TEXT("sg.ViewDistanceQuality"),
		TEXT("r.MotionBlurQuality"), TEXT("t.MaxFPS"), TEXT("r.VSync"), TEXT("r.Nanite.AllowSkinnedMeshes"), TEXT("r.Nanite.Foliage"), TEXT("r.Nanite.AllowAssemblies") };
	FString Live = TEXT("  live:");
	for (const TCHAR* Name : Watched)
	{
		Live += FString::Printf(TEXT(" %s=%s"), Name, *GetString(Name));
	}
	Lines.Add(Live);
	return Lines;
}
