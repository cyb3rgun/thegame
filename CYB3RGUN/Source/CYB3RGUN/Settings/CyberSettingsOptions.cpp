// CYB3RGUN THEGAME. One registry of settings options shared by the menu and the console.

#include "CyberSettingsOptions.h"
#include "CyberGameUserSettings.h"
#include "Kismet/KismetSystemLibrary.h"

#define LOCTEXT_NAMESPACE "CyberSettingsOptions"

namespace
{
	FText OffOn(int32 Index)
	{
		return Index == 0 ? LOCTEXT("Off", "Off") : LOCTEXT("On", "On");
	}

	int32 ClosestIndex(const TArray<float>& Values, float Value)
	{
		int32 Best = 0;
		for (int32 i = 1; i < Values.Num(); ++i)
		{
			if (FMath::Abs(Values[i] - Value) < FMath::Abs(Values[Best] - Value))
			{
				Best = i;
			}
		}
		return Best;
	}
}

const TArray<float>& FCyberSettingsOptions::GetFrameRateCaps()
{
	// 0 is unlimited and sits last so the list reads from strict to free
	static const TArray<float> Caps = { 30.0f, 60.0f, 120.0f, 144.0f, 240.0f, 0.0f };
	return Caps;
}

const TArray<float>& FCyberSettingsOptions::GetFieldOfViews()
{
	static TArray<float> Values;
	if (Values.IsEmpty())
	{
		for (float Fov = 60.0f; Fov <= 120.0f + KINDA_SMALL_NUMBER; Fov += 5.0f)
		{
			Values.Add(Fov);
		}
	}
	return Values;
}

const TArray<float>& FCyberSettingsOptions::GetResolutionScales()
{
	// TSR accepts at most twice the display resolution per axis (kMaxTSRResolutionFraction)
	static const TArray<float> Scales = { 100.0f, 125.0f, 150.0f, 175.0f, 200.0f };
	return Scales;
}

const TArray<FIntPoint>& FCyberSettingsOptions::GetResolutions()
{
	static TArray<FIntPoint> Resolutions;
	if (Resolutions.IsEmpty())
	{
		UKismetSystemLibrary::GetSupportedFullscreenResolutions(Resolutions);
		if (UCyberGameUserSettings* Settings = UCyberGameUserSettings::Get())
		{
			Resolutions.AddUnique(Settings->GetDesktopResolution());
		}
		if (Resolutions.IsEmpty())
		{
			Resolutions.Add(FIntPoint(1920, 1080));
		}
		Resolutions.Sort([](const FIntPoint& A, const FIntPoint& B)
		{
			return A.X * A.Y < B.X * B.Y || (A.X * A.Y == B.X * B.Y && A.X < B.X);
		});
	}
	return Resolutions;
}

FText FCyberSettingsOptions::GetOptionLabel(ECyberSettingOption Option)
{
	switch (Option)
	{
	case ECyberSettingOption::Preset: return LOCTEXT("Preset", "Quality preset");
	case ECyberSettingOption::MegaLights: return LOCTEXT("MegaLights", "MegaLights");
	case ECyberSettingOption::GlobalIllumination: return LOCTEXT("GI", "Global illumination");
	case ECyberSettingOption::VirtualShadowMaps: return LOCTEXT("VSM", "Virtual shadow maps");
	case ECyberSettingOption::VolumetricFog: return LOCTEXT("Fog", "Volumetric fog");
	case ECyberSettingOption::AntiAliasing: return LOCTEXT("AA", "Anti aliasing and upscaling");
	case ECyberSettingOption::ResolutionScale: return LOCTEXT("ResolutionScale", "Resolution scale");
	case ECyberSettingOption::Nanite: return LOCTEXT("Nanite", "Nanite static meshes");
	case ECyberSettingOption::EffectsDensity: return LOCTEXT("Effects", "Effects and particle density");
	case ECyberSettingOption::ViewDistance: return LOCTEXT("ViewDistance", "View distance");
	case ECyberSettingOption::MotionBlur: return LOCTEXT("MotionBlur", "Motion blur");
	case ECyberSettingOption::FrameRateCap: return LOCTEXT("FrameCap", "Frame rate cap");
	case ECyberSettingOption::VSync: return LOCTEXT("VSync", "VSync");
	case ECyberSettingOption::Resolution: return LOCTEXT("Resolution", "Resolution");
	case ECyberSettingOption::WindowMode: return LOCTEXT("WindowMode", "Window mode");
	case ECyberSettingOption::FieldOfView: return LOCTEXT("Fov", "Field of view");
	case ECyberSettingOption::ExperimentalNaniteSkinnedMeshes: return LOCTEXT("NaniteSkinned", "EXPERIMENTAL Nanite skeletal meshes (restart)");
	case ECyberSettingOption::ExperimentalNaniteFoliage: return LOCTEXT("NaniteFoliage", "EXPERIMENTAL Nanite foliage (restart)");
	default: return FText::GetEmpty();
	}
}

