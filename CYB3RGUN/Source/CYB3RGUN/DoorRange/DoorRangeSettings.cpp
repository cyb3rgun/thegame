// CYB3RGUN THEGAME. Tunables for the door range.

#include "DoorRangeSettings.h"

UDoorRangeSettings::UDoorRangeSettings()
{
	// Class defaults follow the Standard preset so an unset settings reference still plays sensibly.
	FDoorWaveSettings Wave1;
	Wave1.ExposureWindow = 2.5f;
	Wave1.TelegraphDuration = 0.5f;
	Wave1.TimeBetweenOpenings = 1.2f;
	Wave1.HostileShare = 0.55f;
	Wave1.EmptyShare = 0.1f;
	Wave1.Openings = 10;
	Wave1.VisibleDoors = 2;

	FDoorWaveSettings Wave2;
	Wave2.ExposureWindow = 2.0f;
	Wave2.TelegraphDuration = 0.5f;
	Wave2.TimeBetweenOpenings = 1.0f;
	Wave2.HostileShare = 0.6f;
	Wave2.EmptyShare = 0.1f;
	Wave2.Openings = 12;
	Wave2.VisibleDoors = 3;

	FDoorWaveSettings Wave3;
	Wave3.ExposureWindow = 1.6f;
	Wave3.TelegraphDuration = 0.4f;
	Wave3.TimeBetweenOpenings = 0.8f;
	Wave3.HostileShare = 0.65f;
	Wave3.EmptyShare = 0.1f;
	Wave3.Openings = 14;
	Wave3.VisibleDoors = 3;

	Waves = { Wave1, Wave2, Wave3 };
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
