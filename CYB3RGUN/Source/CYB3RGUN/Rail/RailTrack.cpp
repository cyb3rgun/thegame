// CYB3RGUN THEGAME. A rail segment: a spline route plus the beats that happen along it.

#include "RailTrack.h"
#include "Components/SplineComponent.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogRailTrack, Log, All);

ARailTrack::ARailTrack()
{
	PrimaryActorTick.bCanEverTick = false;

	Spline = CreateDefaultSubobject<USplineComponent>(TEXT("Spline"));
	Spline->SetClosedLoop(false);
	RootComponent = Spline;
}

void ARailTrack::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RebuildFromRoutePoints();
}

void ARailTrack::RebuildFromRoutePoints()
{
	if (RoutePoints.Num() >= 2)
	{
		Spline->SetSplinePoints(RoutePoints, ESplineCoordinateSpace::Local, true);
	}
}

void ARailTrack::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// sorted before any BeginPlay, so a rider never sees the editor order
	if (GetWorld() && GetWorld()->IsGameWorld())
	{
		Beats.StableSort([](const FRailBeat& A, const FRailBeat& B)
		{
			return A.TriggerDistance < B.TriggerDistance;
		});
	}
}

void ARailTrack::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogRailTrack, Log, TEXT("%s: length %.0f cm, %d beats, %d next segments"), *GetName(), GetLength(), Beats.Num(), NextTracks.Num());
	for (int32 i = 0; i < Beats.Num(); ++i)
	{
		if (Beats[i].TriggerDistance > GetLength())
		{
			UE_LOG(LogRailTrack, Warning, TEXT("%s: beat %d %s at %.0f cm lies beyond the end of the segment and never fires"), *GetName(), i, *Beats[i].Name.ToString(), Beats[i].TriggerDistance);
		}
	}
}

float ARailTrack::GetLength() const
{
	return Spline ? Spline->GetSplineLength() : 0.0f;
}

FTransform ARailTrack::GetWorldTransformAtDistance(float Distance) const
{
	if (!Spline)
	{
		return GetActorTransform();
	}
	return Spline->GetTransformAtDistanceAlongSpline(FMath::Clamp(Distance, 0.0f, GetLength()), ESplineCoordinateSpace::World);
}

ARailTrack* ARailTrack::SelectNextTrack_Implementation(ARailPawn* Rider) const
{
	for (const TObjectPtr<ARailTrack>& Next : NextTracks)
	{
		if (Next && Next != this)
		{
			return Next;
		}
	}
	return nullptr;
}
