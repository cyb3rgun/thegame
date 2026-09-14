// CYB3RGUN THEGAME. A place behind cover where flight targets rise from.

#include "FlightLaunchPoint.h"
#include "Components/ArrowComponent.h"

AFlightLaunchPoint::AFlightLaunchPoint()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

#if WITH_EDITORONLY_DATA
	Arrow = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("Arrow"));
	if (Arrow)
	{
		Arrow->SetupAttachment(Root);
		Arrow->SetRelativeRotation(FRotator(60.0f, 0.0f, 0.0f));
		Arrow->ArrowColor = FColor(0, 220, 255);
	}
#endif
}
