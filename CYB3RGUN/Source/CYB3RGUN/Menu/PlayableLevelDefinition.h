// CYB3RGUN THEGAME. One playable entry of the level selection (D-037).

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PlayableLevelDefinition.generated.h"

class UTexture2D;

UENUM(BlueprintType)
enum class EPlayableLevelKind : uint8
{
	Scenario,
	TestMap UMETA(DisplayName = "Test map")
};

/**
 *  A level the player can start from the menu. The level selection lists every definition the asset manager
 *  finds (primary asset type PlayableLevel, scanned in Core/Levels), so adding a level means adding an asset,
 *  not editing code or map names.
 */
UCLASS(BlueprintType)
class CYB3RGUN_API UPlayableLevelDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Level")
	FText DisplayName;

	/** One or two lines under the name on the level card */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Level", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Level")
	TSoftObjectPtr<UWorld> Map;

	/** 16:9 picture on the level card */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Level")
	TSoftObjectPtr<UTexture2D> Preview;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Level")
	EPlayableLevelKind Kind = EPlayableLevelKind::Scenario;

	/** Lower comes first within its kind */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Level")
	int32 SortOrder = 0;

	static const FPrimaryAssetType AssetType;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	/** The display name, or the asset name while the display name is empty */
	FText GetDisplayText() const;

	/** Every definition the asset manager knows, scenarios first, then by sort order and name */
	static TArray<UPlayableLevelDefinition*> LoadAll();
};
