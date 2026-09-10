// CYB3RGUN THEGAME. Data asset describing an encounter.

#include "EncounterDefinition.h"

int32 FEncounterWave::GetEnemyCount() const
{
	int32 Total = 0;
	for (const FEnemySpawnEntry& Entry : Composition)
	{
		if (Entry.Definition)
		{
			Total += FMath::Max(Entry.Count, 0);
		}
	}
	return Total;
}

int32 UEncounterDefinition::GetTotalEnemyCount() const
{
	int32 Total = 0;
	for (const FEncounterWave& Wave : Waves)
	{
		Total += Wave.GetEnemyCount();
	}
	return Total;
}
