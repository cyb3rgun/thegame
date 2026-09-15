// CYB3RGUN THEGAME. A fixed piece of the flight range scene that pays or costs points when shot: a windmill, a scarecrow, a sign.

#include "FlightScoringProp.h"
#include "FlightRangeGameMode.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogFlightProp, Log, All);

namespace FlightPropParts
{
	UStaticMeshComponent* MakeMesh(AActor* Owner, USceneComponent* Parent, const TCHAR* Name)
	{
		UStaticMeshComponent* Mesh = Owner->CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Mesh->SetupAttachment(Parent);
		// shots, projectiles and pellets alike, land on it; nothing walks here
		Mesh->SetCollisionProfileName(FName("BlockAll"));
		Mesh->SetGenerateOverlapEvents(false);
		Mesh->SetCanEverAffectNavigation(false);
		Mesh->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
		return Mesh;
	}
}

AFlightScoringProp::AFlightScoringProp()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
	Body = FlightPropParts::MakeMesh(this, Root, TEXT("Body"));
	MovingPivot = CreateDefaultSubobject<USceneComponent>(TEXT("MovingPivot"));
	MovingPivot->SetupAttachment(Root);
	Moving = FlightPropParts::MakeMesh(this, MovingPivot, TEXT("Moving"));
}

void AFlightScoringProp::BeginPlay()
{
	Super::BeginPlay();
	RestRotation = MovingPivot->GetRelativeRotation().Quaternion();
}

void AFlightScoringProp::ResetForRound()
{
	bPaidThisRound = false;
}

bool AFlightScoringProp::NotifyShot(const FHitResult& Hit, const FVector& ShotDirection, AController* InstigatedBy)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	// every pellet of a shot arrives in the same frame: the shot reacts and scores once
	const double Now = World->GetTimeSeconds();
	if (Now == LastScoredAt)
	{
		return false;
	}
	LastScoredAt = Now;

	if (Reaction == EFlightPropReaction::Spin)
	{
		Boost += HitStrength;
	}
	else
	{
		// the swing starts away from the side the shot came from
		const FVector Side = FVector::CrossProduct(MovingPivot->GetComponentQuat().RotateVector(ReactionAxis.GetSafeNormal()), FVector::UpVector);
		SwaySign = FVector::DotProduct(ShotDirection, Side) >= 0.0f ? 1.0f : -1.0f;
		SwayAge = 0.0f;
	}

	AFlightRangeGameMode* Range = Cast<AFlightRangeGameMode>(World->GetAuthGameMode());
	if (!Range || Range->GetRoundState() != EFlightRoundState::Running || Points == 0)
	{
		return false;
	}
	if (Points > 0 && bPaidThisRound)
	{
		UE_LOG(LogFlightProp, Log, TEXT("%s hit again, it paid this round already"), *GetName());
		return false;
	}

	bPaidThisRound = true;
	Range->AddPropPoints(this, Points, Hit.ImpactPoint);
	return true;
}

void AFlightScoringProp::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const FVector Axis = ReactionAxis.GetSafeNormal();
	if (Axis.IsNearlyZero())
	{
		return;
	}

	if (Reaction == EFlightPropReaction::Spin)
	{
		Boost = FMath::Max(Boost - Boost * Damping * DeltaSeconds - 5.0f * DeltaSeconds, 0.0f);
		Angle = FMath::Fmod(Angle + (IdleSpinSpeed + Boost) * DeltaSeconds, 360.0f);
	}
	else
	{
		SwayAge += DeltaSeconds;
		const float Envelope = FMath::Exp(-Damping * SwayAge);
		Angle = Envelope > 0.01f ? SwaySign * HitStrength * Envelope * FMath::Sin(UE_TWO_PI * SwayFrequency * SwayAge) : 0.0f;
	}
	MovingPivot->SetRelativeRotation(RestRotation * FQuat(Axis, FMath::DegreesToRadians(Angle)));
}
