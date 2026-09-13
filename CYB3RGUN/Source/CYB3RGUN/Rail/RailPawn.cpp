// CYB3RGUN THEGAME. The pawn that rides a rail route.

#include "RailPawn.h"
#include "EncounterDefinition.h"
#include "RailAimComponent.h"
#include "WeaponDefinition.h"
#include "CombatFeelSubsystem.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "NavigationInvokerComponent.h"
#include "TimerManager.h"
#include "InputCoreTypes.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogRail, Log, All);

ARailPawn::ARailPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	Capsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Capsule"));
	Capsule->InitCapsuleSize(40.0f, 90.0f);
	Capsule->SetCollisionProfileName(UCollisionProfile::Pawn_ProfileName);
	// like a character's capsule it must not cut the nav mesh, or the enemies' move goal lands in a hole
	Capsule->SetCanEverAffectNavigation(false);
	RootComponent = Capsule;

	CameraRoot = CreateDefaultSubobject<USceneComponent>(TEXT("CameraRoot"));
	CameraRoot->SetupAttachment(Capsule);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CameraRoot);
	Camera->bUsePawnControlRotation = false;
	Camera->SetFieldOfView(90.0f);

	NavInvoker = CreateDefaultSubobject<UNavigationInvokerComponent>(TEXT("NavInvoker"));
	NavInvoker->SetGenerationRadii(NavInvokerRadius, NavInvokerRadius + 500.0f);

	Aim = CreateDefaultSubobject<URailAimComponent>(TEXT("Aim"));
	Aim->SetInputMode(ERailAimInputMode::Relative);

	// the rider carries the 3R service pistol, both AP JET big bores and the scattergun, switching uses the shooter template's action
	static ConstructorHelpers::FObjectFinder<UWeaponDefinition> SidearmWeapon(TEXT("/Game/CYB3RGUN/Weapons/DA_Weapon_3R_9x19.DA_Weapon_3R_9x19"));
	static ConstructorHelpers::FObjectFinder<UWeaponDefinition> JetSingleWeapon(TEXT("/Game/CYB3RGUN/Weapons/DA_Weapon_APJet1.DA_Weapon_APJet1"));
	static ConstructorHelpers::FObjectFinder<UWeaponDefinition> JetSemiWeapon(TEXT("/Game/CYB3RGUN/Weapons/DA_Weapon_APJet2.DA_Weapon_APJet2"));
	static ConstructorHelpers::FObjectFinder<UWeaponDefinition> ScattergunWeapon(TEXT("/Game/CYB3RGUN/Weapons/DA_Weapon_Scattergun.DA_Weapon_Scattergun"));
	static ConstructorHelpers::FObjectFinder<UInputAction> SwitchInput(TEXT("/Game/Variant_Shooter/Input/Actions/IA_SwapWeapon.IA_SwapWeapon"));
	Aim->SetWeapons({ SidearmWeapon.Object, JetSingleWeapon.Object, JetSemiWeapon.Object, ScattergunWeapon.Object });
	SwitchWeaponAction = SwitchInput.Object;
	static ConstructorHelpers::FObjectFinder<UInputAction> OverclockInput(TEXT("/Game/CYB3RGUN/Core/Input/IA_Overclock.IA_Overclock"));
	OverclockAction = OverclockInput.Object;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
	AutoPossessAI = EAutoPossessAI::Disabled;
}

