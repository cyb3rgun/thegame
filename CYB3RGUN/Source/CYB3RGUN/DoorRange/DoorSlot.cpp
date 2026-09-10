// CYB3RGUN THEGAME. One door of the door module.

#include "DoorSlot.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	UStaticMeshComponent* MakeShape(AActor* Owner, USceneComponent* Parent, const TCHAR* Name, UStaticMesh* Mesh, const FVector& Location, const FVector& Scale)
	{
		UStaticMeshComponent* Component = Owner->CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Component->SetupAttachment(Parent);
		Component->SetStaticMesh(Mesh);
		Component->SetRelativeLocation(Location);
		Component->SetRelativeScale3D(Scale);
		Component->SetCollisionProfileName(FName("BlockAll"));
		Component->SetGenerateOverlapEvents(false);
		Component->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
		return Component;
	}

	void SetShapeActive(UStaticMeshComponent* Component, bool bActive)
	{
		Component->SetVisibility(bActive);
		Component->SetCollisionEnabled(bActive ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	}
}

ADoorSlot::ADoorSlot()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMesh(TEXT("/Engine/BasicShapes/Cone.Cone"));

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	// The actor's forward (+X) faces the player. The door plane is the YZ plane at X = 0.
	// Basic shapes are 100 cm cubes, cylinders, spheres and cones, so scale is size in metres.
	FramePostLeft = MakeShape(this, Root, TEXT("FramePostLeft"), CubeMesh.Object, FVector(0.0f, -55.0f, 100.0f), FVector(0.15f, 0.1f, 2.0f));
	FramePostRight = MakeShape(this, Root, TEXT("FramePostRight"), CubeMesh.Object, FVector(0.0f, 55.0f, 100.0f), FVector(0.15f, 0.1f, 2.0f));
	FrameTop = MakeShape(this, Root, TEXT("FrameTop"), CubeMesh.Object, FVector(0.0f, 0.0f, 205.0f), FVector(0.15f, 1.2f, 0.1f));
	BackWall = MakeShape(this, Root, TEXT("BackWall"), CubeMesh.Object, FVector(-130.0f, 0.0f, 110.0f), FVector(0.1f, 1.6f, 2.4f));

	Hinge = CreateDefaultSubobject<USceneComponent>(TEXT("Hinge"));
	Hinge->SetupAttachment(Root);
	Hinge->SetRelativeLocation(FVector(0.0f, -50.0f, 0.0f));

	DoorPanel = MakeShape(this, Hinge, TEXT("DoorPanel"), CubeMesh.Object, FVector(0.0f, 50.0f, 100.0f), FVector(0.06f, 1.0f, 2.0f));

	SpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("SpawnPoint"));
	SpawnPoint->SetupAttachment(Root);
	SpawnPoint->SetRelativeLocation(FVector(-60.0f, 0.0f, 0.0f));

	OccupantRoot = CreateDefaultSubobject<USceneComponent>(TEXT("OccupantRoot"));
	OccupantRoot->SetupAttachment(SpawnPoint);

	// Hostile: tall narrow body, wide arm bar at shoulder height, cone head. A spike with a crossbar.
	HostileBody = MakeShape(this, OccupantRoot, TEXT("HostileBody"), CylinderMesh.Object, FVector(0.0f, 0.0f, 65.0f), FVector(0.4f, 0.4f, 1.3f));
	HostileArms = MakeShape(this, OccupantRoot, TEXT("HostileArms"), CubeMesh.Object, FVector(0.0f, 0.0f, 118.0f), FVector(0.14f, 1.3f, 0.12f));
	HostileHead = MakeShape(this, OccupantRoot, TEXT("HostileHead"), ConeMesh.Object, FVector(0.0f, 0.0f, 158.0f), FVector(0.45f, 0.45f, 0.55f));

	// Friendly: short round body and round head. A snowman.
	FriendlyBody = MakeShape(this, OccupantRoot, TEXT("FriendlyBody"), SphereMesh.Object, FVector(0.0f, 0.0f, 42.0f), FVector(0.85f, 0.85f, 0.85f));
	FriendlyHead = MakeShape(this, OccupantRoot, TEXT("FriendlyHead"), SphereMesh.Object, FVector(0.0f, 0.0f, 108.0f), FVector(0.5f, 0.5f, 0.5f));
}

void ADoorSlot::BeginPlay()
{
	Super::BeginPlay();

	HideOccupant();
	ApplyPanelAlpha(0.0f);
	SetState(EDoorState::Closed);
}

