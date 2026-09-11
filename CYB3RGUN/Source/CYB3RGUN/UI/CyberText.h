// CYB3RGUN THEGAME. Numbers the player reads, the same on every machine.

#pragma once

#include "CoreMinimal.h"

/**
 *  Formats every number the player reads in the invariant culture and without digit grouping, so a machine set to a German
 *  locale still shows x1.5 and 1250 instead of x1,5 and 1.250 (D-057). FText::AsNumber on its own follows the machine's locale.
 */
struct CYB3RGUN_API FCyberText
{
	/** A whole number, with a leading minus when negative */
	static FText Int(int64 Value);

	/** A whole number that always carries its sign: +20, -300 */
	static FText Signed(int64 Value);

	/** A decimal number with exactly this many fractional digits */
	static FText Fixed(double Value, int32 FractionalDigits);
};
