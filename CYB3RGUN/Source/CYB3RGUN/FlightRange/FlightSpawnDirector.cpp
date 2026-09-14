// CYB3RGUN THEGAME. Keeps the flight range sky busy: launches targets from the sides and from behind cover.

#include "FlightSpawnDirector.h"
#include "FlightLaunchPoint.h"
#include "FlightPath.h"
#include "FlightRangeSettings.h"
#include "FlightTarget.h"
#include "FlightTargetDefinition.h"
#include "Engine/World.h"
#include "EngineUtils.h"

DEFINE_LOG_CATEGORY_STATIC(LogFlightDirector, Log, All);

namespace FlightDirectorTuning
{
	/** Launches remembered for weighting the sources against their recent use */
	constexpr int32 RecentLaunches = 6;
}

AFlightSpawnDirector::AFlightSpawnDirector()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AFlightSpawnDirector::StartLaunching(const UFlightRangeSettings* InSettings, const FTransform& InField, const FVector& InShooterLocation)
{
	Settings = InSettings;
	Field = InField;
	ShooterLocation = InShooterLocation;
	RecentSources.Reset();
	LastLaunchPoint = INDEX_NONE;
	for (int32& Count : LaunchCounts)
	{
		Count = 0;
	}

	LaunchPoints.Reset();
	for (TActorIterator<AFlightLaunchPoint> It(GetWorld()); It; ++It)
	{
		LaunchPoints.Add(*It);
	}
	LaunchPoints.Sort([](const AFlightLaunchPoint& A, const AFlightLaunchPoint& B) { return A.GetName() < B.GetName(); });

	TimeToNextLaunch = 0.0f;
	bLaunching = Settings != nullptr;
	UE_LOG(LogFlightDirector, Log, TEXT("Launching with %d launch points, %d targets in flight, field at %s facing yaw %.0f"),
		LaunchPoints.Num(), Settings ? Settings->TargetsInFlight : 0, *Field.GetLocation().ToCompactString(), Field.Rotator().Yaw);
}

void AFlightSpawnDirector::StopLaunching(bool bRetireTargets)
{
	bLaunching = false;
	if (!bRetireTargets)
	{
		return;
	}

	TArray<AFlightTarget*> Current;
	GetTargets(Current);
	for (AFlightTarget* Target : Current)
	{
		Target->Retire();
	}
}

int32 AFlightSpawnDirector::CountInFlight() const
{
	int32 Count = 0;
	for (const TWeakObjectPtr<AFlightTarget>& Target : Targets)
	{
		if (Target.IsValid() && !Target->IsDown())
		{
			++Count;
		}
	}
	return Count;
}

void AFlightSpawnDirector::GetTargets(TArray<AFlightTarget*>& OutTargets) const
{
	for (const TWeakObjectPtr<AFlightTarget>& Target : Targets)
	{
		if (Target.IsValid())
		{
			OutTargets.Add(Target.Get());
		}
	}
}

void AFlightSpawnDirector::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	Targets.RemoveAll([](const TWeakObjectPtr<AFlightTarget>& Target) { return !Target.IsValid(); });
	if (!bLaunching || !Settings)
	{
		return;
	}

	TimeToNextLaunch -= DeltaSeconds;
	if (TimeToNextLaunch > 0.0f || CountInFlight() >= Settings->TargetsInFlight)
	{
		return;
	}

	Launch();
	TimeToNextLaunch = FMath::FRandRange(Settings->LaunchIntervalMin, FMath::Max(Settings->LaunchIntervalMin, Settings->LaunchIntervalMax));
}

