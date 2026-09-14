// CYB3RGUN THEGAME. A flying target: flies its path, reacts to a hit, falls (D-078, D-079).

#include "FlightTarget.h"
#include "FlightTargetDefinition.h"
#include "HitReactions.h"
#include "StyleScoringComponent.h"
#include "StyleSettings.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

DEFINE_LOG_CATEGORY_STATIC(LogFlightTarget, Log, All);

namespace FlightTargetTuning
{
	/** Seconds a fallen target may drop before it is removed, even if it never reaches the ground */
	constexpr float MaxFallSeconds = 4.0f;

	/** Wing angle of a falling target, folded up */
	constexpr float FoldedWingDegrees = 60.0f;

	/** Steepest climb or dive the body leans into */
	constexpr float MaxPitchDegrees = 35.0f;
}

AFlightTarget::AFlightTarget()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	BodyRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Body"));
	BodyRoot->SetupAttachment(Root);

	BodyHit = CreateDefaultSubobject<USphereComponent>(TEXT("BodyHit"));
	BodyHit->SetupAttachment(Root);
	HeadHit = CreateDefaultSubobject<USphereComponent>(TEXT("HeadHit"));
	HeadHit->SetupAttachment(Root);
	BodyLead = CreateDefaultSubobject<USphereComponent>(TEXT("BodyLead"));
	BodyLead->SetupAttachment(Root);
	HeadLead = CreateDefaultSubobject<USphereComponent>(TEXT("HeadLead"));
	HeadLead->SetupAttachment(Root);

	// the spheres at the body answer projectiles, the leading spheres the traces of hitscan pellets and of the aim
	SetupHitSphere(BodyHit, FHitReactions::ProjectileChannel);
	SetupHitSphere(HeadHit, FHitReactions::ProjectileChannel);
	SetupHitSphere(BodyLead, ECC_Visibility);
	SetupHitSphere(HeadLead, ECC_Visibility);
}

void AFlightTarget::SetupHitSphere(USphereComponent* Sphere, ECollisionChannel Channel) const
{
	Sphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Sphere->SetCollisionObjectType(ECC_WorldDynamic);
	Sphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	Sphere->SetCollisionResponseToChannel(Channel, ECR_Block);
	Sphere->SetGenerateOverlapEvents(false);
	Sphere->SetCanEverAffectNavigation(false);
	Sphere->SetHiddenInGame(true);
	Sphere->SetUsingAbsoluteRotation(true);
}

void AFlightTarget::Launch(const UFlightTargetDefinition* InDefinition, const FFlightPathMotion& InMotion, float InSize, const FVector& InShooterLocation, float InLeadShotSpeed, float InGroundZ)
{
	Definition = InDefinition;
	Motion = InMotion;
	Size = FMath::Max(InSize, 0.1f);
	ShooterLocation = InShooterLocation;
	LeadShotSpeed = FMath::Max(InLeadShotSpeed, 1000.0f);
	GroundZ = InGroundZ;

	BuildBody();

	const float HeadRadius = Definition ? Definition->HeadHitRadius * Size : 0.0f;
	const float BodyRadius = Definition ? Definition->BodyHitRadius * Size : 20.0f;
	BodyHit->SetSphereRadius(BodyRadius);
	BodyLead->SetSphereRadius(BodyRadius);
	HeadHit->SetSphereRadius(HeadRadius);
	HeadLead->SetSphereRadius(HeadRadius);
	if (HeadRadius <= 0.0f)
	{
		HeadHit->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		HeadLead->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	SetActorLocation(Motion.GetLocation());
	PlaceHitSpheres();

	if (Definition && Definition->LaunchSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Definition->LaunchSound, Motion.GetLocation(), 1.0f, Definition->LaunchPitch);
	}
}

