// CYB3RGUN THEGAME. A point the encounter director spawns enemies at.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnemySpawnPoint.generated.h"

class UArrowComponent;
class USceneComponent;

/** Marker actor. Place a few around the arena; the director picks among them at random. */
UCLASS()
class CYB3RGUN_API AEnemySpawnPoint : public AActor
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USceneComponent* Root;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<UArrowComponent> Arrow;
#endif

public:

	/** Spawns scatter within this radius so enemies do not stack */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spawn", meta = (ClampMin = 0.0, Units = "cm"))
	float ScatterRadius = 150.0f;

	AEnemySpawnPoint();

	/** A spawn transform near this point */
	UFUNCTION(BlueprintPure, Category="Spawn")
	FTransform GetSpawnTransform() const;
};
