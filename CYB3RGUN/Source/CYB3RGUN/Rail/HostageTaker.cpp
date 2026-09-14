// CYB3RGUN THEGAME. A rail set piece: a hostile machine holding a hostage in front of it.

#include "HostageTaker.h"
#include "StandInBody.h"
#include "HitReactions.h"
#include "HitZoneSettings.h"
#include "TargetAnimInstance.h"
#include "StyleScoringComponent.h"
#include "StyleSettings.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/HitResult.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogHostageTaker, Log, All);

namespace HostageTakerParts
{
	/** The taker is the hostile mannequin at the door range's hostile size, the taker root scales it with TakerBodyScale */
	constexpr float ReferenceScale = 1.05f;

	/** Size of the hostage, the door range's friendly size */
	constexpr float HostageScale = 0.9f;

	/** Socket of the taker's right hand the pistol sits in */
	const FName WeaponSocket(TEXT("HandGrip_R"));

	/** Bones the aim points sit on */
	const FName HeadBone(TEXT("head"));
	const FName ChestBone(TEXT("spine_03"));

	/** Hit volumes of set pieces saved before the physics asset bodies became the hit targets */
	const TCHAR* const StaleVolumes[] = { TEXT("TakerBody"), TEXT("TakerArms"), TEXT("TakerHead"), TEXT("HostageBody"), TEXT("HostageHead") };