void AFlightTarget::BuildBody()
{
	if (!Definition)
	{
		return;
	}

	BodyRoot->SetRelativeScale3D(FVector(Size));

	auto AddMesh = [this](USceneComponent* Parent, UStaticMesh* Mesh, UMaterialInterface* Material, const FTransform& Transform)
	{
		UStaticMeshComponent* Piece = NewObject<UStaticMeshComponent>(this);
		Piece->SetupAttachment(Parent);
		Piece->SetRelativeTransform(Transform);
		Piece->SetStaticMesh(Mesh);
		if (Material)
		{
			Piece->SetMaterial(0, Material);
		}
		Piece->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Piece->SetGenerateOverlapEvents(false);
		Piece->SetCanEverAffectNavigation(false);
		Piece->RegisterComponent();
	};

	// a finished model replaces the primitive body
	if (Definition->Mesh)
	{
		AddMesh(BodyRoot, Definition->Mesh, Definition->MeshMaterial, Definition->MeshTransform);
		return;
	}

	for (const FFlightTargetPart& Part : Definition->Parts)
	{
		if (!Part.Mesh)
		{
			continue;
		}

		USceneComponent* Pivot = NewObject<USceneComponent>(this);
		Pivot->SetupAttachment(BodyRoot);
		Pivot->SetRelativeLocation(Part.Pivot);
		Pivot->RegisterComponent();
		AddMesh(Pivot, Part.Mesh, Part.Material, Part.Transform);

		if (!FMath::IsNearlyZero(Part.FlapSign))
		{
			WingPivots.Add(Pivot);
			WingSigns.Add(FMath::Sign(Part.FlapSign));
		}
	}
}

void AFlightTarget::PlaceHitSpheres()
{
	const FVector Location = GetActorLocation();
	const FVector HeadLocal = Definition ? BodyRoot->GetComponentQuat().RotateVector(Definition->HeadOffset * Size) : FVector::ZeroVector;
	const FVector Lead = GetLeadPoint() - Location;

	BodyHit->SetWorldLocation(Location);
	HeadHit->SetWorldLocation(Location + HeadLocal);
	BodyLead->SetWorldLocation(Location + Lead);
	HeadLead->SetWorldLocation(Location + HeadLocal + Lead);
}

float AFlightTarget::GetShooterDistance() const
{
	return bDown ? DistanceAtHit : FVector::Dist(ShooterLocation, GetActorLocation());
}

FVector AFlightTarget::GetLeadPoint() const
{
	// the charge needs distance over speed to arrive, the target moves on by its velocity meanwhile
	const float FlightSeconds = FVector::Dist(ShooterLocation, GetActorLocation()) / LeadShotSpeed;
	return GetActorLocation() + GetFlightVelocity() * FlightSeconds;
}

void AFlightTarget::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!Definition || bDone)
	{
		return;
	}

	if (!bDown)
	{
		FlightTime += DeltaSeconds;
		Motion.Advance(DeltaSeconds);
		SetActorLocation(Motion.GetLocation());

		// the body looks where it flies and leans into climbs and dives
		const FVector Velocity = Motion.GetVelocity();
		if (!Velocity.IsNearlyZero())
		{
			const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(Velocity.Y, Velocity.X));
			const float Pitch = FMath::Clamp(FMath::RadiansToDegrees(FMath::Atan2(Velocity.Z, Velocity.Size2D())), -FlightTargetTuning::MaxPitchDegrees, FlightTargetTuning::MaxPitchDegrees);
			BodyRoot->SetWorldRotation(FRotator(Pitch, Yaw, 0.0f));
		}

		// smaller bodies beat their wings faster
		const float Beat = FMath::Sin(UE_TWO_PI * Definition->FlapFrequency / Size * FlightTime) * Definition->FlapDegrees;
		for (int32 Index = 0; Index < WingPivots.Num(); ++Index)
		{
			if (WingPivots[Index])
			{
				WingPivots[Index]->SetRelativeRotation(FRotator(0.0f, 0.0f, WingSigns[Index] * Beat));
			}
		}

		PlaceHitSpheres();
		if (Motion.IsFinished())
		{
			Finish(false);
		}
		return;
	}

	// down: a follow up shot inside the controlled pair window still counts, after that the spheres go
	const double Now = GetWorld()->GetTimeSeconds();
	if (bHitSpheresOn && Now - HitAt > UStyleSettings::Get(this)->ControlledPairWindow)
	{
		bHitSpheresOn = false;
		for (USphereComponent* Sphere : { BodyHit.Get(), HeadHit.Get(), BodyLead.Get(), HeadLead.Get() })
		{
			Sphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
		if (Definition->HitBehaviour == EFlightHitBehaviour::Vanish)
		{
			Finish(true);
			return;
		}
	}

	FallTime += DeltaSeconds;
	FallVelocity.Z -= Definition->FallGravity * DeltaSeconds;
	SetActorLocation(GetActorLocation() + FallVelocity * DeltaSeconds);
	BodyRoot->AddLocalRotation(FRotator(Definition->FallSpinDegreesPerSecond * 0.35f * DeltaSeconds, 0.0f, Definition->FallSpinDegreesPerSecond * DeltaSeconds));
	if (bHitSpheresOn)
	{
		PlaceHitSpheres();
	}

	if (GetActorLocation().Z <= GroundZ || FallTime >= FlightTargetTuning::MaxFallSeconds)
	{
		Finish(true);
	}
}

