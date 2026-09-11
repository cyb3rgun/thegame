// CYB3RGUN THEGAME. How close the nearest hostile is to shooting, for the logo reticle.

#include "ThreatSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "HAL/PlatformTime.h"

namespace ThreatReports
{
	/** Longer than a slow frame, shorter than a ring that lingers would be noticed */
	constexpr double StaleSeconds = 0.2;
}

UThreatSubsystem* UThreatSubsystem::Get(const UObject* WorldContext)
{
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull) : nullptr;
	return World ? World->GetSubsystem<UThreatSubsystem>() : nullptr;
}

void UThreatSubsystem::ReportThreat(const UObject* Source, float Level)
{
	if (!Source)
	{
		return;
	}

	const double Now = FPlatformTime::Seconds();
	FThreatReport& Report = Reports.FindOrAdd(Source);
	Report.Level = FMath::Clamp(Level, 0.0f, 1.0f);
	Report.WallTime = Now;

	// sources that stopped reporting drop out, the map stays as small as the doors and enemies in play
	if (Now - LastPruneAt > 1.0)
	{
		LastPruneAt = Now;
		for (auto It = Reports.CreateIterator(); It; ++It)
		{
			if (!It.Key().IsValid() || Now - It.Value().WallTime > ThreatReports::StaleSeconds)
			{
				It.RemoveCurrent();
			}
		}
	}
}

float UThreatSubsystem::GetThreatLevel() const
{
	const double Now = FPlatformTime::Seconds();
	float Strongest = 0.0f;
	for (const TPair<TWeakObjectPtr<const UObject>, FThreatReport>& Pair : Reports)
	{
		if (Pair.Key.IsValid() && Now - Pair.Value.WallTime <= ThreatReports::StaleSeconds)
		{
			Strongest = FMath::Max(Strongest, Pair.Value.Level);
		}
	}
	return Strongest;
}
