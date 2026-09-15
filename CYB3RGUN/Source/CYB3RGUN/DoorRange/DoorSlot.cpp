// CYB3RGUN THEGAME. One door of the door module.

#include "DoorSlot.h"
#include "HitReactions.h"
#include "HitZoneSettings.h"
#include "StandInBody.h"
#include "TargetAnimInstance.h"
#include "ThreatSubsystem.h"
#include "StyleScoringComponent.h"
#include "StyleSettings.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/HitResult.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogDoorSlot, Log, All);

namespace DoorSlotParts
{
	/** Socket of the hostile's right hand the pistol sits in */
	const FName WeaponSocket(TEXT("HandGrip_R"));

	/** Bones the aim points sit on */
	const FName HeadBone(TEXT("head"));
	const FName ChestBone(TEXT("spine_03"));

	/** Material slots of the panel model */
	const FName WoodSlot(TEXT("Wood"));
	const FName MetalSlot(TEXT("Metal"));

	/** Hit volumes of slots saved before the physics asset bodies became the hit targets */
	const TCHAR* const StaleVolumes[] = { TEXT("HostileBody"), TEXT("HostileArms"), TEXT("HostileHead"), TEXT("FriendlyBody"), TEXT("FriendlyHead") };

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

	USkeletalMeshComponent* MakeBody(AActor* Owner, USceneComponent* Parent, const TCHAR* Name, USkeletalMesh* Mesh)
	{
		USkeletalMeshComponent* Body = Owner->CreateDefaultSubobject<USkeletalMeshComponent>(Name);
		Body->SetupAttachment(Parent);
		Body->SetSkeletalMeshAsset(Mesh);
		// the mannequin faces +Y, the slot faces the player along +X
		Body->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
		// the physics asset bodies are the hit target, switched on while the occupant shows
		FHitReactions::SetupBodyTarget(Body);
		// a native anim instance, so hit reactions blend over the base animation
		Body->SetAnimInstanceClass(UTargetAnimInstance::StaticClass());
		Body->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
		Body->SetVisibility(false);
		return Body;
	}

	void PaintBody(USkeletalMeshComponent* Body, UMaterialInterface* Material)
	{
		if (Body && Material)
		{
			for (int32 Index = 0; Index < Body->GetNumMaterials(); ++Index)
			{
				Body->SetMaterial(Index, Material);
			}
			FStandInBody::Paint(Body, Material);
		}
	}
}