bool AFlightTarget::NotifyShot(const FHitResult& Hit, const FVector& ShotDirection, AController* InstigatedBy)
{
	const UPrimitiveComponent* Component = Hit.GetComponent();
	const bool bHead = Component == HeadHit || Component == HeadLead;
	const bool bBody = Component == BodyHit || Component == BodyLead;
	if (bDone || !Definition || !(bHead || bBody))
	{
		return false;
	}

	const EHitZone Zone = bHead ? EHitZone::Head : EHitZone::Torso;
	UStyleScoringComponent* Style = UStyleScoringComponent::ForController(InstigatedBy);
	const double Now = GetWorld()->GetTimeSeconds();

	// a second shot on a falling target is a hit for the style record and can complete a controlled pair; it scores nothing here
	if (bDown)
	{
		const bool bFollowUp = Now - HitAt <= UStyleSettings::Get(this)->ControlledPairWindow;
		if (bFollowUp && Style)
		{
			Style->RecordTargetHit(this, Zone, false);
		}
		return bFollowUp;
	}

	bDown = true;
	HitAt = Now;
	SpeedAtHit = Motion.GetSpeed();
	DistanceAtHit = FVector::Dist(ShooterLocation, GetActorLocation());
	FallVelocity = Motion.GetVelocity() * Definition->FallCarry + ShotDirection.GetSafeNormal() * 150.0f;

	// the wings fold as it drops
	for (int32 Index = 0; Index < WingPivots.Num(); ++Index)
	{
		if (WingPivots[Index])
		{
			WingPivots[Index]->SetRelativeRotation(FRotator(0.0f, 0.0f, WingSigns[Index] * FlightTargetTuning::FoldedWingDegrees));
		}
	}
	if (Definition->HitBehaviour == EFlightHitBehaviour::Vanish)
	{
		BodyRoot->SetVisibility(false, true);
	}

	UE_LOG(LogFlightTarget, Verbose, TEXT("%s hit in the %s at %.0f cm, %.0f cm/s, size %.2f"), *GetName(), bHead ? TEXT("head") : TEXT("body"), DistanceAtHit, SpeedAtHit, Size);

	if (Style)
	{
		Style->RecordTargetHit(this, Zone, true);
	}
	if (Definition->HitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Definition->HitSound, GetActorLocation(), 1.0f, Definition->HitPitch);
	}
	OnHit.Broadcast(this, Zone, InstigatedBy);
	return true;
}

void AFlightTarget::Retire()
{
	Finish(bDown);
}

void AFlightTarget::Finish(bool bWasHit)
{
	if (bDone)
	{
		return;
	}
	bDone = true;
	OnDone.Broadcast(this, bWasHit);
	Destroy();
}

void AFlightTarget::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// a target removed with its level still tells its listeners
	if (!bDone)
	{
		bDone = true;
		OnDone.Broadcast(this, bDown);
	}
	Super::EndPlay(EndPlayReason);
}
