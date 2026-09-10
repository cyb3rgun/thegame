// CYB3RGUN THEGAME. The pawn that rides a rail route.

#include "RailPawn.h"
#include "EncounterDefinition.h"
#include "RailAimComponent.h"
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

	CameraRoot->SetRelativeLocation(FVector(0.0f, 0.0f, EyeHeight - Capsule->GetUnscaledCapsuleHalfHeight()));

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
	const float DeltaSeconds = GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.0f;
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
	const float Incoming = Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
	if (Health <= 0.0f || Incoming <= 0.0f)
	{
		return 0.0f;
	}

	const float Applied = FMath::Min(Incoming, Health);
	Health -= Applied;
	UE_LOG(LogRail, Verbose, TEXT("Rider takes %.0f damage from %s, health %.0f"), Applied, *GetNameSafe(DamageCauser), Health);
	OnHealthChanged.Broadcast(Health, MaxHealth);

	if (Health <= 0.0f)
	{
		// placeholder until the rail has a fail state: the ride stops where the rider went down
		UE_LOG(LogRail, Warning, TEXT("Rider down at %.0f cm, ride paused"), DistanceAlongSpline);
		Pause();
	}

	return Applied;
}