EFlightSource AFlightSpawnDirector::PickSource() const
{
	const float Cover = LaunchPoints.IsEmpty() ? 0.0f : Settings->CoverShare;
	float Weights[3] = { (1.0f - Cover) * 0.5f, (1.0f - Cover) * 0.5f, Cover };

	for (int32 Source = 0; Source < 3; ++Source)
	{
		// the more a source launched lately, the less likely it launches again
		int32 Recent = 0;
		for (const EFlightSource Used : RecentSources)
		{
			Recent += static_cast<int32>(Used) == Source ? 1 : 0;
		}
		Weights[Source] /= 1.0f + Recent;

		// and it never exceeds the allowed run
		const int32 Run = FMath::Max(Settings->MaxSameSourceInRow, 1);
		if (RecentSources.Num() >= Run)
		{
			bool bSameRun = true;
			for (int32 Index = RecentSources.Num() - Run; Index < RecentSources.Num(); ++Index)
			{
				bSameRun &= static_cast<int32>(RecentSources[Index]) == Source;
			}
			if (bSameRun)
			{
				Weights[Source] = 0.0f;
			}
		}
	}

	const float Total = Weights[0] + Weights[1] + Weights[2];
	if (Total <= 0.0f)
	{
		return FMath::RandBool() ? EFlightSource::Left : EFlightSource::Right;
	}
	float Roll = FMath::FRandRange(0.0f, Total);
	for (int32 Source = 0; Source < 3; ++Source)
	{
		Roll -= Weights[Source];
		if (Roll <= 0.0f && Weights[Source] > 0.0f)
		{
			return static_cast<EFlightSource>(Source);
		}
	}
	return Weights[2] > 0.0f ? EFlightSource::Cover : EFlightSource::Right;
}

bool AFlightSpawnDirector::PlanSideFlight(float SideSign, float MaxCrossing, FVector& OutStart, FVector& OutHeading, float& OutLength, float& OutCrossing) const
{
	const FVector Forward = Field.GetRotation().GetForwardVector().GetSafeNormal2D();
	const FVector Right = FVector::CrossProduct(FVector::UpVector, Forward);

	// the path's closest point to the player lies at the crossing distance, whatever the heading jitter; from there it runs out
	// to the entry bearing on either side
	const float Nearest = FMath::Min(Settings->CrossingDistanceMin, MaxCrossing);
	OutCrossing = FMath::FRandRange(Nearest, FMath::Max(Nearest, FMath::Min(Settings->CrossingDistanceMax, MaxCrossing)));
	const float HalfLength = OutCrossing * FMath::Tan(FMath::DegreesToRadians(Settings->EntryBearingDegrees));
	const float Jitter = FMath::FRandRange(-Settings->HeadingJitterDegrees, Settings->HeadingJitterDegrees);

	OutHeading = (-SideSign * Right).RotateAngleAxis(Jitter, FVector::UpVector);
	const FVector Closest = Field.GetLocation() + Forward.RotateAngleAxis(Jitter, FVector::UpVector) * OutCrossing;
	OutStart = Closest - OutHeading * HalfLength;
	OutLength = 2.0f * HalfLength;
	return true;
}

bool AFlightSpawnDirector::PlanCoverFlight(FVector& OutStart, FVector& OutHeading, float& OutLength)
{
	if (LaunchPoints.IsEmpty())
	{
		return false;
	}

	// another point than last time, when there is one
	int32 Index = FMath::RandRange(0, LaunchPoints.Num() - 1);
	if (LaunchPoints.Num() > 1 && Index == LastLaunchPoint)
	{
		Index = (Index + 1 + FMath::RandRange(0, LaunchPoints.Num() - 2)) % LaunchPoints.Num();
	}
	LastLaunchPoint = Index;
	const AFlightLaunchPoint* Point = LaunchPoints[Index];
	if (!Point)
	{
		return false;
	}

	const FVector Forward = Field.GetRotation().GetForwardVector().GetSafeNormal2D();
	const FVector Right = FVector::CrossProduct(FVector::UpVector, Forward);
	const float SideOfField = FVector::DotProduct(Point->GetActorLocation() - Field.GetLocation(), Right);
	const float Across = FMath::Abs(SideOfField) > 200.0f ? -FMath::Sign(SideOfField) : (FMath::RandBool() ? 1.0f : -1.0f);
	const float Jitter = FMath::FRandRange(-Settings->HeadingJitterDegrees, Settings->HeadingJitterDegrees);

	// a flushed target breaks towards the middle of the field and a little away from the player
	OutStart = Point->GetActorLocation();
	OutHeading = (Right * Across + Forward * 0.35f).GetSafeNormal().RotateAngleAxis(Jitter, FVector::UpVector);
	OutLength = Settings->CoverFlightLength;
	return true;
}

