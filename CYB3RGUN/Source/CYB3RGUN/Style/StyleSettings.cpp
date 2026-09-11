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
