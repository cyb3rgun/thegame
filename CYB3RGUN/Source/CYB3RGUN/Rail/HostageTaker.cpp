// CYB3RGUN THEGAME. A rail set piece: a hostile machine holding a hostage in front of it.

#include "HostageTaker.h"
#include "StyleScoringComponent.h"
#include "StyleSettings.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogHostageTaker, Log, All);

namespace HostageTakerParts
{
	/** The volumes below wrap the hostile mannequin at the door range's hostile size, the taker root scales them with the body */
	constexpr float ReferenceScale = 1.05f;

	/** Size of the hostage, the door range's friendly size its volumes are made for */
	constexpr float HostageScale = 0.9f;

	/** A hit volume: an engine shape that is never drawn, only its collision counts */
	UStaticMeshComponent* MakeVolume(AActor* Owner, USceneComponent* Parent, const TCHAR* Name, UStaticMesh* Mesh, const FVector& Location, const FVector& Scale)
	{
		UStaticMeshComponent* Component = Owner->CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Component->SetupAttachment(Parent);
		Component->SetStaticMesh(Mesh);
		Component->SetRelativeLocation(Location);
		Component->SetRelativeScale3D(Scale);
		Component->SetCollisionProfileName(FName("BlockAll"));
		Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Component->SetGenerateOverlapEvents(false);
		Component->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
		Component->SetVisibility(false);
		Component->SetHiddenInGame(true);
		Component->SetCastShadow(false);
		return Component;
	}

	USkeletalMeshComponent* MakeBody(AActor* Owner, USceneComponent* Parent, const TCHAR* Name, USkeletalMesh* Mesh, float Scale)
	{
		USkeletalMeshComponent* Body = Owner->CreateDefaultSubobject<USkeletalMeshComponent>(Name);
		Body->SetupAttachment(Parent);
		Body->SetSkeletalMeshAsset(Mesh);
		// the mannequin faces +Y, the set piece faces the rider along +X
		Body->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
		Body->SetRelativeScale3D(FVector(Scale));
		Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Body->SetGenerateOverlapEvents(false);
		Body->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
		Body->SetAnimationMode(EAnimationMode::AnimationSingleNode);
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
		}
	}
}