FString FCyberSettingsOptions::GetOptionName(ECyberSettingOption Option)
{
	switch (Option)
	{
	case ECyberSettingOption::Preset: return TEXT("Preset");
	case ECyberSettingOption::MegaLights: return TEXT("MegaLights");
	case ECyberSettingOption::GlobalIllumination: return TEXT("GI");
	case ECyberSettingOption::VirtualShadowMaps: return TEXT("VSM");
	case ECyberSettingOption::VolumetricFog: return TEXT("Fog");
	case ECyberSettingOption::AntiAliasing: return TEXT("AA");
	case ECyberSettingOption::ResolutionScale: return TEXT("ResScale");
	case ECyberSettingOption::Nanite: return TEXT("Nanite");
	case ECyberSettingOption::EffectsDensity: return TEXT("Effects");
	case ECyberSettingOption::ViewDistance: return TEXT("ViewDistance");
	case ECyberSettingOption::MotionBlur: return TEXT("MotionBlur");
	case ECyberSettingOption::FrameRateCap: return TEXT("FrameCap");
	case ECyberSettingOption::VSync: return TEXT("VSync");
	case ECyberSettingOption::Resolution: return TEXT("Resolution");
	case ECyberSettingOption::WindowMode: return TEXT("WindowMode");
	case ECyberSettingOption::FieldOfView: return TEXT("FOV");
	case ECyberSettingOption::ExperimentalNaniteSkinnedMeshes: return TEXT("NaniteSkinned");
	case ECyberSettingOption::ExperimentalNaniteFoliage: return TEXT("NaniteFoliage");
	default: return FString();
	}
}

FString FCyberSettingsOptions::GetDrivenCVars(ECyberSettingOption Option)
{
	switch (Option)
	{
	case ECyberSettingOption::Preset: return TEXT("sg.* scalability groups (Ultra stays at Epic, Cinematic uses Cine) plus the feature defaults below");
	case ECyberSettingOption::MegaLights: return TEXT("r.MegaLights.EnableForProject");
	case ECyberSettingOption::GlobalIllumination: return TEXT("r.DynamicGlobalIlluminationMethod, r.Lumen.DiffuseIndirect.Allow, r.Lumen.FinalGatherMethod, r.ReflectionMethod, r.Lumen.Reflections.Allow");
	case ECyberSettingOption::VirtualShadowMaps: return TEXT("r.Shadow.Virtual.Enable, r.Shadow.Virtual.ResolutionLodBiasDirectional(Moving), r.Shadow.Virtual.ResolutionLodBiasLocal(Moving), r.Shadow.Virtual.SMRT.RayCountDirectional, r.Shadow.Virtual.SMRT.RayCountLocal, r.Shadow.Virtual.MaxPhysicalPages");
	case ECyberSettingOption::VolumetricFog: return TEXT("r.VolumetricFog, r.VolumetricFog.GridPixelSize, r.VolumetricFog.GridSizeZ");
	case ECyberSettingOption::AntiAliasing: return TEXT("r.AntiAliasingMethod, r.ScreenPercentage");
	case ECyberSettingOption::ResolutionScale: return TEXT("r.ScreenPercentage, multiplied with the anti aliasing mode's percentage");
	case ECyberSettingOption::Nanite: return TEXT("r.Nanite");
	case ECyberSettingOption::EffectsDensity: return TEXT("sg.EffectsQuality");
	case ECyberSettingOption::ViewDistance: return TEXT("sg.ViewDistanceQuality");
	case ECyberSettingOption::MotionBlur: return TEXT("r.MotionBlurQuality");
	case ECyberSettingOption::FrameRateCap: return TEXT("t.MaxFPS");
	case ECyberSettingOption::VSync: return TEXT("r.VSync");
	case ECyberSettingOption::Resolution: return TEXT("r.SetRes (through UGameUserSettings)");
	case ECyberSettingOption::WindowMode: return TEXT("r.SetRes window mode suffix (through UGameUserSettings)");
	case ECyberSettingOption::FieldOfView: return TEXT("none, sets the view target camera's field of view");
	case ECyberSettingOption::ExperimentalNaniteSkinnedMeshes: return TEXT("r.Nanite.AllowSkinnedMeshes (read only, startup)");
	case ECyberSettingOption::ExperimentalNaniteFoliage: return TEXT("r.Nanite.Foliage and r.Nanite.AllowAssemblies (read only, startup)");
	default: return FString();
	}
}