	USkeletalMeshComponent* MakeBody(AActor* Owner, USceneComponent* Parent, const TCHAR* Name, USkeletalMesh* Mesh, float Scale)
	{
		USkeletalMeshComponent* Body = Owner->CreateDefaultSubobject<USkeletalMeshComponent>(Name);
		Body->SetupAttachment(Parent);
		Body->SetSkeletalMeshAsset(Mesh);
		// the mannequin faces +Y, the set piece faces the rider along +X
		Body->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
		Body->SetRelativeScale3D(FVector(Scale));
		// the physics asset bodies are the hit target, switched on while the pair shows
		FHitReactions::SetupBodyTarget(Body);
		// a native anim instance, so hit reactions blend over the base animation
		Body->SetAnimInstanceClass(UTargetAnimInstance::StaticClass());
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

AHostageTaker::AHostageTaker()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> PistolMesh(TEXT("/Game/Weapons/Pistol/Meshes/SKM_Pistol.SKM_Pistol"));
	// character models and their animations may be absent with the closed tier, the bodies then stand in with primitives (D-068)
	USkeletalMesh* const TakerBodyMesh = FStandInBody::LoadOptional<USkeletalMesh>(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
	USkeletalMesh* const HostageBodyMesh = FStandInBody::LoadOptional<USkeletalMesh>(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> TakerLook(TEXT("/Game/CYB3RGUN/Enemies/Materials/MI_Body_Hostile.MI_Body_Hostile"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> HostageLook(TEXT("/Game/CYB3RGUN/Enemies/Materials/MI_Body_Friendly.MI_Body_Friendly"));
	TakerAimAnimation = FStandInBody::LoadOptional<UAnimSequenceBase>(TEXT("/Game/Characters/Mannequins/Anims/Pistol/MF_Pistol_Idle_ADS.MF_Pistol_Idle_ADS"));
	HostageIdleAnimation = FStandInBody::LoadOptional<UAnimSequenceBase>(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle.MM_Idle"));
	HitAnimation = FStandInBody::LoadOptional<UAnimSequenceBase>(TEXT("/Game/Characters/Mannequins/Anims/Death/MM_Death_Back_01.MM_Death_Back_01"));
	TakerMaterial = TakerLook.Object;
	HostageMaterial = HostageLook.Object;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Group = CreateDefaultSubobject<USceneComponent>(TEXT("Group"));
	Group->SetupAttachment(Root);

	// the taker: pistol in the right hand. Shots land on its physics asset bodies, and on the pistol while it can shoot
	TakerRoot = CreateDefaultSubobject<USceneComponent>(TEXT("TakerRoot"));
	TakerRoot->SetupAttachment(Group);
	TakerMesh = HostageTakerParts::MakeBody(this, TakerRoot, TEXT("TakerMesh"), TakerBodyMesh, HostageTakerParts::ReferenceScale);
	TakerWeapon = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("TakerWeapon"));
	TakerWeapon->SetupAttachment(TakerMesh, HostageTakerParts::WeaponSocket);
	TakerWeapon->SetSkeletalMeshAsset(PistolMesh.Object);
	TakerWeapon->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TakerWeapon->SetGenerateOverlapEvents(false);
	TakerWeapon->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;

	// the hostage in front: the smaller mannequin, empty hands
	HostageMesh = HostageTakerParts::MakeBody(this, Group, TEXT("HostageMesh"), HostageBodyMesh, HostageTakerParts::HostageScale);

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
	HostageMesh->SetVisibility(bEditorPreview, true);
}

void AHostageTaker::BeginPlay()
{
	Super::BeginPlay();

	SetupTargets();
	ApplyLayout();
	HostageTakerParts::PaintBody(TakerMesh, TakerMaterial);
	HostageTakerParts::PaintBody(HostageMesh, HostageMaterial);
	Group->SetRelativeLocation(HiddenOffset);
	SetShown(false);
}

void AHostageTaker::SetupTargets()
{
	DisableStaleHitVolumes();

	// placed set pieces keep whatever their level saved, so the setup is made again here
	for (USkeletalMeshComponent* Body : { TakerMesh, HostageMesh })
	{
		UTargetAnimInstance::Ensure(Body);
		FHitReactions::SetupBodyTarget(Body);
	}
	FStandInBody::Ensure(TakerMesh, TEXT("the rail hostage taker"));
	FStandInBody::Ensure(HostageMesh, TEXT("the rail hostage"));
}

void AHostageTaker::DisableStaleHitVolumes()
{
	TInlineComponentArray<UStaticMeshComponent*> Shapes(this);
	for (UStaticMeshComponent* Shape : Shapes)
	{
		for (const TCHAR* Stale : HostageTakerParts::StaleVolumes)
		{
			if (Shape && Shape->GetFName() == FName(Stale))
			{
				Shape->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				UE_LOG(LogHostageTaker, Warning, TEXT("%s still carries the old hit volume %s, it no longer catches shots"), *GetName(), Stale);
			}
		}
	}
}

void AHostageTaker::ApplyLayout()
{
	TakerRoot->SetRelativeLocation(TakerOffset);
	TakerRoot->SetRelativeScale3D(FVector(TakerBodyScale / HostageTakerParts::ReferenceScale));
}

void AHostageTaker::SetShown(bool bShown)
{
	// the pistol goes back into the hand first, one left lying from an earlier reveal would neither hide nor show with the body
	FHitReactions::ResetWeapon(TakerWeapon, TakerMesh, HostageTakerParts::WeaponSocket);
	FHitReactions::StopReactions(TakerMesh);
	FHitReactions::StopReactions(HostageMesh);
	TakerMesh->SetVisibility(bShown, true);
	HostageMesh->SetVisibility(bShown, true);
	SetBodiesShootable(bShown);
}

void AHostageTaker::SetBodiesShootable(bool bShootable)
{
	FHitReactions::SetBodyShootable(TakerMesh, bShootable);
	FHitReactions::SetBodyShootable(HostageMesh, bShootable);

	// the pistol only while the taker can still shoot with it
	FHitReactions::SetWeaponShootable(TakerWeapon, bShootable && !bTakerDown && !bHostageDown);
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
	bTakerDisarmed = false;
	RevealAlpha = 0.0f;
	RevealDirection = 1.0f;
	PendingOutcome = EHostageOutcome::Escaped;

	Group->SetRelativeLocation(HiddenOffset);
	SetShown(true);
	PlayLoop(TakerMesh, TakerAimAnimation);
	PlayLoop(HostageMesh, HostageIdleAnimation);
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
	// the side of the taker's head that shows past the hostage
	FVector Head;
	if (!FHitReactions::GetBonePoint(TakerMesh, HostageTakerParts::HeadBone, Head))
	{
		Head = TakerMesh->GetComponentLocation();
	}
	return Head + GetActorRightVector() * FMath::Sign(TakerOffset.Y) * TakerAimSideOffset;
}

FVector AHostageTaker::GetHostageAimPoint() const
{
	FVector Chest;
	return FHitReactions::GetBonePoint(HostageMesh, HostageTakerParts::ChestBone, Chest) ? Chest : HostageMesh->GetComponentLocation();
}

bool AHostageTaker::GetBonePoint(FName Bone, bool bHostage, FVector& OutPoint) const
{
	return FHitReactions::GetBonePoint(bHostage ? HostageMesh : TakerMesh, Bone, OutPoint);
}

bool AHostageTaker::GetWeaponPoint(FVector& OutPoint) const
{
	return FHitReactions::GetWeaponPoint(TakerWeapon, OutPoint);
}

bool AHostageTaker::NotifyShot(const FHitResult& InHit, const FVector& ShotDirection, AController* InstigatedBy)
{
	// a shot on a primitive stand in counts on the body it stands in for (D-068)
	const FHitResult Hit = FStandInBody::Redirect(InHit);

	UPrimitiveComponent* HitComponent = Hit.GetComponent();
	if (!bActive || bResolved || !HitComponent)
	{
		return false;
	}

	UStyleScoringComponent* Style = UStyleScoringComponent::ForController(InstigatedBy);
	const bool bOnTaker = HitComponent == TakerMesh || HitComponent == TakerWeapon;

	// the hostage's body shapes leave gaps the old volumes did not: a shot that grazes the hostage on its way to the taker counts on the hostage
	FHitResult Graze;
	const bool bGrazed = bOnTaker && !bHostageDown && FHitReactions::Grazes(HostageMesh, Hit, ShotDirection, Graze);

	// the hostage counts whenever it is hit, a freed hostage still stands in the line of fire until the set piece ends
	if (HitComponent == HostageMesh || bGrazed)
	{
		if (bHostageDown)
		{
			return false;
		}

		bHostageDown = true;
		FHitReactions::SetWeaponShootable(TakerWeapon, false);
		PlayInjured(HostageMesh, UHitZoneSettings::Resolve(bGrazed ? Graze : Hit, ShotDirection));
		if (Style)
		{
			Style->RecordNonTargetHit(this);
		}
		UE_LOG(LogHostageTaker, Log, TEXT("%s: the hostage was hit%s"), *GetName(), bGrazed ? TEXT(", grazed by a shot on the taker") : TEXT(""));
		ScheduleOutcome(EHostageOutcome::HostageHit);
		return true;
	}

	if (!bOnTaker)
	{
		return false;
	}

	// with the hostage down there is nothing left to save
	if (bHostageDown)
	{
		return false;
	}

	const FHitZoneResult Zone = UHitZoneSettings::Resolve(Hit, ShotDirection);

	// a second shot on a taker that is already down or disarmed still reaches the style record and can complete a pair
	if (bTakerDown)
	{
		const double Now = GetWorld()->GetTimeSeconds();
		const bool bFollowUp = Now - TakerDownAt <= UStyleSettings::Get(this)->ControlledPairWindow;
		if (bFollowUp && Style)
		{
			// another pellet of the shot that counted can still show a better zone, a later shot cannot
			Style->RecordTargetHit(this, Now == TakerDownAt ? Zone.Zone : EHitZone::None, false);
		}
		return bFollowUp;
	}

	// whatever shows of the taker beside its hostage is the zone: the taker is out of the fight and the hostage is free
	bTakerDown = true;
	TakerDownAt = GetWorld()->GetTimeSeconds();
	if (Zone.IsDisarm())
	{
		// the pistol, or the arm holding it: disarmed and rescued, the taker stays up without its weapon (D-050)
		const bool bClean = Zone.Zone == EHitZone::Weapon;
		bTakerDisarmed = true;
		FHitReactions::DropWeapon(TakerWeapon, ShotDirection, Hit.ImpactPoint, bClean);
		FHitReactions::PlayReaction(TakerMesh, Zone.GetReaction(), Zone.Zone, Zone.Direction);
		if (!FHitReactions::PlayDisarmedIdle(TakerMesh))
		{
			PlayLoop(TakerMesh, HostageIdleAnimation, UHitZoneSettings::Get()->DisarmedBlend);
		}
		if (Style)
		{
			Style->RecordDisarm(this, bClean);
			Style->RecordRescue(this);
		}
		UE_LOG(LogHostageTaker, Log, TEXT("Disarm on %s: %s, the hostage is free"), *GetName(), bClean ? TEXT("clean") : TEXT("arm"));
	}
	else
	{
		FHitReactions::SetWeaponShootable(TakerWeapon, false);
		PlayDown(TakerMesh, Zone);
		if (Style)
		{
			Style->RecordTargetHit(this, Zone.Zone, true);
			Style->RecordRescue(this);
		}
		UE_LOG(LogHostageTaker, Log, TEXT("%s is down with a %s hit, the hostage is free"), *GetName(), *StaticEnum<EHitZone>()->GetNameStringByValue(static_cast<int64>(Zone.Zone)));
	}
	ScheduleOutcome(EHostageOutcome::Rescued);
	return true;
}

void AHostageTaker::PlayLoop(USkeletalMeshComponent* Body, UAnimSequenceBase* Animation, float BlendTime)
{
	UTargetAnimInstance* Anim = UTargetAnimInstance::FromMesh(Body);
	if (Anim && Animation)
	{
		Anim->PlayBase(Animation, true, 1.0f, 0.0f, BlendTime);
	}
}

void AHostageTaker::PlayDown(USkeletalMeshComponent* Body, const FHitZoneResult& Zone)
{
	// a leg staggers and an arm flinches before the body falls, a head or torso hit falls at once
	const bool bLimb = Zone.Zone == EHitZone::Leg || Zone.Zone == EHitZone::OffArm || Zone.Zone == EHitZone::WeaponArm;
	const EHitDirection Direction = Zone.Direction;
	if (bLimb)
	{
		TWeakObjectPtr<AHostageTaker> WeakTaker(this);
		TWeakObjectPtr<USkeletalMeshComponent> WeakBody(Body);
		const bool bReacting = FHitReactions::PlayReaction(Body, Zone.GetReaction(), Zone.Zone, Direction, [WeakTaker, WeakBody, Direction]()
		{
			if (AHostageTaker* Taker = WeakTaker.Get())
			{
				Taker->PlayFallOn(WeakBody.Get(), Direction);
			}
		});
		if (bReacting)
		{
			return;
		}
	}
	PlayFallOn(Body, Direction);
}

void AHostageTaker::PlayInjured(USkeletalMeshComponent* Body, const FHitZoneResult& Zone)
{
	// the penalty is the same wherever it lands; a killing zone drops the hostage, any other only makes it react
	const EHitReaction Reaction = Zone.GetReaction();
	if (Reaction != EHitReaction::None && Reaction != EHitReaction::Kill && FHitReactions::PlayReaction(Body, Reaction, Zone.Zone, Zone.Direction))
	{
		return;
	}
	PlayFallOn(Body, Zone.Direction);
}

void AHostageTaker::PlayFallOn(USkeletalMeshComponent* Body, EHitDirection Direction)
{
	if (Body && !FHitReactions::PlayDeath(Body, Direction))
	{
		PlayHitOn(Body);
	}
}

void AHostageTaker::PlayHitOn(USkeletalMeshComponent* Body)
{
	FHitReactions::PlayFall(Body, HitAnimation, HitAnimationRate);
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

	// the bodies stay where they fell, they only stop catching shots
	SetBodiesShootable(false);

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
	SetBodiesShootable(false);
	RevealDirection = -1.0f;
	SetActorTickEnabled(true);
	PendingOutcome = EHostageOutcome::Escaped;
	GetWorldTimerManager().SetTimer(OutcomeTimer, this, &AHostageTaker::ReportOutcome, FMath::Max(RevealSeconds, 0.01f), false);
}
