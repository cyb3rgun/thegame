// CYB3RGUN THEGAME. One grade for the night levels and the menu: cyan shadows, magenta highlights, bloom for neon.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/Scene.h"
#include "GameFramework/Actor.h"
#include "CyberGrade.generated.h"

class UPostProcessComponent;

/**
 *  The shared cyberpunk grade: cyan and magenta split toning, bloom tuned for neon, a slight chromatic aberration, fine
 *  grain and controlled contrast. Only the fields switched on here override a level's own post process, so exposure and
 *  anything lighting specific stay with the level. One asset serves every level that places an ACyberGrade.
 */
UCLASS(BlueprintType)
class CYB3RGUN_API UCyberGradeSettings : public UDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grade")
	FPostProcessSettings Settings;
};

/**
 *  Places the shared grade in a level: an unbound post process above the level's own volume, filled from a
 *  UCyberGradeSettings asset. The night door range, the rail level and the menu place one; the precision scenario places
 *  none and keeps its neutral look.
 */
UCLASS()
class CYB3RGUN_API ACyberGrade : public AActor
{
	GENERATED_BODY()

public:

	ACyberGrade();

	virtual void OnConstruction(const FTransform& Transform) override;

protected:

	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category="Grade")
	TObjectPtr<UPostProcessComponent> Grade;

	/** The grade this level shows */
	UPROPERTY(EditAnywhere, Category="Grade")
	TObjectPtr<UCyberGradeSettings> GradeSettings;

	/** Above the level's own post process volume, so the shared grade wins wherever it speaks */
	UPROPERTY(EditAnywhere, Category="Grade")
	float Priority = 10.0f;

	/** Copies the asset into the post process, in the editor viewport as in play */
	void ApplyGrade();
};
