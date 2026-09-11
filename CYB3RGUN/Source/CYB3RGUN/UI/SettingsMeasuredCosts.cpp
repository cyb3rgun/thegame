// CYB3RGUN THEGAME. Measured costs of the graphics options, shown next to each option in the settings menu.

#include "SettingsMeasuredCosts.h"

#define LOCTEXT_NAMESPACE "SettingsMeasuredCosts"

bool FSettingsMeasuredCosts::Find(ECyberSettingOption Option, int32 ValueIndex, float& OutMs, bool& bOutAbsolute)
{
	bOutAbsolute = false;
	switch (Option)
	{
	case ECyberSettingOption::Preset:
		// Low is the only preset whose definition is still the one run 4 measured
		if (ValueIndex == static_cast<int32>(ECyberQualityPreset::Low)) { OutMs = 5.88f; bOutAbsolute = true; return true; }
		return false;
	case ECyberSettingOption::MegaLights:
		if (ValueIndex == 1) { OutMs = 0.90f; return true; }
		return false;
	case ECyberSettingOption::GlobalIllumination:
		if (ValueIndex == static_cast<int32>(ECyberGIMode::LumenLite)) { OutMs = -1.26f; return true; }
		if (ValueIndex == static_cast<int32>(ECyberGIMode::Lumen)) { OutMs = 0.10f; return true; }
		return false;
	case ECyberSettingOption::VirtualShadowMaps:
		if (ValueIndex == static_cast<int32>(ECyberShadowQuality::Low)) { OutMs = 0.40f; return true; }
		if (ValueIndex == static_cast<int32>(ECyberShadowQuality::High)) { OutMs = 0.70f; return true; }
		if (ValueIndex == static_cast<int32>(ECyberShadowQuality::Epic)) { OutMs = 1.14f; return true; }
		return false;
	case ECyberSettingOption::VolumetricFog:
		if (ValueIndex == static_cast<int32>(ECyberFogQuality::Low)) { OutMs = 0.47f; return true; }
		if (ValueIndex == static_cast<int32>(ECyberFogQuality::High)) { OutMs = 2.38f; return true; }
		return false;
	case ECyberSettingOption::AntiAliasing:
		if (ValueIndex == static_cast<int32>(ECyberAntiAliasing::TSRNative)) { OutMs = 4.68f; return true; }
		if (ValueIndex == static_cast<int32>(ECyberAntiAliasing::TSRQuality)) { OutMs = -1.82f; return true; }
		if (ValueIndex == static_cast<int32>(ECyberAntiAliasing::TSRBalanced)) { OutMs = -3.15f; return true; }
		if (ValueIndex == static_cast<int32>(ECyberAntiAliasing::TSRPerformance)) { OutMs = -4.23f; return true; }
		return false;
	case ECyberSettingOption::Nanite:
		// Nanite alone, the Nanite lap with Virtual Shadow Maps Low minus the Virtual Shadow Maps Low cost
		if (ValueIndex == 1) { OutMs = 1.02f; return true; }
		return false;
	case ECyberSettingOption::EffectsDensity:
		if (ValueIndex == 3) { OutMs = 0.04f; return true; }
		return false;
	case ECyberSettingOption::ViewDistance:
		if (ValueIndex == 3) { OutMs = 0.10f; return true; }
		return false;
	case ECyberSettingOption::MotionBlur:
		if (ValueIndex == 1) { OutMs = 0.62f; return true; }
		return false;
	default:
		return false;
	}
}

FText FSettingsMeasuredCosts::Describe(ECyberSettingOption Option, int32 ValueIndex)
{
	float Ms = 0.0f;
	bool bAbsolute = false;
	if (!Find(Option, ValueIndex, Ms, bAbsolute))
	{
		return FText::GetEmpty();
	}
	return FText::FromString(bAbsolute ? FString::Printf(TEXT("%.1f ms"), Ms) : FString::Printf(TEXT("%+.2f ms"), Ms));
}

FText FSettingsMeasuredCosts::GetSourceNote()
{
	return LOCTEXT("SourceNote", "Grey figures: measured cost per option against all features off, RTX 3090 at 5120 x 1440, run 4 in docs/benchmark.md.");
}

#undef LOCTEXT_NAMESPACE
