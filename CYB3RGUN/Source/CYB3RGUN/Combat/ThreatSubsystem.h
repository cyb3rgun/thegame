// CYB3RGUN THEGAME. How close the nearest hostile is to shooting, for the logo reticle.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ThreatSubsystem.generated.h"

/** One source's last report */
struct FThreatReport
{
	float Level = 0.0f;
	double WallTime = 0.0;
};

/**
 *  Collects threat reports from whatever is about to shoot at the player: a door hostile drawing its pistol, an enemy
 *  winding up a strike. A source reports a level between 0 and 1 every frame while it threatens, and the logo reticle reads
 *  the strongest (D-048). Reports run on the wall clock, so hit stop and Overclock never make a live threat look stale.
 */
UCLASS()
class CYB3RGUN_API UThreatSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:

	static UThreatSubsystem* Get(const UObject* WorldContext);

	/** 0 while calm, 1 when the source can shoot now. Renew it every frame, a report older than a fifth of a second is gone. */
	void ReportThreat(const UObject* Source, float Level);

	/** The strongest live report */
	float GetThreatLevel() const;

private:

	TMap<TWeakObjectPtr<const UObject>, FThreatReport> Reports;

	double LastPruneAt = 0.0;
};
