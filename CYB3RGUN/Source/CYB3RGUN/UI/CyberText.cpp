// CYB3RGUN THEGAME. Numbers the player reads, the same on every machine.

#include "CyberText.h"
#include "Internationalization/Culture.h"
#include "Internationalization/Internationalization.h"

namespace CyberTextFormat
{
	FNumberFormattingOptions Options(int32 FractionalDigits, bool bAlwaysSign)
	{
		FNumberFormattingOptions Result;
		Result.SetUseGrouping(false);
		Result.SetAlwaysSign(bAlwaysSign);
		Result.SetMinimumFractionalDigits(FractionalDigits);
		Result.SetMaximumFractionalDigits(FractionalDigits);
		return Result;
	}
}

FText FCyberText::Int(int64 Value)
{
	const FNumberFormattingOptions Format = CyberTextFormat::Options(0, false);
	return FText::AsNumber(Value, &Format, FInternationalization::Get().GetInvariantCulture());
}

FText FCyberText::Signed(int64 Value)
{
	const FNumberFormattingOptions Format = CyberTextFormat::Options(0, true);
	return FText::AsNumber(Value, &Format, FInternationalization::Get().GetInvariantCulture());
}

FText FCyberText::Fixed(double Value, int32 FractionalDigits)
{
	const FNumberFormattingOptions Format = CyberTextFormat::Options(FMath::Max(FractionalDigits, 0), false);
	return FText::AsNumber(Value, &Format, FInternationalization::Get().GetInvariantCulture());
}