void ARailPawn::BeginPlay()
{
	Super::BeginPlay();

	Health = MaxHealth;
	OnHealthChanged.Broadcast(Health, MaxHealth);

	CameraBaseLocation = FVector(0.0f, 0.0f, EyeHeight - Capsule->GetUnscaledCapsuleHalfHeight());
	CameraRoot->SetRelativeLocation(CameraBaseLocation);

	// the invoker registers its radii when it activates, so re-register with the configured radius
	NavInvoker->Deactivate();
	NavInvoker->SetGenerationRadii(NavInvokerRadius, NavInvokerRadius + 500.0f);
	NavInvoker->Activate(true);

	if (!Track)
	{
		for (TActorIterator<ARailTrack> It(GetWorld()); It; ++It)
		{
			Track = *It;
			UE_LOG(LogRail, Warning, TEXT("%s has no track assigned, using %s"), *GetName(), *Track->GetName());
			break;
		}
	}

	if (Track)
	{
		InitialTrack = Track;
		SetTrack(Track, DistanceAlongSpline);
	}
	else
	{
		UE_LOG(LogRail, Warning, TEXT("%s found no rail track in the level"), *GetName());
	}

	if (bAutoStart)
	{
		GetWorldTimerManager().SetTimer(StartTimer, this, &ARailPawn::StartRide, FMath::Max(StartDelay, 0.01f), false);
	}
}

void ARailPawn::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(StartTimer);
	GetWorldTimerManager().ClearTimer(RespawnTimer);
	Super::EndPlay(EndPlayReason);
}

void ARailPawn::PawnClientRestart()
{
	Super::PawnClientRestart();

	const APlayerController* PC = Cast<APlayerController>(GetController());
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = PC ? ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()) : nullptr)
	{
		for (UInputMappingContext* Context : MappingContexts)
		{
			if (Context)
			{
				Subsystem->AddMappingContext(Context, 0);
			}
		}

		// Q and the mouse wheel switch as well, E and the left shoulder hold Overclock; the rail has no reload key, taking cover reloads
		if (!CombatMappingContext)
		{
			CombatMappingContext = NewObject<UInputMappingContext>(this, TEXT("CombatMappingContext"));
			if (SwitchWeaponAction)
			{
				CombatMappingContext->MapKey(SwitchWeaponAction, EKeys::Q);
				CombatMappingContext->MapKey(SwitchWeaponAction, EKeys::MouseScrollUp);
				CombatMappingContext->MapKey(SwitchWeaponAction, EKeys::MouseScrollDown);
			}
			if (OverclockAction)
			{
				CombatMappingContext->MapKey(OverclockAction, EKeys::E);
				CombatMappingContext->MapKey(OverclockAction, EKeys::Gamepad_LeftShoulder);
			}
		}
		if (CombatMappingContext)
		{
			Subsystem->AddMappingContext(CombatMappingContext, 1);
		}
	}
}

void ARailPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!Input)
	{
		UE_LOG(LogRail, Warning, TEXT("%s needs an Enhanced Input component"), *GetName());
		return;
	}

	if (FireAction)
	{
		Input->BindAction(FireAction, ETriggerEvent::Started, this, &ARailPawn::DoFire);
	}
	if (MouseAimAction)
	{
		Input->BindAction(MouseAimAction, ETriggerEvent::Triggered, this, &ARailPawn::MouseAimInput);
	}
	if (StickAimAction)
	{
		Input->BindAction(StickAimAction, ETriggerEvent::Triggered, this, &ARailPawn::StickAimInput);
	}
	if (CoverAction)
	{
		Input->BindAction(CoverAction, ETriggerEvent::Started, this, &ARailPawn::CoverPressed);
		Input->BindAction(CoverAction, ETriggerEvent::Completed, this, &ARailPawn::CoverReleased);
	}
	if (SwitchWeaponAction)
	{
		Input->BindAction(SwitchWeaponAction, ETriggerEvent::Triggered, this, &ARailPawn::DoSwitchWeapon);
	}
	if (OverclockAction)
	{
		Input->BindAction(OverclockAction, ETriggerEvent::Started, this, &ARailPawn::OverclockPressed);
		Input->BindAction(OverclockAction, ETriggerEvent::Completed, this, &ARailPawn::OverclockReleased);
	}
}

void ARailPawn::CoverPressed()
{
	SetInCover(bHoldForCover ? true : !bInCover);
}

void ARailPawn::CoverReleased()
{
	if (bHoldForCover)
	{
		SetInCover(false);
	}
}