AHostageTaker::AHostageTaker()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> TakerBodyMesh(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> HostageBodyMesh(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple"));
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> PistolMesh(TEXT("/Game/Weapons/Pistol/Meshes/SKM_Pistol.SKM_Pistol"));
	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> AimAnim(TEXT("/Game/Characters/Mannequins/Anims/Pistol/MF_Pistol_Idle_ADS.MF_Pistol_Idle_ADS"));
	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> IdleAnim(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle.MM_Idle"));
	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> HitAnim(TEXT("/Game/Characters/Mannequins/Anims/Death/MM_Death_Back_01.MM_Death_Back_01"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> TakerLook(TEXT("/Game/CYB3RGUN/Enemies/Bodies/MI_Body_Hostile.MI_Body_Hostile"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> HostageLook(TEXT("/Game/CYB3RGUN/Enemies/Bodies/MI_Body_Friendly.MI_Body_Friendly"));
	TakerAimAnimation = AimAnim.Object;
	HostageIdleAnimation = IdleAnim.Object;
	HitAnimation = HitAnim.Object;
	TakerMaterial = TakerLook.Object;
	HostageMaterial = HostageLook.Object;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Group = CreateDefaultSubobject<USceneComponent>(TEXT("Group"));
	Group->SetupAttachment(Root);

	// the taker: pistol in the right hand, volumes around the aiming pose like the door range's hostile
	TakerRoot = CreateDefaultSubobject<USceneComponent>(TEXT("TakerRoot"));
	TakerRoot->SetupAttachment(Group);
	TakerMesh = HostageTakerParts::MakeBody(this, TakerRoot, TEXT("TakerMesh"), TakerBodyMesh.Object, HostageTakerParts::ReferenceScale);
	TakerWeapon = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("TakerWeapon"));
	TakerWeapon->SetupAttachment(TakerMesh, FName("HandGrip_R"));
	TakerWeapon->SetSkeletalMeshAsset(PistolMesh.Object);
	TakerWeapon->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TakerWeapon->SetGenerateOverlapEvents(false);
	TakerBody = HostageTakerParts::MakeVolume(this, TakerRoot, TEXT("TakerBody"), CylinderMesh.Object, FVector(0.0f, 0.0f, 78.0f), FVector(0.42f, 0.42f, 1.56f));
	TakerArms = HostageTakerParts::MakeVolume(this, TakerRoot, TEXT("TakerArms"), CubeMesh.Object, FVector(30.0f, 0.0f, 148.0f), FVector(0.6f, 0.3f, 0.16f));
	TakerHead = HostageTakerParts::MakeVolume(this, TakerRoot, TEXT("TakerHead"), SphereMesh.Object, FVector(0.0f, 0.0f, 176.0f), FVector(0.3f, 0.3f, 0.32f));

	// the hostage in front: the smaller mannequin, empty hands
	HostageMesh = HostageTakerParts::MakeBody(this, Group, TEXT("HostageMesh"), HostageBodyMesh.Object, HostageTakerParts::HostageScale);
	HostageBody = HostageTakerParts::MakeVolume(this, Group, TEXT("HostageBody"), CylinderMesh.Object, FVector(0.0f, 0.0f, 65.0f), FVector(0.4f, 0.4f, 1.3f));
	HostageHead = HostageTakerParts::MakeVolume(this, Group, TEXT("HostageHead"), SphereMesh.Object, FVector(0.0f, 0.0f, 145.0f), FVector(0.28f, 0.28f, 0.3f));

	ApplyLayout();
}

void AHostageTaker::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	ApplyLayout();
	HostageTakerParts::PaintBody(TakerMesh, TakerMaterial);
	HostageTakerParts::PaintBody(HostageMesh, HostageMaterial);

	// in the editor the pair stands where it ends up, so the set piece can be placed by eye
	const bool bEditorPreview = GetWorld() && !GetWorld()->IsGameWorld();
	TakerMesh->SetVisibility(bEditorPreview, true);
	HostageMesh->SetVisibility(bEditorPreview);
}

void AHostageTaker::BeginPlay()
{
	Super::BeginPlay();

	ApplyLayout();
	HostageTakerParts::PaintBody(TakerMesh, TakerMaterial);
	HostageTakerParts::PaintBody(HostageMesh, HostageMaterial);
	Group->SetRelativeLocation(HiddenOffset);
	SetShown(false);
}

void AHostageTaker::ApplyLayout()
{
	TakerRoot->SetRelativeLocation(TakerOffset);
	TakerRoot->SetRelativeScale3D(FVector(TakerBodyScale / HostageTakerParts::ReferenceScale));
}

void AHostageTaker::SetShown(bool bShown)
{
	TakerMesh->SetVisibility(bShown, true);
	HostageMesh->SetVisibility(bShown);
	SetVolumesActive(bShown);
}

void AHostageTaker::SetVolumesActive(bool bActiveVolumes)
{
	for (UStaticMeshComponent* Volume : { TakerBody, TakerArms, TakerHead, HostageBody, HostageHead })
	{
		Volume->SetCollisionEnabled(bActiveVolumes ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	}
}

void AHostageTaker::Activate()
{
	// a resolved set piece can be revealed again, for a restarted ride
	if (bActive && !bResolved)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(OutcomeTimer);

	bActive = true;
	bResolved = false;
	bTakerDown = false;
	bHostageDown = false;
	RevealAlpha = 0.0f;
	RevealDirection = 1.0f;
	PendingOutcome = EHostageOutcome::Escaped;

	Group->SetRelativeLocation(HiddenOffset);
	SetShown(true);
	if (TakerAimAnimation)
	{
		TakerMesh->PlayAnimation(TakerAimAnimation, true);
	}
	if (HostageIdleAnimation)
	{
		HostageMesh->PlayAnimation(HostageIdleAnimation, true);
	}
	SetActorTickEnabled(true);

	const float Window = RevealSeconds + TimeLimit;
	GetWorldTimerManager().SetTimer(TimeLimitTimer, this, &AHostageTaker::HandleTimeLimit, Window, false);
	UE_LOG(LogHostageTaker, Log, TEXT("%s steps out with its hostage, %.1f s to free it"), *GetName(), Window);
}

void AHostageTaker::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	RevealAlpha = FMath::Clamp(RevealAlpha + RevealDirection * DeltaSeconds / FMath::Max(RevealSeconds, 0.01f), 0.0f, 1.0f);
	Group->SetRelativeLocation(FMath::Lerp(HiddenOffset, FVector::ZeroVector, FMath::InterpEaseOut(0.0f, 1.0f, RevealAlpha, 2.0f)));

	if (RevealDirection > 0.0f && RevealAlpha >= 1.0f)
	{
		SetActorTickEnabled(false);
	}
	else if (RevealDirection < 0.0f && RevealAlpha <= 0.0f)
	{
		SetShown(false);
		SetActorTickEnabled(false);
	}
}

FVector AHostageTaker::GetTakerAimPoint() const
{
	// the side of the taker's head that shows past the hostage, half its radius out from the centre
	const FVector Side = GetActorRightVector() * FMath::Sign(TakerOffset.Y);
	return TakerHead->GetComponentLocation() + Side * TakerHead->Bounds.BoxExtent.X * 0.5f;
}

FVector AHostageTaker::GetHostageAimPoint() const
{
	return HostageBody->GetComponentLocation() + FVector(0.0f, 0.0f, 35.0f);
}

bool AHostageTaker::NotifyShot(UPrimitiveComponent* HitComponent, const FVector& HitLocation, AController* InstigatedBy)
{
	if (!bActive || bResolved || !HitComponent)
	{
		return false;
	}

	UStyleScoringComponent* Style = UStyleScoringComponent::ForController(InstigatedBy);

	// the hostage counts whenever it is hit, a freed hostage still stands in the line of fire until the set piece ends
	if (HitComponent == HostageBody || HitComponent == HostageHead)
	{
		if (bHostageDown)
		{
			return false;
		}

		bHostageDown = true;
		PlayHitOn(HostageMesh);
		if (Style)
		{
			Style->RecordNonTargetHit(this);
		}
		UE_LOG(LogHostageTaker, Log, TEXT("%s: the hostage was hit"), *GetName());
		ScheduleOutcome(EHostageOutcome::HostageHit);
		return true;
	}

	if (HitComponent != TakerBody && HitComponent != TakerArms && HitComponent != TakerHead)
	{
		return false;
	}

	// with the hostage down there is nothing left to save
	if (bHostageDown)
	{
		return false;
	}

	// a second shot on a taker that is already going down still reaches the style record and can complete a pair
	if (bTakerDown)
	{
		const bool bFollowUp = GetWorld()->GetTimeSeconds() - TakerDownAt <= UStyleSettings::Get(this)->ControlledPairWindow;
		if (bFollowUp && Style)
		{
			Style->RecordTargetHit(this, false, false);
		}
		return bFollowUp;
	}

	// whatever shows of the taker beside its hostage is the zone: it goes down at once and the hostage is free
	bTakerDown = true;
	TakerDownAt = GetWorld()->GetTimeSeconds();
	PlayHitOn(TakerMesh);
	if (Style)
	{
		Style->RecordTargetHit(this, HitComponent == TakerHead, true);
		Style->RecordRescue(this);
	}
	UE_LOG(LogHostageTaker, Log, TEXT("%s is down%s, the hostage is free"), *GetName(), HitComponent == TakerHead ? TEXT(" with a head hit") : TEXT(""));
	ScheduleOutcome(EHostageOutcome::Rescued);
	return true;
}

void AHostageTaker::PlayHitOn(USkeletalMeshComponent* Body)
{
	if (Body && HitAnimation)
	{
		Body->PlayAnimation(HitAnimation, false);
		Body->SetPlayRate(HitAnimationRate);
	}
}

void AHostageTaker::ScheduleOutcome(EHostageOutcome Outcome)
{
	FTimerManager& Timers = GetWorldTimerManager();
	Timers.ClearTimer(TimeLimitTimer);

	if (!Timers.IsTimerActive(OutcomeTimer))
	{
		PendingOutcome = Outcome;
		Timers.SetTimer(OutcomeTimer, this, &AHostageTaker::ReportOutcome, FMath::Max(OutcomeHoldSeconds, 0.01f), false);
	}
	else if (Outcome == EHostageOutcome::HostageHit)
	{
		PendingOutcome = Outcome;
	}
}

void AHostageTaker::ReportOutcome()
{
	bResolved = true;

	// the bodies stay where they fell, only their volumes stop catching shots
	SetVolumesActive(false);

	UE_LOG(LogHostageTaker, Log, TEXT("%s resolved: %s"), *GetName(), *StaticEnum<EHostageOutcome>()->GetNameStringByValue(static_cast<int64>(PendingOutcome)));
	OnResolved.Broadcast(this, PendingOutcome);
}

void AHostageTaker::HandleTimeLimit()
{
	if (bTakerDown || bHostageDown)
	{
		return;
	}

	// nobody fired in time: the taker backs away with its hostage the way it came, out of reach of any shot
	UE_LOG(LogHostageTaker, Log, TEXT("%s got away with its hostage"), *GetName());
	SetVolumesActive(false);
	RevealDirection = -1.0f;
	SetActorTickEnabled(true);
	PendingOutcome = EHostageOutcome::Escaped;
	GetWorldTimerManager().SetTimer(OutcomeTimer, this, &AHostageTaker::ReportOutcome, FMath::Max(RevealSeconds, 0.01f), false);
}
