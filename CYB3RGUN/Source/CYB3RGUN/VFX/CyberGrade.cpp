// CYB3RGUN THEGAME. One grade for the night levels and the menu: cyan shadows, magenta highlights, bloom for neon.

#include "CyberGrade.h"
#include "Components/PostProcessComponent.h"
#include "Components/SceneComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogCyberGrade, Log, All);

ACyberGrade::ACyberGrade()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Grade = CreateDefaultSubobject<UPostProcessComponent>(TEXT("Grade"));
	Grade->SetupAttachment(RootComponent);
	Grade->bUnbound = true;
}

void ACyberGrade::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyGrade();
}

void ACyberGrade::BeginPlay()
{
	Super::BeginPlay();
	ApplyGrade();
	UE_LOG(LogCyberGrade, Log, TEXT("%s grades the level with %s at priority %.0f"), *GetName(), *GetNameSafe(GradeSettings), Priority);
}

void ACyberGrade::ApplyGrade()
{
	Grade->bUnbound = true;
	Grade->Priority = Priority;
	Grade->BlendWeight = 1.0f;
	Grade->Settings = GradeSettings ? GradeSettings->Settings : FPostProcessSettings();
}
