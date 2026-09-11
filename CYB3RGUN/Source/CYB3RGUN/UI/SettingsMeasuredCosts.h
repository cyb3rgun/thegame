// CYB3RGUN THEGAME. Measured costs of the graphics options, shown next to each option in the settings menu.

#pragma once

#include "CoreMinimal.h"
#include "CyberSettingsTypes.h"

/**
 *  The numbers of run 4 in docs/benchmark.md: an RTX 3090 at 5120 x 1440, every option value measured against a
 *  baseline with every heavy feature off, inside one process. A preset carries an absolute frame time only while
 *  its definition is the one run 4 measured, which since G03-B03 is Low alone. Values run 4 did not measure show
 *  nothing, they are not estimated. Update this table together with docs/benchmark.md after a new measurement.
 */
struct CYB3RGUN_API FSettingsMeasuredCosts
{
	/** True when run 4 measured this value. bOutAbsolute marks a preset frame time rather than a cost. */
	static bool Find(ECyberSettingOption Option, int32 ValueIndex, float& OutMs, bool& bOutAbsolute);

	/** Text for the menu row: "+0.90 ms" for a cost, "5.9 ms" for a preset, empty when unmeasured */
	static FText Describe(ECyberSettingOption Option, int32 ValueIndex);

	/** Where the numbers come from, for the menu footnote */
	static FText GetSourceNote();
};
