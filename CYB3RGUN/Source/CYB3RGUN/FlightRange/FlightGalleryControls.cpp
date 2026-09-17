// CYB3RGUN THEGAME. The flight range's gallery controls: a free crosshair on a fixed forward view that slides along the scene (D-078).

#include "FlightGalleryControls.h"
#include "CyberGameUserSettings.h"
#include "FlightRangeSettings.h"
#include "RailAimComponent.h"
#include "ShooterCharacter.h"
#include "Camera/PlayerCameraManager.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"

DEFINE_LOG_CATEGORY_STATIC(LogFlightGallery, Log, All);

namespace FlightGalleryInput
{
	/** Above the template's look, move and jump contexts, so the keys this mode takes are not passed on to them */
	constexpr int32 ContextPriority = 10;

	UInputAction* MakeAction(UObject* Outer, const TCHAR* Name, EInputActionValueType Type)
	{
		UInputAction* Action = NewObject<UInputAction>(Outer, Name);
		Action->ValueType = Type;
		Action->bConsumeInput = true;
		return Action;
	}

	void MapNegated(UInputMappingContext* Context, UInputAction* Action, const FKey& Key)
	{
		FEnhancedActionKeyMapping& Mapping = Context->MapKey(Action, Key);
		Mapping.Modifiers.Add(NewObject<UInputModifierNegate>(Context));
	}
}

UFlightGalleryControls::UFlightGalleryControls()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

UFlightGalleryControls* UFlightGalleryControls::Attach(APawn* Pawn, const UFlightRangeSettings* InSettings, const FTransform& Field)
{
	if (!Pawn || !InSettings)
	{
		return nullptr;
	}

	UFlightGalleryControls* Controls = NewObject<UFlightGalleryControls>(Pawn, TEXT("GalleryControls"));
	Controls->Settings = InSettings;
	Controls->Aim = Pawn->FindComponentByClass<URailAimComponent>();
	Controls->RailOrigin = Pawn->GetActorLocation();
	const FVector Forward = Field.GetRotation().GetForwardVector().GetSafeNormal2D();
	Controls->Right = FVector::CrossProduct(FVector::UpVector, Forward);
	Controls->ViewRotation = FRotator(InSettings->ViewPitchDegrees, Field.Rotator().Yaw, 0.0f);
	Controls->RegisterComponent();

	// the weapons fire at the crosshair, which now moves over the screen instead of the view turning under it
	if (Controls->Aim)
	{
		Controls->Aim->SetInputMode(ERailAimInputMode::Relative);
		Controls->Aim->SetCrosshairNormalized(FVector2D(0.5, 0.5));
	}
	else
	{
		UE_LOG(LogFlightGallery, Warning, TEXT("%s has no screen space aim, the crosshair cannot move"), *GetNameSafe(Pawn));
	}

	if (APlayerController* PC = Controls->GetPlayerController())
	{
		PC->SetIgnoreLookInput(true);
		PC->SetControlRotation(Controls->ViewRotation);
	}
	Controls->SetupInput(Pawn);
	Controls->UpdateTravel();

	UE_LOG(LogFlightGallery, Log, TEXT("Gallery controls on %s: screen width %.0f cm at %.0f cm, rail %.0f cm either side, edge zone %.2f, slide %.2f screens per second"),
		*GetNameSafe(Pawn), Controls->ScreenWidth, InSettings->GalleryDepth, Controls->HalfTravel, InSettings->EdgeZone, InSettings->ScrollSpeed);
	return Controls;
}

APlayerController* UFlightGalleryControls::GetPlayerController() const
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	return Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
}

void UFlightGalleryControls::SetControlsEnabled(bool bEnabled)
{
	bControlsEnabled = bEnabled;
	KeyScroll = 0.0f;
	DriverScroll = 0.0f;
}

FVector UFlightGalleryControls::GetEyeLocation() const
{
	if (const APlayerController* PC = GetPlayerController())
	{
		FVector Location;
		FRotator Rotation;
		PC->GetPlayerViewPoint(Location, Rotation);
		return Location;
	}
	return GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector;
}

void UFlightGalleryControls::UpdateTravel()
{
	const APlayerController* PC = GetPlayerController();
	const float FOV = PC && PC->PlayerCameraManager ? PC->PlayerCameraManager->GetFOVAngle() : 90.0f;
	ScreenWidth = 2.0f * Settings->GalleryDepth * FMath::Tan(FMath::DegreesToRadians(FMath::Clamp(FOV, 20.0f, 150.0f) * 0.5f));
	HalfTravel = 0.5f * FMath::Max(Settings->MapWidthScreens - 1.0f, 0.0f) * ScreenWidth;
}

float UFlightGalleryControls::GetEdgeScroll() const
{
	if (!Aim || !Settings)
	{
		return 0.0f;
	}

	// nothing in the middle; from the inner border of the zone to the furthest the crosshair can go, the speed grows to full
	const float X = static_cast<float>(Aim->GetCrosshairNormalized().X);
	const float Zone = FMath::Max(Settings->EdgeZone - Aim->GetEdgeMargin(), 0.01f);
	const float ToRight = FMath::Clamp((X - (1.0f - Settings->EdgeZone)) / Zone, 0.0f, 1.0f);
	const float ToLeft = FMath::Clamp((Settings->EdgeZone - X) / Zone, 0.0f, 1.0f);
	return ToRight - ToLeft;
}