int32 FCyberSettingsOptions::GetValueCount(ECyberSettingOption Option)
{
	switch (Option)
	{
	case ECyberSettingOption::Preset: return 6;
	case ECyberSettingOption::GlobalIllumination: return 3;
	case ECyberSettingOption::VirtualShadowMaps: return 5;
	case ECyberSettingOption::VolumetricFog: return 4;
	case ECyberSettingOption::AntiAliasing: return 5;
	case ECyberSettingOption::ResolutionScale: return GetResolutionScales().Num();
	case ECyberSettingOption::EffectsDensity: return 4;
	case ECyberSettingOption::ViewDistance: return 4;
	case ECyberSettingOption::FrameRateCap: return GetFrameRateCaps().Num();
	case ECyberSettingOption::Resolution: return GetResolutions().Num();
	case ECyberSettingOption::WindowMode: return 3;
	case ECyberSettingOption::FieldOfView: return GetFieldOfViews().Num();
	default: return 2;
	}
}

FText FCyberSettingsOptions::GetValueLabel(ECyberSettingOption Option, int32 Index)
{
	static const FText Presets[] = { LOCTEXT("Low", "Low"), LOCTEXT("Medium", "Medium"), LOCTEXT("High", "High"), LOCTEXT("Epic", "Epic"), LOCTEXT("Ultra", "Ultra"), LOCTEXT("Cinematic", "Cinematic") };
	static const FText GI[] = { LOCTEXT("Off", "Off"), LOCTEXT("LumenLite", "Lumen Lite"), LOCTEXT("Lumen", "Lumen") };
	static const FText Levels5[] = { LOCTEXT("Off", "Off"), LOCTEXT("Low", "Low"), LOCTEXT("Medium", "Medium"), LOCTEXT("High", "High"), LOCTEXT("Epic", "Epic") };
	static const FText Levels4Off[] = { LOCTEXT("Off", "Off"), LOCTEXT("Low", "Low"), LOCTEXT("Medium", "Medium"), LOCTEXT("High", "High") };
	static const FText AA[] = { LOCTEXT("Off", "Off"), LOCTEXT("TSRNative", "TSR Native"), LOCTEXT("TSRQuality", "TSR Quality"), LOCTEXT("TSRBalanced", "TSR Balanced"), LOCTEXT("TSRPerformance", "TSR Performance") };
	static const FText Density[] = { LOCTEXT("Low", "Low"), LOCTEXT("Medium", "Medium"), LOCTEXT("High", "High"), LOCTEXT("Epic", "Epic") };
	static const FText Distance[] = { LOCTEXT("Near", "Near"), LOCTEXT("Medium", "Medium"), LOCTEXT("Far", "Far"), LOCTEXT("Epic", "Epic") };
	static const FText Window[] = { LOCTEXT("Fullscreen", "Fullscreen"), LOCTEXT("Borderless", "Borderless"), LOCTEXT("Windowed", "Windowed") };

	const int32 Count = GetValueCount(Option);
	Index = FMath::Clamp(Index, 0, FMath::Max(Count - 1, 0));

	switch (Option)
	{
	case ECyberSettingOption::Preset: return Presets[Index];
	case ECyberSettingOption::GlobalIllumination: return GI[Index];
	case ECyberSettingOption::VirtualShadowMaps: return Levels5[Index];
	case ECyberSettingOption::VolumetricFog: return Levels4Off[Index];
	case ECyberSettingOption::AntiAliasing: return AA[Index];
	case ECyberSettingOption::EffectsDensity: return Density[Index];
	case ECyberSettingOption::ViewDistance: return Distance[Index];
	case ECyberSettingOption::WindowMode: return Window[Index];
	case ECyberSettingOption::FrameRateCap:
	{
		const float Cap = GetFrameRateCaps()[Index];
		return Cap <= 0.0f ? LOCTEXT("Unlimited", "Unlimited") : FText::AsNumber(FMath::RoundToInt(Cap));
	}
	case ECyberSettingOption::Resolution:
	{
		const FIntPoint Res = GetResolutions()[Index];
		return FText::FromString(FString::Printf(TEXT("%d x %d"), Res.X, Res.Y));
	}
	case ECyberSettingOption::FieldOfView:
		return FText::FromString(FString::Printf(TEXT("%d deg"), FMath::RoundToInt(GetFieldOfViews()[Index])));
	case ECyberSettingOption::ResolutionScale:
		return FText::FromString(FString::Printf(TEXT("%d %%"), FMath::RoundToInt(GetResolutionScales()[Index])));
	default:
		return OffOn(Index);
	}
}