void ARailPawn::SetInCover(bool bCover)
{
	if (bInCover == bCover)
	{
		return;
	}

	bInCover = bCover;
	Aim->SetFireBlocked(bInCover);

	// cover is the reload: entering it starts one, leaving before it finished leaves the magazine as it was
	if (bInCover)
	{
		Aim->StartReload();
	}
	else
	{
		Aim->CancelReload();
	}
	UE_LOG(LogRail, Log, TEXT("Cover %s at %.0f cm, ride %s"), bInCover ? TEXT("on") : TEXT("off"), DistanceAlongSpline, IsMoving() ? TEXT("moving") : TEXT("waiting"));
	OnCoverChanged.Broadcast(bInCover);
}

void ARailPawn::UpdateCoverCamera(float DeltaSeconds)
{
	const float Target = bInCover ? 1.0f : 0.0f;
	if (FMath::IsNearlyEqual(CoverBlend, Target, KINDA_SMALL_NUMBER))
	{
		return;
	}
	CoverBlend = FMath::FInterpTo(CoverBlend, Target, DeltaSeconds, CoverBlendSpeed);
	CameraRoot->SetRelativeLocation(CameraBaseLocation + CoverCameraOffset * CoverBlend);
}

void ARailPawn::MouseAimInput(const FInputActionValue& Value)
{
	// IMC_MouseLook negates the raw mouse Y, which is positive upward, so this value already grows downward like the screen
	const FVector2D Delta = Value.Get<FVector2D>();
	Aim->AddAimInput(FVector2D(Delta.X, Delta.Y) * MouseAimSensitivity);
}

void ARailPawn::StickAimInput(const FInputActionValue& Value)
{
	// stick Y is positive upward and arrives without modifiers, the screen grows downward
	const FVector2D Deflection = Value.Get<FVector2D>();
	// on the rider's own time, so a slowed world does not slow the crosshair
	const float DeltaSeconds = GetWorld() ? GetWorld()->GetDeltaSeconds() * CustomTimeDilation : 0.0f;
	Aim->AddAimInput(FVector2D(Deflection.X, -Deflection.Y) * StickAimSpeed * DeltaSeconds);
}

void ARailPawn::DoFire()
{
	Aim->Fire();
}

void ARailPawn::StartRide()
{
	if (bStarted || !Track)
	{
		return;
	}

	bStarted = true;
	RideStartTime = GetWorld()->GetTimeSeconds();
	UE_LOG(LogRail, Log, TEXT("Ride starts on %s at %.0f cm, speed %.0f cm/s, segment length %.0f cm"), *Track->GetName(), DistanceAlongSpline, Speed, Track->GetLength());
}

void ARailPawn::RestartRide(float StartDistance)
{
	ARailTrack* First = InitialTrack ? InitialTrack.Get() : Track.Get();
	if (!First)
	{
		return;
	}

	bStarted = true;
	bFinished = false;
	bPausedByRequest = false;
	RideDistance = 0.0f;
	RideStartTime = GetWorld()->GetTimeSeconds();
	GetWorldTimerManager().ClearTimer(StartTimer);
	SetTrack(First, StartDistance);
	UE_LOG(LogRail, Log, TEXT("Ride restarts on %s at %.0f cm, speed %.0f cm/s"), *First->GetName(), StartDistance, Speed);
}

void ARailPawn::Pause()
{
	if (!bPausedByRequest)
	{
		bPausedByRequest = true;
		UE_LOG(LogRail, Log, TEXT("Ride paused at %.0f cm"), DistanceAlongSpline);
	}
}

void ARailPawn::Resume()
{
	if (bPausedByRequest)
	{
		bPausedByRequest = false;
		UE_LOG(LogRail, Log, TEXT("Ride resumed at %.0f cm"), DistanceAlongSpline);
	}
}

void ARailPawn::SetSpeed(float NewSpeed)
{
	Speed = FMath::Max(NewSpeed, 0.0f);
	UE_LOG(LogRail, Log, TEXT("Ride speed %.0f cm/s"), Speed);
}

bool ARailPawn::IsMoving() const
{
	return bStarted && !bFinished && Track && !bPausedByRequest && HeldBeatIndex == INDEX_NONE && !IsHeldByState();
}

