// CYB3RGUN THEGAME. Every value of the style system in one data asset.

#include "StyleSettings.h"
#include "StyleProjectSettings.h"
#include "StyleScenario.h"
#include "Engine/World.h"
#include "GameFramework/GameModeBase.h"

#define LOCTEXT_NAMESPACE "StyleSettings"

UStyleSettings::UStyleSettings()
{
	auto MakeRank = [](float Threshold, const FText& Label)
	{
		FStyleRank Rank;
		Rank.Threshold = Threshold;
		Rank.Label = Label;
		return Rank;
	};

	// explicit bands, so a new asset starts complete (D-015)
	Ranks.Add(MakeRank(0.0f, LOCTEXT("RankCold", "COLD")));
	Ranks.Add(MakeRank(20.0f, LOCTEXT("RankSteady", "STEADY")));
	Ranks.Add(MakeRank(45.0f, LOCTEXT("RankSharp", "SHARP")));
	Ranks.Add(MakeRank(70.0f, LOCTEXT("RankClean", "CLEAN")));
	Ranks.Add(MakeRank(90.0f, LOCTEXT("RankFlawless", "FLAWLESS")));

	auto MakeShake = [](float Degrees, float DecaySeconds, float Frequency)
	{
		FStyleShake Shake;
		Shake.Degrees = Degrees;
		Shake.DecaySeconds = DecaySeconds;
		Shake.Frequency = Frequency;
		return Shake;
	};

	// three strengths of the camera kick, small turns that are gone within a fifth of a second
	LightShake = MakeShake(0.25f, 0.08f, 18.0f);
	MediumShake = MakeShake(0.6f, 0.12f, 16.0f);
	HeavyShake = MakeShake(1.1f, 0.16f, 14.0f);
}

const UStyleSettings* UStyleSettings::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (const IStyleScenario* Scenario = World ? Cast<IStyleScenario>(World->GetAuthGameMode()) : nullptr)
	{
		if (const UStyleSettings* Own = Scenario->GetStyleSettings())
		{
			return Own;
		}
	}

	if (const UStyleSettings* ProjectDefault = GetDefault<UStyleProjectSettings>()->DefaultStyleSettings.LoadSynchronous())
	{
		return ProjectDefault;
	}
	return GetDefault<UStyleSettings>();
}

FText UStyleSettings::GetRankLabel(float MeterValue) const
{
	FText Label;
	float Best = -1.0f;
	for (const FStyleRank& Rank : Ranks)
	{
		if (MeterValue >= Rank.Threshold && Rank.Threshold > Best)
		{
			Best = Rank.Threshold;
			Label = Rank.Label;
		}
	}
	return Label;
}

#undef LOCTEXT_NAMESPACE