FText FCyberSettingsOptions::GetValueDisplayLabel(ECyberSettingOption Option, int32 Index)
{
	if (Option == ECyberSettingOption::Preset && Index == static_cast<int32>(ECyberQualityPreset::Cinematic))
	{
		return LOCTEXT("CinematicCapture", "Cinematic, capture only");
	}
	return GetValueLabel(Option, Index);
}

int32 FCyberSettingsOptions::GetValueIndex(ECyberSettingOption Option, const FCyberSettingsState& State)
{
	const FCyberFeatureSettings& F = State.Features;
	switch (Option)
	{
	case ECyberSettingOption::Preset: return static_cast<int32>(State.Preset);
	case ECyberSettingOption::MegaLights: return F.bMegaLights ? 1 : 0;
	case ECyberSettingOption::GlobalIllumination: return static_cast<int32>(F.GlobalIllumination);
	case ECyberSettingOption::VirtualShadowMaps: return static_cast<int32>(F.VirtualShadowMaps);
	case ECyberSettingOption::VolumetricFog: return static_cast<int32>(F.VolumetricFog);
	case ECyberSettingOption::AntiAliasing: return static_cast<int32>(F.AntiAliasing);
	case ECyberSettingOption::ResolutionScale: return ClosestIndex(GetResolutionScales(), static_cast<float>(F.ResolutionScale));
	case ECyberSettingOption::Nanite: return F.bNanite ? 1 : 0;
	case ECyberSettingOption::EffectsDensity: return FMath::Clamp(F.EffectsQuality, 0, 3);
	case ECyberSettingOption::ViewDistance: return FMath::Clamp(F.ViewDistanceQuality, 0, 3);
	case ECyberSettingOption::MotionBlur: return F.bMotionBlur ? 1 : 0;
	case ECyberSettingOption::FrameRateCap:
	{
		const TArray<float>& Caps = GetFrameRateCaps();
		return State.FrameRateLimit <= 0.0f ? Caps.Num() - 1 : ClosestIndex(Caps, State.FrameRateLimit);
	}
	case ECyberSettingOption::VSync: return State.bVSync ? 1 : 0;
	case ECyberSettingOption::Resolution:
	{
		const TArray<FIntPoint>& Res = GetResolutions();
		const int32 Exact = Res.IndexOfByKey(State.Resolution);
		return Exact != INDEX_NONE ? Exact : Res.Num() - 1;
	}
	case ECyberSettingOption::WindowMode: return FMath::Clamp(State.WindowMode, 0, 2);
	case ECyberSettingOption::FieldOfView: return ClosestIndex(GetFieldOfViews(), State.FieldOfView);
	case ECyberSettingOption::ExperimentalNaniteSkinnedMeshes: return State.bExperimentalNaniteSkinnedMeshes ? 1 : 0;
	case ECyberSettingOption::ExperimentalNaniteFoliage: return State.bExperimentalNaniteFoliage ? 1 : 0;
	default: return 0;
	}
}

void FCyberSettingsOptions::SetValueIndex(ECyberSettingOption Option, FCyberSettingsState& State, int32 Index)
{
	Index = FMath::Clamp(Index, 0, FMath::Max(GetValueCount(Option) - 1, 0));
	FCyberFeatureSettings& F = State.Features;

	switch (Option)
	{
	case ECyberSettingOption::Preset:
		State.Preset = static_cast<ECyberQualityPreset>(Index);
		State.Features = UCyberGameUserSettings::GetPresetFeatures(State.Preset);
		if (!IsUltraOrAbove(State.Preset))
		{
			State.bExperimentalNaniteSkinnedMeshes = false;
			State.bExperimentalNaniteFoliage = false;
		}
		break;
	case ECyberSettingOption::MegaLights: F.bMegaLights = Index == 1; break;
	case ECyberSettingOption::GlobalIllumination: F.GlobalIllumination = static_cast<ECyberGIMode>(Index); break;
	case ECyberSettingOption::VirtualShadowMaps: F.VirtualShadowMaps = static_cast<ECyberShadowQuality>(Index); break;
	case ECyberSettingOption::VolumetricFog: F.VolumetricFog = static_cast<ECyberFogQuality>(Index); break;
	case ECyberSettingOption::AntiAliasing: F.AntiAliasing = static_cast<ECyberAntiAliasing>(Index); break;
	case ECyberSettingOption::ResolutionScale:
		F.ResolutionScale = IsUltraOrAbove(State.Preset) ? FMath::RoundToInt(GetResolutionScales()[Index]) : 100;
		break;
	case ECyberSettingOption::Nanite: F.bNanite = Index == 1; break;
	case ECyberSettingOption::EffectsDensity: F.EffectsQuality = Index; break;
	case ECyberSettingOption::ViewDistance: F.ViewDistanceQuality = Index; break;
	case ECyberSettingOption::MotionBlur: F.bMotionBlur = Index == 1; break;
	case ECyberSettingOption::FrameRateCap: State.FrameRateLimit = GetFrameRateCaps()[Index]; break;
	case ECyberSettingOption::VSync: State.bVSync = Index == 1; break;
	case ECyberSettingOption::Resolution: State.Resolution = GetResolutions()[Index]; break;
	case ECyberSettingOption::WindowMode: State.WindowMode = Index; break;
	case ECyberSettingOption::FieldOfView: State.FieldOfView = GetFieldOfViews()[Index]; break;
	case ECyberSettingOption::ExperimentalNaniteSkinnedMeshes:
		State.bExperimentalNaniteSkinnedMeshes = Index == 1 && IsUltraOrAbove(State.Preset);
		break;
	case ECyberSettingOption::ExperimentalNaniteFoliage:
		State.bExperimentalNaniteFoliage = Index == 1 && IsUltraOrAbove(State.Preset);
		break;
	default:
		break;
	}
}