void ADoorSlot::Open(const FDoorOpenParams& InParams)
{
	if (State != EDoorState::Closed)
	{
		return;
	}

	Params = InParams;
	Params.OpenDuration = FMath::Max(Params.OpenDuration, 0.01f);
	Params.ExposureWindow = FMath::Max(Params.ExposureWindow, 0.0f);
	Params.CloseDuration = FMath::Max(Params.CloseDuration, 0.01f);
	Params.TelegraphDuration = FMath::Max(Params.TelegraphDuration, 0.0f);

	bHitRegistered = false;
	bDrawn = false;
	DrawnAt = 0.0f;
	HitReactionElapsed = 0.0f;
	bReportNextClose = true;

	if (Params.Occupant != EDoorOccupant::Empty)
	{
		ShowOccupant(Params.Occupant, Params.OccupantMaterial);
	}

	// hostiles start crouched behind the door, friendlies are up at once
	SetOccupantRise(Params.Occupant == EDoorOccupant::Hostile ? 0.0f : 1.0f);
	ApplyHitReaction(0.0f);

	PlaySlotSound(Params.DoorSound, Params.DoorPitch);
	SetState(EDoorState::Opening);
}

void ADoorSlot::ForceClose(bool bReportClose)
{
	if (State == EDoorState::Opening || State == EDoorState::Showing)
	{
		bReportNextClose = bReportClose;
		SetState(EDoorState::Closing);
	}
	else if (State == EDoorState::Closing)
	{
		// already swinging shut, only decide whether that closing gets reported
		bReportNextClose = bReportClose;
	}
}

bool ADoorSlot::RegisterHit()
{
	if (State == EDoorState::Closed || Params.Occupant == EDoorOccupant::Empty || bHitRegistered || !bDrawn)
	{
		return false;
	}

	bHitRegistered = true;
	HitReactionElapsed = 0.0f;

	const float ExposureFraction = GetExposureFraction();
	OnSlotHit.Broadcast(this, Params.Occupant, ExposureFraction);

	// the occupant tips over and the door swings shut
	ForceClose();

	return true;
}

float ADoorSlot::GetExposureFraction() const
{
	switch (State)
	{
	case EDoorState::Showing:
		if (!bDrawn)
		{
			return 0.0f;
		}
		return Params.ExposureWindow > 0.0f ? FMath::Clamp((StateElapsed - DrawnAt) / Params.ExposureWindow, 0.0f, 1.0f) : 1.0f;
	case EDoorState::Closing:
		return 1.0f;
	default:
		return 0.0f;
	}
}

FVector ADoorSlot::GetOccupantAimPoint() const
{
	const float Height = Params.Occupant == EDoorOccupant::Hostile ? 110.0f : 70.0f;
	return SpawnPoint->GetComponentLocation() + FVector(0.0f, 0.0f, Height);
}

bool ADoorSlot::NotifyShot(UPrimitiveComponent* HitComponent, const FVector& HitLocation, AController* InstigatedBy)
{
	// only the occupant counts, hits on the frame or the panel are ignored
	if (!IsOccupantComponent(HitComponent))
	{
		return false;
	}

	return RegisterHit();
}

void ADoorSlot::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	StateElapsed += DeltaSeconds;

	switch (State)
	{
	case EDoorState::Opening:
	{
		const float Alpha = FMath::Clamp(StateElapsed / Params.OpenDuration, 0.0f, 1.0f);
		ApplyPanelAlpha(Alpha);
		if (Alpha >= 1.0f)
		{
			SetState(EDoorState::Showing);
		}
		break;
	}
	case EDoorState::Showing:
	{
		if (!bDrawn)
		{
			// hostile telegraph: rise from the crouch, then draw
			const float Rise = Params.TelegraphDuration > 0.0f ? FMath::Clamp(StateElapsed / Params.TelegraphDuration, 0.0f, 1.0f) : 1.0f;
			SetOccupantRise(Rise);
			if (Rise >= 1.0f)
			{
				bDrawn = true;
				DrawnAt = StateElapsed;
				PlaySlotSound(Params.DrawSound, Params.DrawPitch);
				OnSlotDrawn.Broadcast(this);
			}
		}
		else if (StateElapsed - DrawnAt >= Params.ExposureWindow)
		{
			SetState(EDoorState::Closing);
		}
		break;
	}
	case EDoorState::Closing:
	{
		const float CloseTime = FMath::Max(Params.CloseDuration * ClosingStartAlpha, 0.01f);
		const float Alpha = ClosingStartAlpha * (1.0f - FMath::Clamp(StateElapsed / CloseTime, 0.0f, 1.0f));
		ApplyPanelAlpha(Alpha);
		if (StateElapsed >= CloseTime)
		{
			SetState(EDoorState::Closed);
		}
		break;
	}
	default:
		break;
	}

	if (bHitRegistered && State != EDoorState::Closed)
	{
		HitReactionElapsed += DeltaSeconds;
		ApplyHitReaction(FMath::Clamp(HitReactionElapsed / HitReactionDuration, 0.0f, 1.0f));
	}
}

