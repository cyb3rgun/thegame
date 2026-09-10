// CYB3RGUN THEGAME. Scatters instances of one mesh over an area.

#include "BenchmarkScatter.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Math/RandomStream.h"

ABenchmarkScatter::ABenchmarkScatter()
{
	PrimaryActorTick.bCanEverTick = false;

	Instances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("Instances"));
	Instances->SetMobility(EComponentMobility::Static);
	Instances->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RootComponent = Instances;
}

void ABenchmarkScatter::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Rebuild();
}

void ABenchmarkScatter::BeginPlay()
{
	Super::BeginPlay();

	// a loaded level keeps its instances, but a changed count or seed must still show up
	if (Instances->GetInstanceCount() != Count)
	{
		Rebuild();
	}
}

void ABenchmarkScatter::Rebuild()
{
	Instances->ClearInstances();
	Instances->SetStaticMesh(Mesh);
	Instances->SetCastShadow(bCastShadow);
	if (Material)
	{
		Instances->SetMaterial(0, Material);
	}
	if (!Mesh || Count <= 0)
	{
		return;
	}

	FRandomStream Stream(Seed);
	TArray<FTransform> Transforms;
	Transforms.Reserve(Count);

	int32 Attempts = 0;
	while (Transforms.Num() < Count && Attempts < Count * 4)
	{
		++Attempts;
		const float X = Stream.FRandRange(-HalfExtent.X, HalfExtent.X);
		const float Y = Stream.FRandRange(-HalfExtent.Y, HalfExtent.Y);
		const float Uniform = Stream.FRandRange(ScaleRange.X, ScaleRange.Y);
		const float Yaw = Stream.FRandRange(0.0f, 360.0f);
		if (FMath::Abs(Y) < ClearLaneHalfWidth)
		{
			continue;
		}

		const FVector Scale = AxisScale * Uniform;
		Transforms.Add(FTransform(FRotator(0.0f, Yaw, 0.0f), FVector(X, Y, ZOffset * Scale.Z), Scale));
	}

	Instances->AddInstances(Transforms, false, false);
}
