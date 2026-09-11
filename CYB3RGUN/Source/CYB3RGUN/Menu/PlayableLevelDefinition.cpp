// CYB3RGUN THEGAME. One playable entry of the level selection.

#include "PlayableLevelDefinition.h"
#include "Engine/AssetManager.h"
#include "Engine/Texture2D.h"

const FPrimaryAssetType UPlayableLevelDefinition::AssetType(TEXT("PlayableLevel"));

FPrimaryAssetId UPlayableLevelDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(AssetType, GetFName());
}

FText UPlayableLevelDefinition::GetDisplayText() const
{
	return DisplayName.IsEmpty() ? FText::FromName(GetFName()) : DisplayName;
}

TArray<UPlayableLevelDefinition*> UPlayableLevelDefinition::LoadAll()
{
	TArray<UPlayableLevelDefinition*> Levels;
	UAssetManager* Manager = UAssetManager::GetIfInitialized();
	if (!Manager)
	{
		return Levels;
	}

	TArray<FPrimaryAssetId> Ids;
	Manager->GetPrimaryAssetIdList(AssetType, Ids);
	for (const FPrimaryAssetId& Id : Ids)
	{
		// a handful of small assets, loading them on the spot is fine
		if (UPlayableLevelDefinition* Level = Cast<UPlayableLevelDefinition>(Manager->GetPrimaryAssetPath(Id).TryLoad()))
		{
			Levels.Add(Level);
		}
	}

	Levels.Sort([](const UPlayableLevelDefinition& A, const UPlayableLevelDefinition& B)
	{
		if (A.Kind != B.Kind)
		{
			return A.Kind < B.Kind;
		}
		if (A.SortOrder != B.SortOrder)
		{
			return A.SortOrder < B.SortOrder;
		}
		return A.GetDisplayText().CompareTo(B.GetDisplayText()) < 0;
	});
	return Levels;
}