void ARailPawn::SetTrack(ARailTrack* NewTrack, float StartDistance)
{
	Track = NewTrack;
	if (!Track)
	{
		return;
	}

	DistanceAlongSpline = FMath::Clamp(StartDistance, 0.0f, Track->GetLength());
	HeldBeatIndex = INDEX_NONE;

	// beats behind the start distance are skipped, a beat exactly at the start fires on the first advance
	const TArray<FRailBeat>& Beats = Track->GetBeats();
	NextBeatIndex = 0;
	while (NextBeatIndex < Beats.Num() && Beats[NextBeatIndex].TriggerDistance < DistanceAlongSpline)
	{
		++NextBeatIndex;
	}

	ApplyTransform();
}

void ARailPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (IsMoving())
	{
		Advance(Speed * DeltaSeconds);
	}

	UpdateCoverCamera(DeltaSeconds);
}

void ARailPawn::Advance(float Delta)
{
	const float Length = Track->GetLength();
	const float Previous = DistanceAlongSpline;
	float Target = DistanceAlongSpline + Delta;

	const TArray<FRailBeat>& Beats = Track->GetBeats();
	while (NextBeatIndex < Beats.Num() && Beats[NextBeatIndex].TriggerDistance <= Target)
	{
		const int32 Index = NextBeatIndex++;
		const FRailBeat& Beat = Beats[Index];

		if (Beat.bHoldUntilCleared)
		{
			// stop exactly on the beat, the rest of this frame's travel is dropped
			Target = Beat.TriggerDistance;
			HeldBeatIndex = Index;
			HoldStartTime = GetWorld()->GetTimeSeconds();
		}
		if (Beat.bOverrideSpeed)
		{
			SetSpeed(Beat.SpeedOverride);
		}

		DistanceAlongSpline = FMath::Min(Beat.TriggerDistance, Length);
		UE_LOG(LogRail, Log, TEXT("Beat %d %s reached at %.0f cm on %s after %.1f s, hold %d, encounter %s"),
			Index, *Beat.Name.ToString(), Beat.TriggerDistance, *Track->GetName(), GetRideSeconds(), Beat.bHoldUntilCleared ? 1 : 0, *GetNameSafe(Beat.Encounter));

		OnBeatReached.Broadcast(Track, Index, Beat);

		// a listener may release the hold right away, otherwise the ride waits here
		if (HeldBeatIndex != INDEX_NONE)
		{
			break;
		}
	}

	DistanceAlongSpline = FMath::Clamp(Target, 0.0f, Length);
	RideDistance += FMath::Max(DistanceAlongSpline - Previous, 0.0f);
	ApplyTransform();

	if (HeldBeatIndex == INDEX_NONE && DistanceAlongSpline >= Length)
	{
		ReachEndOfTrack();
	}
}

void ARailPawn::ApplyTransform()
{
	if (!Track)
	{
		return;
	}

	const FTransform OnSpline = Track->GetWorldTransformAtDistance(DistanceAlongSpline);
	FRotator Rotation = OnSpline.Rotator();
	if (!bFollowSplinePitch)
	{
		Rotation.Pitch = 0.0f;
	}
	Rotation.Roll = 0.0f;

	// the spline runs along the ground, the capsule stands on it
	const FVector Location = OnSpline.GetLocation() + FVector(0.0f, 0.0f, Capsule->GetScaledCapsuleHalfHeight());
	SetActorLocationAndRotation(Location, Rotation, false, nullptr, ETeleportType::TeleportPhysics);

	if (Controller)
	{
		Controller->SetControlRotation(Rotation);
	}
}

void ARailPawn::ReleaseBeatHold(int32 BeatIndex)
{
	if (HeldBeatIndex == INDEX_NONE || HeldBeatIndex != BeatIndex)
	{
		return;
	}

	const float Held = GetWorld()->GetTimeSeconds() - HoldStartTime;
	HeldBeatIndex = INDEX_NONE;
	UE_LOG(LogRail, Log, TEXT("Beat %d cleared after %.1f s hold, ride continues from %.0f cm"), BeatIndex, Held, DistanceAlongSpline);
	OnBeatCleared.Broadcast(Track, BeatIndex, Held);
}

