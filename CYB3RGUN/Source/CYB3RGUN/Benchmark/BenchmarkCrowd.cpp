// CYB3RGUN THEGAME. A grid of animated skeletal meshes.

#include "BenchmarkCrowd.h"
#include "Animation/AnimationAsset.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"

ABenchmarkCrowd::ABenchmarkCrowd()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void ABenchmarkCrowd::BeginPlay()
{
	Super::BeginPlay();
	Spawn();
}

void ABenchmarkCrowd::Spawn()
{
	if (!Mesh)
	{
		return;
	}

	const float HalfWidth = 0.5f * (Columns - 1) * Spacing;
	const float HalfDepth = 0.5f * (Rows - 1) * Spacing;
	for (int32 Row = 0; Row < Rows; ++Row)
	{
		for (int32 Column = 0; Column < Columns; ++Column)
		{
			USkeletalMeshComponent* Member = NewObject<USkeletalMeshComponent>(this, NAME_None, RF_Transient);
			Member->SetSkeletalMesh(Mesh);
			Member->SetupAttachment(RootComponent);
			Member->SetRelativeLocation(FVector(Row * Spacing - HalfDepth, Column * Spacing - HalfWidth, 0.0f));
			Member->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
			Member->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Member->RegisterComponent();
			if (Animation)
			{
				Member->PlayAnimation(Animation, true);
				Member->SetPosition(0.37f * (Row * Columns + Column), false);
			}
			Members.Add(Member);
		}
	}
}