void UFlightGalleryControls::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Pawn || !Settings)
	{
		return;
	}

	// the view stays forward whatever else tries to turn it
	if (APlayerController* PC = GetPlayerController())
	{
		if (!PC->GetControlRotation().Equals(ViewRotation, 0.01f))
		{
			PC->SetControlRotation(ViewRotation);
		}
	}

	UpdateTravel();
	if (!bControlsEnabled)
	{
		return;
	}

	// on real time, so a slowed world never slows the view the player steers with
	const float RealDelta = Pawn->GetWorld() ? Pawn->GetWorld()->GetDeltaSeconds() / FMath::Max(Pawn->GetWorld()->GetWorldSettings()->GetEffectiveTimeDilation(), 0.01f) : DeltaTime;
	const float Scroll = FMath::Clamp(GetEdgeScroll() + KeyScroll + DriverScroll, -1.0f, 1.0f);
	const float NewOffset = FMath::Clamp(Offset + Scroll * Settings->ScrollSpeed * ScreenWidth * RealDelta, -HalfTravel, HalfTravel);
	if (!FMath::IsNearlyEqual(NewOffset, Offset) || !Pawn->GetActorLocation().Equals(RailOrigin + Right * Offset, 0.1))
	{
		Offset = NewOffset;
		Pawn->SetActorLocation(RailOrigin + Right * Offset);
	}
}

void UFlightGalleryControls::SetupInput(APawn* Pawn)
{
	const APlayerController* PC = GetPlayerController();
	UEnhancedInputLocalPlayerSubsystem* Subsystem = PC ? ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()) : nullptr;
	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(Pawn->InputComponent);
	if (!Subsystem || !Input)
	{
		UE_LOG(LogFlightGallery, Warning, TEXT("%s has no Enhanced Input to bind the gallery controls to"), *GetNameSafe(Pawn));
		return;
	}

	using namespace FlightGalleryInput;
	MouseAimAction = MakeAction(this, TEXT("GalleryMouseAim"), EInputActionValueType::Axis2D);
	StickAimAction = MakeAction(this, TEXT("GalleryStickAim"), EInputActionValueType::Axis2D);
	ScrollAction = MakeAction(this, TEXT("GalleryScroll"), EInputActionValueType::Axis1D);
	ReloadAction = MakeAction(this, TEXT("GalleryReload"), EInputActionValueType::Boolean);

	GalleryContext = NewObject<UInputMappingContext>(this, TEXT("GalleryContext"));
	GalleryContext->MapKey(MouseAimAction, EKeys::Mouse2D);
	GalleryContext->MapKey(StickAimAction, EKeys::Gamepad_Right2D);
	GalleryContext->MapKey(ScrollAction, EKeys::D);
	GalleryContext->MapKey(ScrollAction, EKeys::Right);
	GalleryContext->MapKey(ScrollAction, EKeys::Gamepad_LeftX);
	MapNegated(GalleryContext, ScrollAction, EKeys::A);
	MapNegated(GalleryContext, ScrollAction, EKeys::Left);
	GalleryContext->MapKey(ReloadAction, EKeys::SpaceBar);
	GalleryContext->MapKey(ReloadAction, EKeys::RightMouseButton);
	Subsystem->AddMappingContext(GalleryContext, ContextPriority);

	Input->BindAction(MouseAimAction, ETriggerEvent::Triggered, this, &UFlightGalleryControls::HandleMouseAim);
	Input->BindAction(StickAimAction, ETriggerEvent::Triggered, this, &UFlightGalleryControls::HandleStickAim);
	Input->BindAction(ScrollAction, ETriggerEvent::Triggered, this, &UFlightGalleryControls::HandleScroll);
	Input->BindAction(ScrollAction, ETriggerEvent::Completed, this, &UFlightGalleryControls::HandleScrollReleased);
	Input->BindAction(ReloadAction, ETriggerEvent::Started, this, &UFlightGalleryControls::HandleReload);
}

void UFlightGalleryControls::HandleMouseAim(const FInputActionValue& Value)
{
	// raw mouse Y grows upward, the screen downward
	const FVector2D Delta = Value.Get<FVector2D>();
	if (Aim && bControlsEnabled && Settings)
	{
		Aim->AddAimInput(FVector2D(Delta.X, -Delta.Y) * Settings->MouseAimSensitivity * UCyberGameUserSettings::GetAimSensitivityOrDefault());
	}
}

void UFlightGalleryControls::HandleStickAim(const FInputActionValue& Value)
{
	const FVector2D Deflection = Value.Get<FVector2D>();
	const UWorld* World = GetWorld();
	if (Aim && bControlsEnabled && Settings && World)
	{
		const float RealDelta = World->GetDeltaSeconds() / FMath::Max(World->GetWorldSettings()->GetEffectiveTimeDilation(), 0.01f);
		Aim->AddAimInput(FVector2D(Deflection.X, -Deflection.Y) * Settings->StickAimSpeed * RealDelta);
	}
}

void UFlightGalleryControls::HandleScroll(const FInputActionValue& Value)
{
	KeyScroll = FMath::Clamp(Value.Get<float>(), -1.0f, 1.0f);
}

void UFlightGalleryControls::HandleScrollReleased(const FInputActionValue& Value)
{
	KeyScroll = 0.0f;
}

void UFlightGalleryControls::HandleReload(const FInputActionValue& Value)
{
	if (AShooterCharacter* Shooter = Cast<AShooterCharacter>(GetOwner()))
	{
		if (bControlsEnabled)
		{
			Shooter->DoReload();
		}
	}
}