void ARailPawn::ReachEndOfTrack()
{
	ARailTrack* Finished = Track;
	ARailTrack* Next = Track->SelectNextTrack(this);

	UE_LOG(LogRail, Log, TEXT("Segment %s ends at %.0f cm after %.1f s, next %s"), *Finished->GetName(), DistanceAlongSpline, GetRideSeconds(), *GetNameSafe(Next));
	OnTrackEnded.Broadcast(Finished, Next);

	if (Next)
	{
		SetTrack(Next, 0.0f);
		return;
	}

	bFinished = true;
	UE_LOG(LogRail, Log, TEXT("Ride finished: %.0f cm in %.1f s"), RideDistance, GetRideSeconds());
	OnRideFinished.Broadcast(RideDistance, GetRideSeconds());
}

float ARailPawn::GetRideSeconds() const
{
	return bStarted ? GetWorld()->GetTimeSeconds() - RideStartTime : 0.0f;
}

float ARailPawn::TakeDamage(float Damage, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	// enemy hits do not land while the rider is in cover
	if (bInCover && Damage > 0.0f)
	{
		++HitsBlockedByCover;
		UE_LOG(LogRail, Verbose, TEXT("Cover blocks %.0f damage from %s, blocked %d"), Damage, *GetNameSafe(DamageCauser), HitsBlockedByCover);
		return 0.0f;
	}

	const float Incoming = Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
	if (Health <= 0.0f || Incoming <= 0.0f)
	{
		return 0.0f;
	}

	const float Applied = FMath::Min(Incoming, Health);
	Health -= Applied;
	UE_LOG(LogRail, Verbose, TEXT("Rider takes %.0f damage from %s, health %.0f"), Applied, *GetNameSafe(DamageCauser), Health);
	OnHealthChanged.Broadcast(Health, MaxHealth);
	if (UCombatFeelSubsystem* Feel = UCombatFeelSubsystem::Get(this))
	{
		Feel->NotifyPlayerDamaged(Applied);
	}

	if (Health <= 0.0f)
	{
		// the ride stops where the rider went down, and after a moment the rider gets back up and the ride goes on (D-059)
		UE_LOG(LogRail, Warning, TEXT("Rider down at %.0f cm, ride paused, back up in %.1f s"), DistanceAlongSpline, RiderRespawnDelay);
		Pause();
		GetWorldTimerManager().SetTimer(RespawnTimer, this, &ARailPawn::RiderGetsUp, FMath::Max(RiderRespawnDelay, 0.01f), false);
	}

	return Applied;
}

void ARailPawn::RiderGetsUp()
{
	Health = MaxHealth;
	OnHealthChanged.Broadcast(Health, MaxHealth);
	UE_LOG(LogRail, Log, TEXT("Rider back up at %.0f cm with %.0f health, the ride goes on"), DistanceAlongSpline, Health);
	Resume();
}

void ARailPawn::DoSwitchWeapon()
{
	if (Aim->SwitchWeapon() && bInCover)
	{
		Aim->StartReload();
	}
}

bool ARailPawn::GetWeaponStatus(FWeaponStatus& OutStatus) const
{
	if (!Aim->GetWeaponStatus(OutStatus))
	{
		return false;
	}
	OutStatus.ReloadHint = NSLOCTEXT("RailPawn", "ReloadHint", "TAKE COVER TO RELOAD");
	return true;
}

void ARailPawn::GetLoadout(TArray<FWeaponStatus>& OutLoadout) const
{
	Aim->GetLoadout(OutLoadout);
}

void ARailPawn::OverclockPressed()
{
	if (UCombatFeelSubsystem* Feel = UCombatFeelSubsystem::Get(this))
	{
		Feel->SetOverclockHeld(true);
	}
}

void ARailPawn::OverclockReleased()
{
	if (UCombatFeelSubsystem* Feel = UCombatFeelSubsystem::Get(this))
	{
		Feel->SetOverclockHeld(false);
	}
}