ADoorSlot::ADoorSlot()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> FrameModel(TEXT("/Game/CYB3RGUN/Core/Models/SM_DoorFrame.SM_DoorFrame"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PanelModel(TEXT("/Game/CYB3RGUN/Core/Models/SM_DoorPanel.SM_DoorPanel"));
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> PistolMesh(TEXT("/Game/Weapons/Pistol/Meshes/SKM_Pistol.SKM_Pistol"));
	// character models and their animations may be absent with the closed tier, the bodies then stand in with primitives (D-068)
	USkeletalMesh* const HostileBodyMesh = FStandInBody::LoadOptional<USkeletalMesh>(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
	USkeletalMesh* const FriendlyBodyMesh = FStandInBody::LoadOptional<USkeletalMesh>(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> FrameLook(TEXT("/Game/CYB3RGUN/Core/Materials/MI_Door_Frame.MI_Door_Frame"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> PanelLook(TEXT("/Game/CYB3RGUN/Core/Materials/MI_Door_PanelModel.MI_Door_PanelModel"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> HardwareLook(TEXT("/Game/CYB3RGUN/Core/Materials/MI_Door_Hardware.MI_Door_Hardware"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> WallLook(TEXT("/Game/CYB3RGUN/Core/Materials/MI_Wall_Alcove.MI_Wall_Alcove"));
	FrameMaterial = FrameLook.Object;
	PanelMaterial = PanelLook.Object;
	HardwareMaterial = HardwareLook.Object;
	WallMaterial = WallLook.Object;
	HostileDrawAnimation = FStandInBody::LoadOptional<UAnimSequenceBase>(TEXT("/Game/Characters/Mannequins/Anims/Pistol/MM_Pistol_Equip.MM_Pistol_Equip"));
	HostileAimAnimation = FStandInBody::LoadOptional<UAnimSequenceBase>(TEXT("/Game/Characters/Mannequins/Anims/Pistol/MF_Pistol_Idle_ADS.MF_Pistol_Idle_ADS"));
	FriendlyIdleAnimation = FStandInBody::LoadOptional<UAnimSequenceBase>(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle.MM_Idle"));
	HitAnimation = FStandInBody::LoadOptional<UAnimSequenceBase>(TEXT("/Game/Characters/Mannequins/Anims/Death/MM_Death_Back_01.MM_Death_Back_01"));

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	// The actor's forward (+X) faces the player. The door plane is the YZ plane at X = 0. The frame model has a 100 by 200 cm
	// opening, 15 cm deep jambs and its pivot on the floor in the door plane. The wall behind is a basic cube, scaled in metres.
	Frame = DoorSlotParts::MakeShape(this, Root, TEXT("Frame"), FrameModel.Object, FVector::ZeroVector, FVector::OneVector);
	BackWall = DoorSlotParts::MakeShape(this, Root, TEXT("BackWall"), CubeMesh.Object, FVector(-130.0f, 0.0f, 110.0f), FVector(0.1f, 1.6f, 2.4f));

	// the panel hangs on the front edge of the left jamb, closes against the stop in the opening and swings clear of the casing
	Hinge = CreateDefaultSubobject<USceneComponent>(TEXT("Hinge"));
	Hinge->SetupAttachment(Root);
	Hinge->SetRelativeLocation(FVector(7.5f, -50.0f, 0.0f));

	// the panel model has its pivot in its centre, the hinge edge at -50 cm and the knuckles 3 cm in front of its centre plane
	DoorPanel = DoorSlotParts::MakeShape(this, Hinge, TEXT("DoorPanel"), PanelModel.Object, FVector(-3.0f, 50.0f, 100.0f), FVector::OneVector);

	SpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("SpawnPoint"));
	SpawnPoint->SetupAttachment(Root);
	SpawnPoint->SetRelativeLocation(FVector(-60.0f, 0.0f, 0.0f));

	OccupantRoot = CreateDefaultSubobject<USceneComponent>(TEXT("OccupantRoot"));
	OccupantRoot->SetupAttachment(SpawnPoint);

	HostileRoot = CreateDefaultSubobject<USceneComponent>(TEXT("HostileRoot"));
	HostileRoot->SetupAttachment(OccupantRoot);

	// Hostile: the taller mannequin with a pistol in the right hand. Shots land on its physics asset
	// bodies, and on the pistol while it can shoot.
	HostileMesh = DoorSlotParts::MakeBody(this, HostileRoot, TEXT("HostileMesh"), HostileBodyMesh);
	HostileWeapon = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("HostileWeapon"));
	HostileWeapon->SetupAttachment(HostileMesh, DoorSlotParts::WeaponSocket);
	HostileWeapon->SetSkeletalMeshAsset(PistolMesh.Object);
	HostileWeapon->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HostileWeapon->SetGenerateOverlapEvents(false);
	HostileWeapon->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
	HostileWeapon->SetVisibility(false);

	// Friendly: the smaller mannequin, empty hands, at ease.
	FriendlyMesh = DoorSlotParts::MakeBody(this, OccupantRoot, TEXT("FriendlyMesh"), FriendlyBodyMesh);

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
	if (FrameMaterial)
	{
		Frame->SetMaterial(0, FrameMaterial);
	}
	if (PanelMaterial)
	{
		DoorPanel->SetMaterialByName(DoorSlotParts::WoodSlot, PanelMaterial);
	}
	if (HardwareMaterial)
	{
		DoorPanel->SetMaterialByName(DoorSlotParts::MetalSlot, HardwareMaterial);
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
	SetupTargets();

	HideOccupant();
	ApplyPanelAlpha(0.0f);
	SetState(EDoorState::Closed);
}

void ADoorSlot::SetupTargets()
{
	DisableStaleHitVolumes();

	// placed slots keep whatever their level saved, so the setup is made again here
	for (USkeletalMeshComponent* Body : { HostileMesh, FriendlyMesh })
	{
		UTargetAnimInstance::Ensure(Body);
		FHitReactions::SetupBodyTarget(Body);
	}
	FStandInBody::Ensure(HostileMesh, TEXT("the door range hostile"));
	FStandInBody::Ensure(FriendlyMesh, TEXT("the door range friendly"));
}

void ADoorSlot::DisableStaleHitVolumes()
{
	TInlineComponentArray<UStaticMeshComponent*> Shapes(this);
	for (UStaticMeshComponent* Shape : Shapes)
	{
		for (const TCHAR* Stale : DoorSlotParts::StaleVolumes)
		{
			if (Shape && Shape->GetFName() == FName(Stale))
			{
				Shape->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				UE_LOG(LogDoorSlot, Warning, TEXT("%s still carries the old hit volume %s, it no longer catches shots"), *GetName(), Stale);
			}
		}
	}
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
	bDisarmed = false;
	bTipOver = false;
	bDrawn = false;
	DrawnAt = 0.0f;
	HitReactionElapsed = 0.0f;
	bReportNextClose = true;
	bTakerDown = false;
	bHostageDown = false;
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
	else if (Params.Occupant == EDoorOccupant::HostageTaker)
	{
		// the taker already aims past its hostage, there is no draw to wait for
		PlayBodyLoop(HostileMesh, HostileAimAnimation);
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
	// a hit without a shot behind it has no zone, the body falls as it always has
	return RegisterZoneHit(FHitZoneResult());
}

bool ADoorSlot::RegisterZoneHit(const FHitZoneResult& Zone)
{
	if (State == EDoorState::Closed || Params.Occupant == EDoorOccupant::Empty || bHitRegistered || bDisarmed || !bDrawn)
	{
		return false;
	}

	// a hostage taker's door only knows who was hit from the shot itself, see NotifyHostageShot
	if (Params.Occupant == EDoorOccupant::HostageTaker)
	{
		return false;
	}

	bHitRegistered = true;
	HitRegisteredAt = GetWorld()->GetTimeSeconds();
	HitReactionElapsed = 0.0f;
	UpdateWeaponTarget();

	// any accepted hit brings a hostile down, the zone only decides how it goes; a friendly falls or flinches
	if (Params.Occupant == EDoorOccupant::Hostile)
	{
		PlayDown(HostileMesh, Zone);
	}
	else
	{
		PlayInjured(GetShownBody(), Zone);
	}

	const float ExposureFraction = GetExposureFraction();
	OnSlotHit.Broadcast(this, Params.Occupant, Zone.Zone, ExposureFraction);

	CloseAfterHit();
	return true;
}

void ADoorSlot::DisarmHostile(const FHitZoneResult& Zone, const FHitResult& Hit, const FVector& ShotDirection)
{
	// the pistol flies: knocked away along the shot when it was hit, falling from the hand when the arm was
	FHitReactions::DropWeapon(HostileWeapon, ShotDirection, Hit.ImpactPoint, Zone.Zone == EHitZone::Weapon);

	// the hostile jerks with the hit, a flinch for the pistol and the arm for the arm, then stands empty handed
	FHitReactions::PlayReaction(HostileMesh, Zone.GetReaction(), Zone.Zone, Zone.Direction);
	if (!FHitReactions::PlayDisarmedIdle(HostileMesh))
	{
		PlayBodyLoop(HostileMesh, FriendlyIdleAnimation, UHitZoneSettings::Get()->DisarmedBlend);
	}
	UpdateWeaponTarget();
}

void ADoorSlot::PlayDown(USkeletalMeshComponent* Body, const FHitZoneResult& Zone)
{
	// a leg staggers and an arm flinches before the body falls, a head or torso hit falls at once
	const bool bLimb = Zone.Zone == EHitZone::Leg || Zone.Zone == EHitZone::OffArm || Zone.Zone == EHitZone::WeaponArm;
	const EHitDirection Direction = Zone.Direction;
	if (bLimb)
	{
		TWeakObjectPtr<ADoorSlot> WeakSlot(this);
		TWeakObjectPtr<USkeletalMeshComponent> WeakBody(Body);
		const bool bReacting = FHitReactions::PlayReaction(Body, Zone.GetReaction(), Zone.Zone, Direction, [WeakSlot, WeakBody, Direction]()
		{
			// the door may have shut while the reaction ran
			ADoorSlot* Slot = WeakSlot.Get();
			if (Slot && Slot->State != EDoorState::Closed)
			{
				Slot->PlayFallOn(WeakBody.Get(), Direction);
			}
		});
		if (bReacting)
		{
			return;
		}
	}
	PlayFallOn(Body, Direction);
}

void ADoorSlot::PlayInjured(USkeletalMeshComponent* Body, const FHitZoneResult& Zone)
{
	// the penalty is the same wherever it lands; a killing zone drops the innocent, any other only makes it react
	const EHitReaction Reaction = Zone.GetReaction();
	if (Reaction != EHitReaction::None && Reaction != EHitReaction::Kill && FHitReactions::PlayReaction(Body, Reaction, Zone.Zone, Zone.Direction))
	{
		return;
	}
	PlayFallOn(Body, Zone.Direction);
}

void ADoorSlot::PlayFallOn(USkeletalMeshComponent* Body, EHitDirection Direction)
{
	if (Body && !FHitReactions::PlayDeath(Body, Direction))
	{
		PlayHitOn(Body);
	}
}

void ADoorSlot::PlayHitOn(USkeletalMeshComponent* Body)
{
	// the door's own fall; without one the whole occupant tips over
	if (!FHitReactions::PlayFall(Body, HitAnimation, HitAnimationRate))
	{
		bTipOver = true;
		HitReactionElapsed = 0.0f;
	}
}

EHitZone ADoorSlot::GetFollowUpZone(const FHitZoneResult& Zone) const
{
	// another pellet of the shot that already counted can still show the style record a better zone, a later shot cannot
	return GetWorld()->GetTimeSeconds() == HitRegisteredAt ? Zone.Zone : EHitZone::None;
}

void ADoorSlot::CloseAfterHit()
{
	if (GetWorldTimerManager().IsTimerActive(HitCloseTimer))
	{
		return;
	}

	// the occupant reacts and falls, and the door swings shut once the reaction has shown and the controlled pair
	// window has passed, so the second shot of a pair is not stopped by the closing panel
	const float CloseDelay = FMath::Max(UStyleSettings::Get(this)->ControlledPairWindow, UHitZoneSettings::Get()->DoorHoldAfterHit);
	if (CloseDelay > 0.0f)
	{
		GetWorldTimerManager().SetTimer(HitCloseTimer, FTimerDelegate::CreateWeakLambda(this, [this]() { ForceClose(); }), CloseDelay, false);
	}
	else
	{
		ForceClose();
	}
}

void ADoorSlot::ApplyHostileLayout(bool bTaker)
{
	// a hostage taker is the hostile body brought down to the hostage's size and moved behind it, pistol and all
	const float Scale = bTaker ? TakerBodyScale / FMath::Max(HostileBodyScale, 0.01f) : 1.0f;
	HostileRoot->SetRelativeLocation(bTaker ? TakerOffset : FVector::ZeroVector);
	HostileRoot->SetRelativeScale3D(FVector(Scale));
}

int32 ADoorSlot::RepairHostileSet(bool bFix)
{
	// slots placed in a level before the hostile root existed saved their parts on the occupant root. DoorRange.RepairSlots fix
	// moves them in the editor so the level is saved right; nothing repairs them at run time. The pistol rides on the body.
	USceneComponent* const Parts[] = { HostileMesh };
	int32 Stale = 0;
	for (USceneComponent* Part : Parts)
	{
		if (Part && Part->GetAttachParent() != HostileRoot)
		{
			++Stale;
			if (bFix)
			{
				Modify();
				Part->Modify();
				Part->AttachToComponent(HostileRoot, FAttachmentTransformRules::KeepRelativeTransform);
			}
		}
	}
	return Stale;
}

void ADoorSlot::ReportThreat() const
{
	// the logo reticle tightens over the draw, is fully tight while the hostile can shoot and lets go as the door shuts (D-048).
	// A disarmed hostile is no threat any more
	if (bHitRegistered || bDisarmed || (Params.Occupant != EDoorOccupant::Hostile && Params.Occupant != EDoorOccupant::HostageTaker))
	{
		return;
	}

	float Level = 0.0f;
	if (State == EDoorState::Showing)
	{
		Level = bDrawn ? 1.0f : (Params.TelegraphDuration > 0.0f ? FMath::Clamp(StateElapsed / Params.TelegraphDuration, 0.0f, 1.0f) : 1.0f);
	}
	else if (State == EDoorState::Closing && ClosingStartAlpha > 0.0f)
	{
		Level = FMath::Clamp(PanelAlpha / ClosingStartAlpha, 0.0f, 1.0f);
	}

	if (Level > 0.0f)
	{
		if (UThreatSubsystem* Threats = UThreatSubsystem::Get(this))
		{
			Threats->ReportThreat(this, Level);
		}
	}
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
	// a hostage taker is only hit where it shows past its hostage
	if (Params.Occupant == EDoorOccupant::HostageTaker)
	{
		return GetOccupantHeadPoint();
	}

	// the chest of the body that shows
	const USkeletalMeshComponent* Body = Params.Occupant == EDoorOccupant::Friendly ? FriendlyMesh : HostileMesh;
	FVector Point;
	return FHitReactions::GetBonePoint(Body, DoorSlotParts::ChestBone, Point) ? Point : SpawnPoint->GetComponentLocation();
}

FVector ADoorSlot::GetOccupantHeadPoint() const
{
	const USkeletalMeshComponent* Body = Params.Occupant == EDoorOccupant::Friendly ? FriendlyMesh : HostileMesh;
	FVector Head;
	if (!FHitReactions::GetBonePoint(Body, DoorSlotParts::HeadBone, Head))
	{
		Head = Body->GetComponentLocation();
	}

	// the side of the taker's head that shows past the hostage
	if (Params.Occupant == EDoorOccupant::HostageTaker)
	{
		Head += GetActorRightVector() * FMath::Sign(TakerOffset.Y) * TakerAimSideOffset;
	}
	return Head;
}

FVector ADoorSlot::GetHostageAimPoint() const
{
	FVector Point;
	return FHitReactions::GetBonePoint(FriendlyMesh, DoorSlotParts::ChestBone, Point) ? Point : FriendlyMesh->GetComponentLocation();
}

bool ADoorSlot::GetOccupantBonePoint(FName Bone, bool bHostage, FVector& OutPoint) const
{
	const bool bFriendlyBody = bHostage || Params.Occupant == EDoorOccupant::Friendly;
	return FHitReactions::GetBonePoint(bFriendlyBody ? FriendlyMesh : HostileMesh, Bone, OutPoint);
}

bool ADoorSlot::GetOccupantWeaponPoint(FVector& OutPoint) const
{
	const bool bArmed = Params.Occupant == EDoorOccupant::Hostile || Params.Occupant == EDoorOccupant::HostageTaker;
	return bArmed && FHitReactions::GetWeaponPoint(HostileWeapon, OutPoint);
}

bool ADoorSlot::NotifyShot(const FHitResult& InHit, const FVector& ShotDirection, AController* InstigatedBy)
{
	// a shot on a primitive stand in counts on the body it stands in for (D-068)
	const FHitResult Hit = FStandInBody::Redirect(InHit);

	// only the occupant counts, hits on the frame or the panel are ignored
	if (!IsOccupantComponent(Hit.GetComponent()))
	{
		return false;
	}

	UStyleScoringComponent* Style = UStyleScoringComponent::ForController(InstigatedBy);
	const FHitZoneResult Zone = UHitZoneSettings::Resolve(Hit, ShotDirection);
	UE_LOG(LogDoorSlot, Verbose, TEXT("%s: shot on %s, bone %s, zone %s, from the %s"), *GetName(), *GetNameSafe(Hit.GetComponent()), *Hit.BoneName.ToString(),
		*StaticEnum<EHitZone>()->GetNameStringByValue(static_cast<int64>(Zone.Zone)), *StaticEnum<EHitDirection>()->GetNameStringByValue(static_cast<int64>(Zone.Direction)));

	if (Params.Occupant == EDoorOccupant::HostageTaker)
	{
		return NotifyHostageShot(Hit, ShotDirection, Zone, Style);
	}

	// a second shot on a hostile that is already going down, or disarmed, scores nothing on the door, but it is a hit
	// for the style record and can complete a controlled pair
	if (bHitRegistered || bDisarmed)
	{
		const bool bFollowUp = Params.Occupant == EDoorOccupant::Hostile && GetWorld()->GetTimeSeconds() - HitRegisteredAt <= UStyleSettings::Get(this)->ControlledPairWindow;
		if (bFollowUp && Style)
		{
			Style->RecordTargetHit(this, GetFollowUpZone(Zone), false);
		}
		return bFollowUp;
	}

	// the pistol, or the arm holding it: the hostile loses the pistol and stays up, out of the fight without a kill (D-050)
	if (Params.Occupant == EDoorOccupant::Hostile && Zone.IsDisarm())
	{
		if (State == EDoorState::Closed || !bDrawn)
		{
			return false;
		}

		bDisarmed = true;
		HitRegisteredAt = GetWorld()->GetTimeSeconds();
		DisarmHostile(Zone, Hit, ShotDirection);
		if (Style)
		{
			Style->RecordDisarm(this, Zone.Zone == EHitZone::Weapon);
		}
		OnSlotDisarmed.Broadcast(this, Zone.Zone, GetExposureFraction());
		CloseAfterHit();
		return true;
	}

	const EDoorOccupant Occupant = Params.Occupant;
	if (!RegisterZoneHit(Zone))
	{
		return false;
	}

	if (Style)
	{
		if (Occupant == EDoorOccupant::Hostile)
		{
			Style->RecordTargetHit(this, Zone.Zone, true);
		}
		else if (Occupant == EDoorOccupant::Friendly)
		{
			Style->RecordNonTargetHit(this);
		}
	}
	return true;
}

bool ADoorSlot::NotifyHostageShot(const FHitResult& Hit, const FVector& ShotDirection, const FHitZoneResult& Zone, UStyleScoringComponent* Style)
{
	if (State == EDoorState::Closed || !bDrawn)
	{
		return false;
	}

	const float ExposureFraction = GetExposureFraction();
	UPrimitiveComponent* HitComponent = Hit.GetComponent();

	// the hostage's body shapes leave gaps the old volumes did not: a shot that grazes the hostage on its way to the taker counts on the hostage
	FHitResult Graze;
	const bool bOnTaker = HitComponent == HostileMesh || HitComponent == HostileWeapon;
	const bool bGrazed = bOnTaker && !bHostageDown && FHitReactions::Grazes(FriendlyMesh, Hit, ShotDirection, Graze);

	UE_LOG(LogDoorSlot, Log, TEXT("%s: hostage taker door shot on %s, zone %s%s"), *GetName(), *GetNameSafe(HitComponent),
		*StaticEnum<EHitZone>()->GetNameStringByValue(static_cast<int64>(Zone.Zone)), bGrazed ? TEXT(", grazing the hostage") : TEXT(""));

	// the hostage counts whenever it is hit: a freed hostage still stands in the line of fire until the door shuts
	if (HitComponent == FriendlyMesh || bGrazed)
	{
		if (bHostageDown)
		{
			return false;
		}

		bHostageDown = true;
		bHitRegistered = true;
		HitRegisteredAt = GetWorld()->GetTimeSeconds();
		const FHitZoneResult HostageZone = bGrazed ? UHitZoneSettings::Resolve(Graze, ShotDirection) : Zone;
		PlayInjured(FriendlyMesh, HostageZone);
		UpdateWeaponTarget();
		if (Style)
		{
			Style->RecordNonTargetHit(this);
		}
		OnSlotHit.Broadcast(this, EDoorOccupant::Friendly, HostageZone.Zone, ExposureFraction);
		CloseAfterHit();
		return true;
	}

	// with the hostage down there is nothing left to save on this door
	if (bHostageDown)
	{
		return false;
	}

	// a second shot on a taker that is already down or disarmed still reaches the style record and can complete a pair
	if (bTakerDown)
	{
		const bool bFollowUp = GetWorld()->GetTimeSeconds() - HitRegisteredAt <= UStyleSettings::Get(this)->ControlledPairWindow;
		if (bFollowUp && Style)
		{
			Style->RecordTargetHit(this, GetFollowUpZone(Zone), false);
		}
		return bFollowUp;
	}

	// whatever shows of the taker beside its hostage is the zone: the taker is out of the fight and the hostage is free
	bTakerDown = true;
	HitRegisteredAt = GetWorld()->GetTimeSeconds();
	if (Zone.IsDisarm())
	{
		// the pistol, or the arm holding it: disarmed and rescued, the taker stays up without its weapon (D-050)
		bDisarmed = true;
		DisarmHostile(Zone, Hit, ShotDirection);
		if (Style)
		{
			Style->RecordDisarm(this, Zone.Zone == EHitZone::Weapon);
			Style->RecordRescue(this);
		}
		OnSlotDisarmed.Broadcast(this, Zone.Zone, ExposureFraction);
	}
	else
	{
		bHitRegistered = true;
		PlayDown(HostileMesh, Zone);
		UpdateWeaponTarget();
		if (Style)
		{
			Style->RecordTargetHit(this, Zone.Zone, true);
			Style->RecordRescue(this);
		}
		OnSlotHit.Broadcast(this, EDoorOccupant::HostageTaker, Zone.Zone, ExposureFraction);
	}
	CloseAfterHit();
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
				UpdateWeaponTarget();
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

		// the occupant goes the moment the panel covers it, before the last part of the swing (D-076)
		if (bOccupantShown && Alpha <= OccupantCoverAlpha)
		{
			HideOccupant();
		}
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

	ReportThreat();
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
			UpdateWeaponTarget();
		}
		break;
	case EDoorState::Closing:
		ClosingStartAlpha = FMath::Max(PanelAlpha, 0.01f);
		OccupantCoverAlpha = bOccupantShown ? ComputeOccupantCoverAlpha() : 0.0f;
		break;
	case EDoorState::Closed:
	{
		ApplyPanelAlpha(0.0f);
		HideOccupant();
		SetActorTickEnabled(false);
		PlaySlotSound(Params.DoorSound, Params.DoorPitch);

		// a disarmed hostile is out of the fight like a hit one, it never counts as escaped
		const EDoorOccupant ClosedOccupant = Params.Occupant;
		const bool bWasHit = bHitRegistered || bDisarmed;
		const bool bReport = bReportNextClose;
		Params.Occupant = EDoorOccupant::Empty;
		bHitRegistered = false;
		bDisarmed = false;
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
	// negative yaw turns the panel out of the door plane towards the player, the occupant stands behind that plane (D-076)
	Hinge->SetRelativeRotation(FRotator(0.0f, -OpenAngle * Alpha, 0.0f));
}

float ADoorSlot::ComputeOccupantCoverAlpha() const
{
	// everything the occupant reaches, in the slot's own space: the panel covers a point from the front once its free edge
	// has come round past the point's sideways position
	const FTransform SlotSpace = GetActorTransform();
	float FarSide = -TNumericLimits<float>::Max();
	TInlineComponentArray<UPrimitiveComponent*> Parts;
	GetComponents(Parts);
	for (const UPrimitiveComponent* Part : Parts)
	{
		if (!Part || !Part->IsVisible() || !Part->IsAttachedTo(OccupantRoot))
		{
			continue;
		}
		const FBox Box = Part->Bounds.GetBox();
		for (int32 Corner = 0; Corner < 8; ++Corner)
		{
			const FVector World((Corner & 1) ? Box.Max.X : Box.Min.X, (Corner & 2) ? Box.Max.Y : Box.Min.Y, (Corner & 4) ? Box.Max.Z : Box.Min.Z);
			FarSide = FMath::Max(FarSide, static_cast<float>(SlotSpace.InverseTransformPosition(World).Y));
		}
	}
	if (FarSide == -TNumericLimits<float>::Max())
	{
		return 0.0f;
	}

	// the panel is hinged at the hinge's side and as long as its model is wide; a small margin covers the view not being exactly frontal
	const float HingeSide = Hinge->GetRelativeLocation().Y;
	const float PanelLength = DoorPanel->CalcLocalBounds().BoxExtent.Y * 2.0f * DoorPanel->GetRelativeScale3D().Y;
	const float Reach = (FarSide + OccupantCoverMargin - HingeSide) / FMath::Max(PanelLength, 1.0f);
	if (Reach >= 1.0f)
	{
		return 0.0f;
	}
	const float CoverAngle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Reach, -1.0f, 1.0f)));
	return FMath::Clamp(CoverAngle / FMath::Max(OpenAngle, 1.0f), 0.0f, 1.0f);
}

void ADoorSlot::ShowOccupant(EDoorOccupant Occupant, UMaterialInterface* Material)
{
	const bool bTaker = Occupant == EDoorOccupant::HostageTaker;
	const bool bHostile = Occupant == EDoorOccupant::Hostile || bTaker;
	const bool bFriendly = Occupant == EDoorOccupant::Friendly || bTaker;

	ApplyHostileLayout(bTaker);

	// a body that lost its character model since play began stands in as well (D-068)
	FStandInBody::Ensure(HostileMesh, TEXT("the door range hostile"));
	FStandInBody::Ensure(FriendlyMesh, TEXT("the door range friendly"));

	bOccupantShown = true;

	// the bodies can be shot from the moment they show; the pistol once its holder can shoot, see UpdateWeaponTarget
	SetBodiesShootable(bHostile, bFriendly);

	HostileMesh->SetVisibility(bHostile, true);
	FriendlyMesh->SetVisibility(bFriendly, true);

	// a hostage taker wears the hostile look and its hostage the friendly one
	if (bHostile)
	{
		DoorSlotParts::PaintBody(HostileMesh, Material);
	}
	if (bFriendly)
	{
		DoorSlotParts::PaintBody(FriendlyMesh, bTaker ? Params.HostageMaterial.Get() : Material);
	}
}

void ADoorSlot::HideOccupant()
{
	// the pistol goes back into the hand first, one lying loose would not hide with the body
	FHitReactions::ResetWeapon(HostileWeapon, HostileMesh, DoorSlotParts::WeaponSocket);
	SetBodiesShootable(false, false);
	FHitReactions::StopReactions(HostileMesh);
	FHitReactions::StopReactions(FriendlyMesh);
	HostileMesh->SetVisibility(false, true);
	FriendlyMesh->SetVisibility(false, true);
	bOccupantShown = false;
	bTipOver = false;
	ApplyHitReaction(0.0f);
	ApplyHostileLayout(false);
}

void ADoorSlot::SetBodiesShootable(bool bHostile, bool bFriendly)
{
	// a body that can be shot keeps its pose fresh off screen as well, so its physics bodies follow the animation
	FHitReactions::SetBodyShootable(HostileMesh, bHostile);
	FHitReactions::SetBodyShootable(FriendlyMesh, bFriendly);
	HostileMesh->VisibilityBasedAnimTickOption = bHostile ? EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones : EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
	FriendlyMesh->VisibilityBasedAnimTickOption = bFriendly ? EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones : EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
}

void ADoorSlot::UpdateWeaponTarget()
{
	// the pistol is a target of its own while the hostile, or the hostage taker, can shoot with it
	const bool bArmed = Params.Occupant == EDoorOccupant::Hostile || Params.Occupant == EDoorOccupant::HostageTaker;
	const bool bCanShoot = bArmed && bDrawn && !bHitRegistered && !bDisarmed && !bTakerDown && !bHostageDown && State != EDoorState::Closed;
	FHitReactions::SetWeaponShootable(HostileWeapon, bCanShoot);
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

	// held on the first frame of the draw, StartHostileDraw sets it going
	if (UTargetAnimInstance* Anim = UTargetAnimInstance::FromMesh(HostileMesh))
	{
		Anim->PlayBase(HostileDrawAnimation, false, 0.0f, 0.0f);
	}
}

void ADoorSlot::StartHostileDraw()
{
	UTargetAnimInstance* Anim = UTargetAnimInstance::FromMesh(HostileMesh);
	if (Anim && HostileDrawAnimation && Params.TelegraphDuration > 0.0f)
	{
		Anim->SetBasePlayRate(HostileDrawAnimation->GetPlayLength() / Params.TelegraphDuration);
	}
}

void ADoorSlot::PlayBodyLoop(USkeletalMeshComponent* Body, UAnimSequenceBase* Animation, float BlendTime)
{
	UTargetAnimInstance* Anim = UTargetAnimInstance::FromMesh(Body);
	if (Anim && Animation)
	{
		Anim->PlayBase(Animation, true, 1.0f, 0.0f, BlendTime);
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
	case EDoorOccupant::HostageTaker:
		return HostileMesh;
	default:
		return nullptr;
	}
}

void ADoorSlot::ApplyHitReaction(float ReactionAlpha)
{
	// with nothing to fall with, the occupant tips backwards away from the player; the root sits at floor level so it falls over its feet
	const float Pitch = bTipOver ? 80.0f * ReactionAlpha : 0.0f;
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
	return Component && (Component == HostileMesh || Component == HostileWeapon || Component == FriendlyMesh);
}
