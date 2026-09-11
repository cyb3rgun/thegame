// CYB3RGUN THEGAME. The slow camera move behind the main menu.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MenuCameraRig.generated.h"

class UCameraComponent;

/**
 *  Circles the actor's location slowly at eye height and looks outward, a little ahead of its path, so the doors
 *  and lanterns of the background pass by. A gentle rise and fall keeps the move from looking mechanical.
 */
UCLASS()
class CYB3RGUN_API AMenuCameraRig : public AActor
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USceneComponent* Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* Camera;

public:

	AMenuCameraRig();

	virtual void Tick(float DeltaSeconds) override;

protected:

	virtual void BeginPlay() override;

	/** Distance of the camera from the centre it circles */
	UPROPERTY(EditAnywhere, Category="Camera", meta = (ClampMin = 0.0, Units = "cm"))
	float OrbitRadius = 160.0f;

	UPROPERTY(EditAnywhere, Category="Camera", meta = (Units = "cm"))
	float Height = 175.0f;

	/** Degrees per second around the centre, negative turns the other way */
	UPROPERTY(EditAnywhere, Category="Camera", meta = (Units = "Degrees"))
	float OrbitSpeed = 3.0f;

	/** How far the view leads the orbit, so the camera looks past its own path at the doors ahead */
	UPROPERTY(EditAnywhere, Category="Camera", meta = (Units = "Degrees"))
	float LookAhead = 35.0f;

	UPROPERTY(EditAnywhere, Category="Camera", meta = (Units = "Degrees"))
	float Pitch = -3.0f;

	UPROPERTY(EditAnywhere, Category="Camera", meta = (ClampMin = 5.0, ClampMax = 170.0, Units = "Degrees"))
	float FieldOfView = 70.0f;

	/** Height of the slow rise and fall */
	UPROPERTY(EditAnywhere, Category="Camera", meta = (ClampMin = 0.0, Units = "cm"))
	float BobHeight = 8.0f;

	UPROPERTY(EditAnywhere, Category="Camera", meta = (ClampMin = 0.0, Units = "s"))
	float BobPeriod = 11.0f;

	float Angle = 0.0f;
	float Elapsed = 0.0f;

	void UpdateCamera();
};
