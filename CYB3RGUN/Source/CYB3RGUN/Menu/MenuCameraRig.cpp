// CYB3RGUN THEGAME. The slow camera move behind the main menu.

#include "MenuCameraRig.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"

AMenuCameraRig::AMenuCameraRig()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bTickEvenWhenPaused = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(Root);
	Camera->SetFieldOfView(FieldOfView);

	// where the move starts, so the level shows the opening view in the editor
	UpdateCamera();
}

void AMenuCameraRig::BeginPlay()
{
	Super::BeginPlay();

	Camera->SetFieldOfView(FieldOfView);
	Elapsed = 0.0f;
	Angle = 0.0f;
	UpdateCamera();
}

void AMenuCameraRig::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	Elapsed += DeltaSeconds;
	Angle = FMath::Fmod(Angle + OrbitSpeed * DeltaSeconds, 360.0f);
	UpdateCamera();
}

void AMenuCameraRig::UpdateCamera()
{
	const float Radians = FMath::DegreesToRadians(Angle);
	const float Bob = BobPeriod > 0.0f ? BobHeight * FMath::Sin(UE_TWO_PI * Elapsed / BobPeriod) : 0.0f;
	const FVector Location(FMath::Cos(Radians) * OrbitRadius, FMath::Sin(Radians) * OrbitRadius, Height + Bob);
	Camera->SetRelativeLocationAndRotation(Location, FRotator(Pitch, Angle + LookAhead, 0.0f));
}
