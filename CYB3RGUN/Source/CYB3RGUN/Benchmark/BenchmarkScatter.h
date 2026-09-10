// CYB3RGUN THEGAME. Scatters instances of one mesh over an area, for benchmark vegetation and clutter.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BenchmarkScatter.generated.h"

class UHierarchicalInstancedStaticMeshComponent;
class UMaterialInterface;
class UStaticMesh;

/**
 *  Places Count instances inside a box from a fixed seed, so every run and every machine sees the same scene.
 *  Two scatters with the same seed, count and area produce the same positions, which is how a canopy sits on a trunk.
 */
UCLASS()
class CYB3RGUN_API ABenchmarkScatter : public AActor
{
	GENERATED_BODY()

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Scatter")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Instances;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Scatter")
	TObjectPtr<UStaticMesh> Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Scatter")
	TObjectPtr<UMaterialInterface> Material;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Scatter", meta = (ClampMin = 0, ClampMax = 200000))
	int32 Count = 1000;

	/** Half size of the area in actor space */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Scatter")
	FVector2D HalfExtent = FVector2D(2000.0, 2000.0);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Scatter")
	int32 Seed = 1;

	/** Uniform scale range */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Scatter")
	FVector2D ScaleRange = FVector2D(0.8, 1.2);

	/** Per axis multiplier applied on top of the uniform scale */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Scatter")
	FVector AxisScale = FVector::OneVector;

	/** Height offset, multiplied by the instance scale so it follows a scaled trunk */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Scatter")
	float ZOffset = 0.0f;

	/** Keeps a lane along the actor's X axis clear, half width in centimetres */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Scatter", meta = (ClampMin = 0.0))
	float ClearLaneHalfWidth = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Scatter")
	bool bCastShadow = true;

public:

	ABenchmarkScatter();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category="Scatter")
	void Rebuild();
};