void ADoorSlot::SetState(EDoorState NewState)
{
	State = NewState;
	StateElapsed = 0.0f;

	switch (State)
	{
	case EDoorState::Opening:
		SetActorTickEnabled(true);
		break;
	case EDoorState::Showing:
		ApplyPanelAlpha(1.0f);
		if (Params.Occupant == EDoorOccupant::Hostile)
		{
			PlaySlotSound(Params.TelegraphSound, Params.TelegraphPitch);
		}
		else
		{
			bDrawn = true;
			DrawnAt = 0.0f;
		}
		break;
	case EDoorState::Closing:
		ClosingStartAlpha = FMath::Max(PanelAlpha, 0.01f);
		break;
	case EDoorState::Closed:
	{
		ApplyPanelAlpha(0.0f);
		HideOccupant();
		SetActorTickEnabled(false);
		PlaySlotSound(Params.DoorSound, Params.DoorPitch);

		const EDoorOccupant ClosedOccupant = Params.Occupant;
		const bool bWasHit = bHitRegistered;
		const bool bReport = bReportNextClose;
		Params.Occupant = EDoorOccupant::Empty;
		bHitRegistered = false;
		bDrawn = false;
		bReportNextClose = true;

		if (bReport)
		{
			OnSlotClosed.Broadcast(this, ClosedOccupant, bWasHit);
		}
		break;
	}
	default:
		break;
	}
}

void ADoorSlot::ApplyPanelAlpha(float Alpha)
{
	PanelAlpha = Alpha;
	Hinge->SetRelativeRotation(FRotator(0.0f, OpenAngle * Alpha, 0.0f));
}

void ADoorSlot::ShowOccupant(EDoorOccupant Occupant, UMaterialInterface* Material)
{
	const bool bHostile = Occupant == EDoorOccupant::Hostile;
	const bool bFriendly = Occupant == EDoorOccupant::Friendly;

	SetShapeActive(HostileBody, bHostile);
	SetShapeActive(HostileArms, bHostile);
	SetShapeActive(HostileHead, bHostile);
	SetShapeActive(FriendlyBody, bFriendly);
	SetShapeActive(FriendlyHead, bFriendly);

	if (Material)
	{
		for (UStaticMeshComponent* Shape : { HostileBody, HostileArms, HostileHead, FriendlyBody, FriendlyHead })
		{
			Shape->SetMaterial(0, Material);
		}
	}
}

void ADoorSlot::HideOccupant()
{
	for (UStaticMeshComponent* Shape : { HostileBody, HostileArms, HostileHead, FriendlyBody, FriendlyHead })
	{
		SetShapeActive(Shape, false);
	}
	SetOccupantRise(1.0f);
	ApplyHitReaction(0.0f);
}

void ADoorSlot::SetOccupantRise(float RiseAlpha)
{
	const float Scale = FMath::Lerp(CrouchScale, 1.0f, FMath::Clamp(RiseAlpha, 0.0f, 1.0f));
	OccupantRoot->SetRelativeScale3D(FVector(1.0f, 1.0f, Scale));
}

void ADoorSlot::ApplyHitReaction(float ReactionAlpha)
{
	// tip backwards away from the player, the occupant root sits at floor level so it falls over its feet
	OccupantRoot->SetRelativeRotation(FRotator(80.0f * ReactionAlpha, 0.0f, 0.0f));
}

void ADoorSlot::PlaySlotSound(USoundBase* Sound, float Pitch) const
{
	if (Sound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Sound, SpawnPoint->GetComponentLocation(), 1.0f, Pitch);
	}
}

bool ADoorSlot::IsOccupantComponent(const UPrimitiveComponent* Component) const
{
	return Component && (Component == HostileBody || Component == HostileArms || Component == HostileHead || Component == FriendlyBody || Component == FriendlyHead);
}