void AFlightSpawnDirector::Launch()
{
	UFlightTargetDefinition* Definition = Settings->PickTarget();
	if (!Definition)
	{
		UE_LOG(LogFlightDirector, Warning, TEXT("No flight target definitions in %s, nothing to launch"), *GetNameSafe(Settings));
		bLaunching = false;
		return;
	}

	EFlightSource Source = PickSource();
	FVector Start;
	FVector Heading;
	float Length = 0.0f;
	float Crossing = 0.0f;
	const FFlightPathSettings* Path = nullptr;

	if (Source == EFlightSource::Cover && PlanCoverFlight(Start, Heading, Length))
	{
		// cover launches rise; a definition without a rising path takes any of its paths from there
		Path = Definition->PickPath({ EFlightPathType::Rising });
		if (!Path)
		{
			Path = Definition->PickPath({});
		}
	}
	else
	{
		if (Source == EFlightSource::Cover)
		{
			Source = FMath::RandBool() ? EFlightSource::Left : EFlightSource::Right;
		}
		// rising belongs to cover, a side entry crosses, dives or flutters; a definition with only rising paths still flies
		Path = Definition->PickPath({ EFlightPathType::Straight, EFlightPathType::Diving, EFlightPathType::Flutter });
		if (!Path)
		{
			Path = Definition->PickPath({});
		}
		if (Path)
		{
			// every target comes within reach where it passes closest: its height there and its wobble go into the distance.
			// A dive passes at its pull out height; a level flight too high for the nearest crossing is brought down
			const float Reach = Settings->ReachDistance;
			const float Wobble = Path->WobbleAmplitude;
			float Height = FMath::FRandRange(Path->HeightMin, FMath::Max(Path->HeightMin, Path->HeightMax));
			const float EyeHeight = ShooterLocation.Z - Field.GetLocation().Z;
			const float HighestAbove = FMath::Sqrt(FMath::Max(FMath::Square(Reach) - FMath::Square(Settings->CrossingDistanceMin), 0.0f)) - Wobble;
			if (Path->Type != EFlightPathType::Diving)
			{
				Height = FMath::Min(Height, EyeHeight + FMath::Max(HighestAbove, 0.0f));
			}
			const float PassHeight = Path->Type == EFlightPathType::Diving ? Path->LevelOutHeight : Height;
			const float Vertical = FMath::Abs(PassHeight - EyeHeight) + Wobble;
			const float MaxCrossing = FMath::Sqrt(FMath::Max(FMath::Square(Reach) - FMath::Square(Vertical), 0.0f));

			PlanSideFlight(Source == EFlightSource::Left ? -1.0f : 1.0f, MaxCrossing, Start, Heading, Length, Crossing);
			Start.Z = Field.GetLocation().Z + Height;
		}
	}

	if (!Path)
	{
		UE_LOG(LogFlightDirector, Warning, TEXT("%s lists no flight paths, nothing to launch"), *GetNameSafe(Definition));
		return;
	}

	const float Speed = FMath::FRandRange(Definition->SpeedMin, FMath::Max(Definition->SpeedMin, Definition->SpeedMax)) * Path->SpeedScale;
	const float Size = FMath::FRandRange(Definition->SizeMin, FMath::Max(Definition->SizeMin, Definition->SizeMax));
	FFlightPathMotion Motion;
	Motion.Start(*Path, Start, Heading, Speed, Field.GetLocation().Z, Length);

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AFlightTarget* Target = GetWorld()->SpawnActor<AFlightTarget>(AFlightTarget::StaticClass(), FTransform(Start), Params);
	if (!Target)
	{
		return;
	}
	Target->Launch(Definition, Motion, Size, ShooterLocation, Settings->LeadShotSpeed, Field.GetLocation().Z);
	Targets.Add(Target);

	RecentSources.Add(Source);
	if (RecentSources.Num() > FlightDirectorTuning::RecentLaunches)
	{
		RecentSources.RemoveAt(0);
	}
	++LaunchCounts[static_cast<int32>(Source)];

	UE_LOG(LogFlightDirector, Log, TEXT("Launch %s from %s: path %s, speed %.0f cm/s, size %.2f, crossing %.0f cm, length %.0f cm"),
		*GetNameSafe(Definition), *StaticEnum<EFlightSource>()->GetNameStringByValue(static_cast<int64>(Source)),
		*StaticEnum<EFlightPathType>()->GetNameStringByValue(static_cast<int64>(Path->Type)), Speed, Size, Crossing, Length);

	OnTargetLaunched.Broadcast(Target, Source);
}
