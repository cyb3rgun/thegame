// CYB3RGUN THEGAME. A point the encounter director spawns enemies at.

#include "EnemySpawnPoint.h"
#include "Components/ArrowComponent.h"
#include "Components/SceneComponent.h"

AEnemySpawnPoint::AEnemySpawnPoint()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

#if WITH_EDITORONLY_DATA
	Arrow = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("Arrow"));
	if (Arrow)
	{
		Arrow->SetupAttachment(Root);
		Arrow->ArrowColor = FColor(255, 60, 60);
		Arrow->ArrowSize = 2.0f;
	}
#endif
}

FTransform AEnemySpawnPoint::GetSpawnTransform() const
{
	const FVector2D Scatter = FMath::RandPointInCircle(ScatterRadius);
	const FVector Location = GetActorLocation() + FVector(Scatter.X, Scatter.Y, 0.0f);
	return FTransform(GetActorRotation(), Location, FVector::OneVector);
}
