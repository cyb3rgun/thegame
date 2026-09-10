// CYB3RGUN THEGAME. A grid of animated skeletal meshes, the benchmark's Nanite skeletal mesh test.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BenchmarkCrowd.generated.h"

class UAnimationAsset;
class USkeletalMesh;
class USkeletalMeshComponent;

/** Spawns Rows by Columns skeletal meshes playing one looping animation, each offset in time so they do not move in sync */
UCLASS()
class CYB3RGUN_API ABenchmarkCrowd : public AActor
{
	GENERATED_BODY()

protected:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Crowd")
	TObjectPtr<USkeletalMesh> Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Crowd")
	TObjectPtr<UAnimationAsset> Animation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Crowd", meta = (ClampMin = 1, ClampMax = 50))
	int32 Rows = 4;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Crowd", meta = (ClampMin = 1, ClampMax = 50))
	int32 Columns = 5;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Crowd", meta = (ClampMin = 50.0))
	float Spacing = 250.0f;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USkeletalMeshComponent>> Members;

public:

	ABenchmarkCrowd();

	virtual void BeginPlay() override;

protected:

	void Spawn();
};