bool FCyberSettingsOptions::IsExperimental(ECyberSettingOption Option)
{
	return Option == ECyberSettingOption::ExperimentalNaniteSkinnedMeshes || Option == ECyberSettingOption::ExperimentalNaniteFoliage;
}

bool FCyberSettingsOptions::NeedsRestart(ECyberSettingOption Option)
{
	return IsExperimental(Option);
}

bool FCyberSettingsOptions::IsAvailable(ECyberSettingOption Option, const FCyberSettingsState& State)
{
	const bool bUltraOnly = IsExperimental(Option) || Option == ECyberSettingOption::ResolutionScale;
	return !bUltraOnly || IsUltraOrAbove(State.Preset);
}

bool FCyberSettingsOptions::IsUltraOrAbove(ECyberQualityPreset Preset)
{
	return Preset == ECyberQualityPreset::Ultra || Preset == ECyberQualityPreset::Cinematic;
}

bool FCyberSettingsOptions::IsNaniteActive(const FCyberFeatureSettings& Features)
{
	// Nanite geometry in conventional cube shadow maps crashed the D3D12 residency manager and hitched above 200 ms
	// with the benchmark level's 60 shadow casting point lights, so Nanite only runs together with Virtual Shadow Maps
	return Features.bNanite && Features.VirtualShadowMaps != ECyberShadowQuality::Off;
}

bool FCyberSettingsOptions::FindOption(const FString& Name, ECyberSettingOption& OutOption)
{
	for (int32 i = 0; i < static_cast<int32>(ECyberSettingOption::Count); ++i)
	{
		const ECyberSettingOption Option = static_cast<ECyberSettingOption>(i);
		if (GetOptionName(Option).Equals(Name, ESearchCase::IgnoreCase))
		{
			OutOption = Option;
			return true;
		}
	}
	return false;
}

bool FCyberSettingsOptions::ParseValue(ECyberSettingOption Option, const FString& Value, int32& OutIndex)
{
	const FString Wanted = Value.Replace(TEXT(" "), TEXT(""));

	// numbers mean the value itself for frame rate, field of view and resolution scale, and an index elsewhere
	if (Wanted.IsNumeric())
	{
		const float Number = FCString::Atof(*Wanted);
		if (Option == ECyberSettingOption::FrameRateCap)
		{
			const TArray<float>& Caps = GetFrameRateCaps();
			OutIndex = Number <= 0.0f ? Caps.Num() - 1 : ClosestIndex(Caps, Number);
			return true;
		}
		if (Option == ECyberSettingOption::FieldOfView)
		{
			OutIndex = ClosestIndex(GetFieldOfViews(), Number);
			return true;
		}
		if (Option == ECyberSettingOption::ResolutionScale)
		{
			OutIndex = ClosestIndex(GetResolutionScales(), Number);
			return true;
		}
		OutIndex = FMath::Clamp(FMath::RoundToInt(Number), 0, GetValueCount(Option) - 1);
		return true;
	}

	for (int32 i = 0; i < GetValueCount(Option); ++i)
	{
		if (GetValueLabel(Option, i).ToString().Replace(TEXT(" "), TEXT("")).Equals(Wanted, ESearchCase::IgnoreCase))
		{
			OutIndex = i;
			return true;
		}
	}
	return false;
}

#undef LOCTEXT_NAMESPACE
