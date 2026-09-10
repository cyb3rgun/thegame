// CYB3RGUN THEGAME. One door of the door module.

#include "DoorSlot.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
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
}

ADoorSlot::ADoorSlot()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	// The actor's forward (+X) faces the player. The door plane is the YZ plane at X = 0.
	// Basic shapes are 100 cm cubes, cylinders and spheres, so scale is size in metres.
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

	OccupantBody = MakeShape(this, SpawnPoint, TEXT("OccupantBody"), CylinderMesh.Object, FVector(0.0f, 0.0f, 70.0f), FVector(0.5f, 0.5f, 1.4f));
	OccupantHead = MakeShape(this, SpawnPoint, TEXT("OccupantHead"), SphereMesh.Object, FVector(0.0f, 0.0f, 165.0f), FVector(0.4f, 0.4f, 0.4f));
}

void ADoorSlot::BeginPlay()
{
	Super::BeginPlay();

	SetOccupantVisible(false);
	ApplyPanelAlpha(0.0f);
	SetState(EDoorState::Closed);
}

void ADoorSlot::Open(EDoorOccupant InOccupant, UMaterialInterface* OccupantMaterial, float InOpenDuration, float InExposureWindow, float InCloseDuration)
{
	if (State != EDoorState::Closed)
	{
		return;
	}

	Occupant = InOccupant;
	OpenDuration = FMath::Max(InOpenDuration, 0.01f);
	ExposureWindow = FMath::Max(InExposureWindow, 0.0f);
	CloseDuration = FMath::Max(InCloseDuration, 0.01f);
	bHitRegistered = false;

	if (Occupant != EDoorOccupant::Empty)
	{
		if (OccupantMaterial)
		{
			OccupantBody->SetMaterial(0, OccupantMaterial);
			OccupantHead->SetMaterial(0, OccupantMaterial);
		}
		SetOccupantVisible(true);
	}

	SetState(EDoorState::Opening);
}

void ADoorSlot::ForceClose()
{
	if (State == EDoorState::Opening || State == EDoorState::Showing)
	{
		SetState(EDoorState::Closing);
	}
}

bool ADoorSlot::RegisterHit()
{
	if (State == EDoorState::Closed || Occupant == EDoorOccupant::Empty || bHitRegistered)
	{
		return false;
	}

	bHitRegistered = true;

	const float ExposureFraction = GetExposureFraction();
	OnSlotHit.Broadcast(this, Occupant, ExposureFraction);

	// the occupant ducks away and the door swings shut
	SetOccupantVisible(false);
	ForceClose();

	return true;
}

float ADoorSlot::GetExposureFraction() const
{
	switch (State)
	{
	case EDoorState::Showing:
		return ExposureWindow > 0.0f ? FMath::Clamp(StateElapsed / ExposureWindow, 0.0f, 1.0f) : 1.0f;
	case EDoorState::Closing:
		return 1.0f;
	default:
		return 0.0f;
	}
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
		const float Alpha = FMath::Clamp(StateElapsed / OpenDuration, 0.0f, 1.0f);
		ApplyPanelAlpha(Alpha);
		if (Alpha >= 1.0f)
		{
			SetState(EDoorState::Showing);
		}
		break;
	}
	case EDoorState::Showing:
	{
		if (StateElapsed >= ExposureWindow)
		{
			SetState(EDoorState::Closing);
		}
		break;
	}
	case EDoorState::Closing:
	{
		const float CloseTime = FMath::Max(CloseDuration * ClosingStartAlpha, 0.01f);
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
		break;
	case EDoorState::Closing:
		ClosingStartAlpha = FMath::Max(PanelAlpha, 0.01f);
		break;
	case EDoorState::Closed:
	{
		ApplyPanelAlpha(0.0f);
		SetOccupantVisible(false);
		SetActorTickEnabled(false);

		const EDoorOccupant ClosedOccupant = Occupant;
		const bool bWasHit = bHitRegistered;
		Occupant = EDoorOccupant::Empty;
		bHitRegistered = false;

		OnSlotClosed.Broadcast(this, ClosedOccupant, bWasHit);
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

void ADoorSlot::SetOccupantVisible(bool bVisible)
{
	const ECollisionEnabled::Type Collision = bVisible ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision;

	OccupantBody->SetVisibility(bVisible);
	OccupantBody->SetCollisionEnabled(Collision);
	OccupantHead->SetVisibility(bVisible);
	OccupantHead->SetCollisionEnabled(Collision);
}

bool ADoorSlot::IsOccupantComponent(const UPrimitiveComponent* Component) const
{
	return Component && (Component == OccupantBody || Component == OccupantHead);
}
