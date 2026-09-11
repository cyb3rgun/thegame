// CYB3RGUN THEGAME. One door of the door module.

#include "DoorSlot.h"
#include "StyleScoringComponent.h"
#include "StyleSettings.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
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

	/** A hit volume is a shape that is never drawn */
	UStaticMeshComponent* MakeHitVolume(AActor* Owner, USceneComponent* Parent, const TCHAR* Name, UStaticMesh* Mesh, const FVector& Location, const FVector& Scale)
	{
		UStaticMeshComponent* Component = MakeShape(Owner, Parent, Name, Mesh, Location, Scale);
		Component->SetVisibility(false);
		Component->SetHiddenInGame(true);
		Component->SetCastShadow(false);
		return Component;
	}

	USkeletalMeshComponent* MakeBody(AActor* Owner, USceneComponent* Parent, const TCHAR* Name, USkeletalMesh* Mesh)
	{
		USkeletalMeshComponent* Body = Owner->CreateDefaultSubobject<USkeletalMeshComponent>(Name);
		Body->SetupAttachment(Parent);
		Body->SetSkeletalMeshAsset(Mesh);
		// the mannequin faces +Y, the slot faces the player along +X
		Body->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
		Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Body->SetGenerateOverlapEvents(false);
		Body->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
		Body->SetAnimationMode(EAnimationMode::AnimationSingleNode);
		Body->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
		Body->SetVisibility(false);
		return Body;
	}

	/** Hit volumes stay hidden, only their collision is switched */
	void SetShapeActive(UStaticMeshComponent* Component, bool bActive)
	{
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
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> HostileBodyMesh(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> FriendlyBodyMesh(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple"));
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> PistolMesh(TEXT("/Game/Weapons/Pistol/Meshes/SKM_Pistol.SKM_Pistol"));
	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> DrawAnim(TEXT("/Game/Characters/Mannequins/Anims/Pistol/MM_Pistol_Equip.MM_Pistol_Equip"));
	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> AimAnim(TEXT("/Game/Characters/Mannequins/Anims/Pistol/MF_Pistol_Idle_ADS.MF_Pistol_Idle_ADS"));
	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> IdleAnim(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle.MM_Idle"));
	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> HitAnim(TEXT("/Game/Characters/Mannequins/Anims/Death/MM_Death_Back_01.MM_Death_Back_01"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> FrameLook(TEXT("/Game/CYB3RGUN/Core/Materials/MI_Door_Frame.MI_Door_Frame"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> PanelLook(TEXT("/Game/CYB3RGUN/Core/Materials/MI_Door_Panel.MI_Door_Panel"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> WallLook(TEXT("/Game/CYB3RGUN/Core/Materials/MI_Wall_Alcove.MI_Wall_Alcove"));
	FrameMaterial = FrameLook.Object;
	PanelMaterial = PanelLook.Object;
	WallMaterial = WallLook.Object;
	HostileDrawAnimation = DrawAnim.Object;
	HostileAimAnimation = AimAnim.Object;
	FriendlyIdleAnimation = IdleAnim.Object;
	HitAnimation = HitAnim.Object;

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

	// Hostile: the taller mannequin with a pistol in the right hand. The volumes wrap the aiming
	// pose: torso and legs, the arms held out towards the player, the head.
	HostileMesh = MakeBody(this, OccupantRoot, TEXT("HostileMesh"), HostileBodyMesh.Object);
	HostileWeapon = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("HostileWeapon"));
	HostileWeapon->SetupAttachment(HostileMesh, FName("HandGrip_R"));
	HostileWeapon->SetSkeletalMeshAsset(PistolMesh.Object);
	HostileWeapon->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HostileWeapon->SetGenerateOverlapEvents(false);
	HostileWeapon->SetVisibility(false);
	HostileBody = MakeHitVolume(this, OccupantRoot, TEXT("HostileBody"), CylinderMesh.Object, FVector(0.0f, 0.0f, 78.0f), FVector(0.42f, 0.42f, 1.56f));
	HostileArms = MakeHitVolume(this, OccupantRoot, TEXT("HostileArms"), CubeMesh.Object, FVector(30.0f, 0.0f, 148.0f), FVector(0.6f, 0.3f, 0.16f));
	HostileHead = MakeHitVolume(this, OccupantRoot, TEXT("HostileHead"), SphereMesh.Object, FVector(0.0f, 0.0f, 176.0f), FVector(0.3f, 0.3f, 0.32f));

	// Friendly: the smaller mannequin, empty hands, at ease.
	FriendlyMesh = MakeBody(this, OccupantRoot, TEXT("FriendlyMesh"), FriendlyBodyMesh.Object);
	FriendlyBody = MakeHitVolume(this, OccupantRoot, TEXT("FriendlyBody"), CylinderMesh.Object, FVector(0.0f, 0.0f, 65.0f), FVector(0.4f, 0.4f, 1.3f));
	FriendlyHead = MakeHitVolume(this, OccupantRoot, TEXT("FriendlyHead"), SphereMesh.Object, FVector(0.0f, 0.0f, 145.0f), FVector(0.28f, 0.28f, 0.3f));

	ApplyBodyScale();
}

void ADoorSlot::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyLookMaterials();
	ApplyBodyScale();
}

void ADoorSlot::ApplyLookMaterials()
{
	for (UStaticMeshComponent* Part : { FramePostLeft, FramePostRight, FrameTop })
	{
		if (FrameMaterial)
		{
			Part->SetMaterial(0, FrameMaterial);
		}
	}
	if (PanelMaterial)
	{
		DoorPanel->SetMaterial(0, PanelMaterial);
	}
	if (WallMaterial)
	{
		BackWall->SetMaterial(0, WallMaterial);
	}
}

void ADoorSlot::BeginPlay()
{
	Super::BeginPlay();

	ApplyLookMaterials();

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
	GetWorldTimerManager().ClearTimer(HitCloseTimer);

	if (Params.Occupant != EDoorOccupant::Empty)
	{
		ShowOccupant(Params.Occupant, Params.OccupantMaterial);
	}

	// a hostile waits with the pistol down while the door opens, a friendly is at ease from the start
	if (Params.Occupant == EDoorOccupant::Hostile)
	{
		PoseHostileReady();
	}
	else if (Params.Occupant == EDoorOccupant::Friendly)
	{
		PlayBodyLoop(FriendlyMesh, FriendlyIdleAnimation);
	}
	ApplyHitReaction(0.0f);

	PlaySlotSound(Params.DoorSound, Params.DoorPitch);
	SetState(EDoorState::Opening);
}

void ADoorSlot::ForceClose(bool bReportClose)
{
	GetWorldTimerManager().ClearTimer(HitCloseTimer);

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
	HitRegisteredAt = GetWorld()->GetTimeSeconds();
	HitReactionElapsed = 0.0f;

	USkeletalMeshComponent* Body = GetShownBody();
	if (Body && HitAnimation)
	{
		Body->PlayAnimation(HitAnimation, false);
		Body->SetPlayRate(HitAnimationRate);
	}

	const float ExposureFraction = GetExposureFraction();
	OnSlotHit.Broadcast(this, Params.Occupant, ExposureFraction);

	// the occupant falls, and the door swings shut once the controlled pair window has passed, so the
	// second shot of a pair is not stopped by the closing panel
	const float CloseDelay = UStyleSettings::Get(this)->ControlledPairWindow;
	if (CloseDelay > 0.0f)
	{
		GetWorldTimerManager().SetTimer(HitCloseTimer, FTimerDelegate::CreateWeakLambda(this, [this]() { ForceClose(); }), CloseDelay, false);
	}
	else
	{
		ForceClose();
	}

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
		// the reward shrinks with the closing panel, a shot that lands on a shut door is worth nothing extra
		return ClosingStartAlpha > 0.0f ? BonusAtCloseStart * FMath::Clamp(PanelAlpha / ClosingStartAlpha, 0.0f, 1.0f) : 0.0f;
	default:
		return 0.0f;
	}
}

FVector ADoorSlot::GetOccupantAimPoint() const
{
	const float Height = Params.Occupant == EDoorOccupant::Hostile ? 125.0f : 100.0f;
	return SpawnPoint->GetComponentLocation() + FVector(0.0f, 0.0f, Height);
}

FVector ADoorSlot::GetOccupantHeadPoint() const
{
	return (Params.Occupant == EDoorOccupant::Friendly ? FriendlyHead : HostileHead)->GetComponentLocation();
}

bool ADoorSlot::NotifyShot(UPrimitiveComponent* HitComponent, const FVector& HitLocation, AController* InstigatedBy)
{
	// only the occupant counts, hits on the frame or the panel are ignored
	if (!IsOccupantComponent(HitComponent))
	{
		return false;
	}

	UStyleScoringComponent* Style = UStyleScoringComponent::ForController(InstigatedBy);

	// a second shot on a hostile that is already going down scores nothing on the door, but it is a hit
	// for the style record and can complete a controlled pair
	if (bHitRegistered)
	{
		const bool bFollowUp = Params.Occupant == EDoorOccupant::Hostile && GetWorld()->GetTimeSeconds() - HitRegisteredAt <= UStyleSettings::Get(this)->ControlledPairWindow;
		if (bFollowUp && Style)
		{
			Style->RecordTargetHit(this, false, false);
		}
		return bFollowUp;
	}

	const EDoorOccupant Occupant = Params.Occupant;
	if (!RegisterHit())
	{
		return false;
	}

	if (Style)
	{
		if (Occupant == EDoorOccupant::Hostile)
		{
			Style->RecordTargetHit(this, HitComponent == HostileHead, true);
		}
		else if (Occupant == EDoorOccupant::Friendly)
		{
			Style->RecordNonTargetHit(this);
		}
	}
	return true;
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
			// hostile telegraph: the draw animation runs, the hostile is shootable once it ends
			if (StateElapsed >= Params.TelegraphDuration)
			{
				bDrawn = true;
				DrawnAt = StateElapsed;
				PlayBodyLoop(HostileMesh, HostileAimAnimation);
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
	// capture the draw bonus before the state clock resets, a close inherits the fraction reached so far
	if (NewState == EDoorState::Closing)
	{
		BonusAtCloseStart = (State == EDoorState::Showing) ? GetExposureFraction() : 0.0f;
	}

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
			StartHostileDraw();
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

	HostileMesh->SetVisibility(bHostile, true);
	FriendlyMesh->SetVisibility(bFriendly);

	USkeletalMeshComponent* Body = bHostile ? HostileMesh : (bFriendly ? FriendlyMesh : nullptr);
	if (Body && Material)
	{
		for (int32 Index = 0; Index < Body->GetNumMaterials(); ++Index)
		{
			Body->SetMaterial(Index, Material);
		}
	}
}

void ADoorSlot::HideOccupant()
{
	for (UStaticMeshComponent* Shape : { HostileBody, HostileArms, HostileHead, FriendlyBody, FriendlyHead })
	{
		SetShapeActive(Shape, false);
	}
	HostileMesh->SetVisibility(false, true);
	FriendlyMesh->SetVisibility(false);
	ApplyHitReaction(0.0f);
}

void ADoorSlot::ApplyBodyScale()
{
	HostileMesh->SetRelativeScale3D(FVector(HostileBodyScale));
	FriendlyMesh->SetRelativeScale3D(FVector(FriendlyBodyScale));
}

void ADoorSlot::PoseHostileReady()
{
	if (!HostileDrawAnimation)
	{
		PlayBodyLoop(HostileMesh, HostileAimAnimation);
		return;
	}

	HostileMesh->PlayAnimation(HostileDrawAnimation, false);
	HostileMesh->SetPlayRate(0.0f);
	HostileMesh->SetPosition(0.0f, false);
}

void ADoorSlot::StartHostileDraw()
{
	if (HostileDrawAnimation && Params.TelegraphDuration > 0.0f)
	{
		HostileMesh->SetPlayRate(HostileDrawAnimation->GetPlayLength() / Params.TelegraphDuration);
	}
}

void ADoorSlot::PlayBodyLoop(USkeletalMeshComponent* Body, UAnimSequenceBase* Animation)
{
	if (Animation)
	{
		Body->PlayAnimation(Animation, true);
		Body->SetPlayRate(1.0f);
	}
}

USkeletalMeshComponent* ADoorSlot::GetShownBody() const
{
	switch (Params.Occupant)
	{
	case EDoorOccupant::Hostile:
		return HostileMesh;
	case EDoorOccupant::Friendly:
		return FriendlyMesh;
	default:
		return nullptr;
	}
}

void ADoorSlot::ApplyHitReaction(float ReactionAlpha)
{
	// without a hit animation the occupant tips backwards away from the player, the root sits at floor level so it falls over its feet
	const float Pitch = HitAnimation ? 0.0f : 80.0f * ReactionAlpha;
	OccupantRoot->SetRelativeRotation(FRotator(Pitch, 0.0f, 0.0f));
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
