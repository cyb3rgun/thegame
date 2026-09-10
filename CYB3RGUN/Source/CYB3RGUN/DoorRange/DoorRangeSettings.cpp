// CYB3RGUN THEGAME. Tunables for the door range.

#include "DoorRangeSettings.h"

UDoorRangeSettings::UDoorRangeSettings()
{
	// Class defaults carry no wave list on purpose. Every preset stores its own explicit values,
	// so a change to these defaults can never silently move a preset. GetWave falls back to one generic wave.
}

const FDoorWaveSettings& UDoorRangeSettings::GetWave(int32 WaveNumber) const
{
	static const FDoorWaveSettings Fallback;

	if (Waves.IsEmpty())
	{
		return Fallback;
	}

	const int32 Index = FMath::Clamp(WaveNumber - 1, 0, Waves.Num() - 1);
	return Waves[Index];
}
