// CYB3RGUN THEGAME. Value types for the graphics settings: presets, feature modes and the full settings state.

#pragma once

#include "CoreMinimal.h"
#include "CyberSettingsTypes.generated.h"

/** Quality presets. Each drives the engine scalability groups and a set of feature defaults. */
UENUM(BlueprintType)
enum class ECyberQualityPreset : uint8
{
	Low,
	Medium,
	High,
	Epic,
	/** Engine Cine scalability plus every feature at its maximum. The experimental options live here. */
	Ultra
};

/** Global illumination. Lumen Lite is Lumen with the Irradiance Field Gather, the engine's mid range tier. */
UENUM(BlueprintType)
enum class ECyberGIMode : uint8
{
	Off,
	LumenLite,
	Lumen
};

/** Virtual Shadow Maps. Off falls back to conventional shadow maps, it does not remove shadows. */
UENUM(BlueprintType)
enum class ECyberShadowQuality : uint8
{
	Off,
	Low,
	Medium,
	High,
	Epic
};

UENUM(BlueprintType)
enum class ECyberFogQuality : uint8
{
	Off,
	Low,
	Medium,
	High
};

/** Anti aliasing and upscaling. The TSR levels differ by internal resolution. */
UENUM(BlueprintType)
enum class ECyberAntiAliasing : uint8
{
	Off,
	TSRNative,
	TSRQuality,
	TSRBalanced,
	TSRPerformance
};

/** Every individual toggle that sits on top of a preset */
USTRUCT(BlueprintType)
struct FCyberFeatureSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Settings")
	bool bMegaLights = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Settings")
	ECyberGIMode GlobalIllumination = ECyberGIMode::Lumen;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Settings")
	ECyberShadowQuality VirtualShadowMaps = ECyberShadowQuality::High;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Settings")
	ECyberFogQuality VolumetricFog = ECyberFogQuality::Low;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Settings")
	ECyberAntiAliasing AntiAliasing = ECyberAntiAliasing::TSRQuality;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Settings")
	bool bNanite = true;

	/** Engine effects scalability group, 0 to 3 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Settings", meta = (ClampMin = 0, ClampMax = 3))
	int32 EffectsQuality = 2;

	/** Engine view distance scalability group, 0 to 3 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Settings", meta = (ClampMin = 0, ClampMax = 3))
	int32 ViewDistanceQuality = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Settings")
	bool bMotionBlur = true;

	bool operator==(const FCyberFeatureSettings& Other) const
	{
		return bMegaLights == Other.bMegaLights && GlobalIllumination == Other.GlobalIllumination && VirtualShadowMaps == Other.VirtualShadowMaps
			&& VolumetricFog == Other.VolumetricFog && AntiAliasing == Other.AntiAliasing && bNanite == Other.bNanite
			&& EffectsQuality == Other.EffectsQuality && ViewDistanceQuality == Other.ViewDistanceQuality && bMotionBlur == Other.bMotionBlur;
	}
	bool operator!=(const FCyberFeatureSettings& Other) const { return !(*this == Other); }
};

/** The complete player facing settings, as one value that menus can edit before applying */
USTRUCT(BlueprintType)
struct FCyberSettingsState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Settings")
	ECyberQualityPreset Preset = ECyberQualityPreset::High;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Settings")
	FCyberFeatureSettings Features;

	/** 0 means unlimited */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Settings")
	float FrameRateLimit = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Settings")
	bool bVSync = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Settings")
	FIntPoint Resolution = FIntPoint::ZeroValue;

	/** EWindowMode: 0 fullscreen, 1 borderless, 2 windowed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Settings")
	int32 WindowMode = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Settings")
	float FieldOfView = 90.0f;

	/** Experimental, Ultra only, needs a restart */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Settings")
	bool bExperimentalNaniteSkinnedMeshes = false;

	/** Experimental, Ultra only, needs a restart */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Settings")
	bool bExperimentalNaniteFoliage = false;
};

/** Every option a settings menu shows, in menu order */
UENUM(BlueprintType)
enum class ECyberSettingOption : uint8
{
	Preset,
	MegaLights,
	GlobalIllumination,
	VirtualShadowMaps,
	VolumetricFog,
	AntiAliasing,
	Nanite,
	EffectsDensity,
	ViewDistance,
	MotionBlur,
	FrameRateCap,
	VSync,
	Resolution,
	WindowMode,
	FieldOfView,
	ExperimentalNaniteSkinnedMeshes,
	ExperimentalNaniteFoliage,
	Count UMETA(Hidden)
};
