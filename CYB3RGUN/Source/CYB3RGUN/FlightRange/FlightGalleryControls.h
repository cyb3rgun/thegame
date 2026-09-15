// CYB3RGUN THEGAME. The flight range's gallery controls: a free crosshair on a fixed forward view that slides along the scene (D-078).

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FlightGalleryControls.generated.h"

class APawn;
class APlayerController;
class UInputAction;
class UInputMappingContext;
class URailAimComponent;
class UFlightRangeSettings;
struct FInputActionValue;

/**
 *  Put on the player's pawn by the flight range. The view never turns: it looks along the field and keeps its pitch. The
 *  mouse and the right stick move the crosshair over the screen instead, through the pawn's screen space aim (D-019), so
 *  the weapons fire where the crosshair is. When the crosshair enters the edge zone at either side, the player slides
 *  sideways along a rail across the field, faster the further out the crosshair is; in the middle of the screen nothing
 *  moves. A/D, the arrow keys and the left stick slide the same way. The rail is as long as the settings' scene width in
 *  screen widths, measured at the gallery depth, with hard stops at both ends, and the player starts in its middle.
 *  Space and the right mouse button reload, next to the weapon's own reload binding.
 */
UCLASS(ClassGroup=(FlightRange))
class CYB3RGUN_API UFlightGalleryControls : public UActorComponent
{
	GENERATED_BODY()

public:

	UFlightGalleryControls();

	/** Adds the controls to a possessed pawn and plants it in the middle of its rail. Field is the ground point and yaw the view faces. */
	static UFlightGalleryControls* Attach(APawn* Pawn, const UFlightRangeSettings* Settings, const FTransform& Field);

	/** Stops sliding and aiming, for the end of the round */
	void SetControlsEnabled(bool bEnabled);

	/** Sideways position on the rail, 0 in the middle, negative to the left */
	float GetOffset() const { return Offset; }

	/** Distance from the middle of the rail to either stop */
	float GetHalfTravel() const { return HalfTravel; }

	/** Width of the screen at the gallery depth */
	float GetScreenWidth() const { return ScreenWidth; }

	/** Unit vector along the rail, to the right of the view */
	const FVector& GetRailRight() const { return Right; }

	/** Where the player's eyes are now */
	FVector GetEyeLocation() const;

	/** Slide input from -1 to 1 on top of the keys and the edge zone, for test drivers */
	void SetDriverScroll(float Value) { DriverScroll = FMath::Clamp(Value, -1.0f, 1.0f); }

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:

	UPROPERTY(Transient)
	TObjectPtr<const UFlightRangeSettings> Settings;

	UPROPERTY(Transient)
	TObjectPtr<URailAimComponent> Aim;

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> GalleryContext;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> MouseAimAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> StickAimAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> ScrollAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> ReloadAction;

	/** Pawn location in the middle of the rail */
	FVector RailOrigin = FVector::ZeroVector;
	FVector Right = FVector::RightVector;
	FRotator ViewRotation = FRotator::ZeroRotator;
	float Offset = 0.0f;
	float HalfTravel = 0.0f;
	float ScreenWidth = 0.0f;
	float KeyScroll = 0.0f;
	float DriverScroll = 0.0f;
	bool bControlsEnabled = true;

	APlayerController* GetPlayerController() const;

	/** Screen width and rail length from the camera's field of view, which the settings menu can change */
	void UpdateTravel();

	/** Slide speed the crosshair asks for, -1 to 1 */
	float GetEdgeScroll() const;

	void SetupInput(APawn* Pawn);
	void HandleMouseAim(const FInputActionValue& Value);
	void HandleStickAim(const FInputActionValue& Value);
	void HandleScroll(const FInputActionValue& Value);
	void HandleScrollReleased(const FInputActionValue& Value);
	void HandleReload(const FInputActionValue& Value);
};
