// CYB3RGUN THEGAME. The player controller of the main menu: a cursor, and the menu camera as the view.

#include "MainMenuPlayerController.h"
#include "MenuCameraRig.h"
#include "Engine/World.h"
#include "EngineUtils.h"

AMainMenuPlayerController::AMainMenuPlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;

	// there is never a pawn to follow, the view stays on the menu camera
	bAutoManageActiveCameraTarget = false;
}

void AMainMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}

	AMenuCameraRig* Rig = nullptr;
	for (TActorIterator<AMenuCameraRig> It(GetWorld()); It; ++It)
	{
		Rig = *It;
		break;
	}
	if (!Rig)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Rig = GetWorld()->SpawnActor<AMenuCameraRig>(FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	}
	if (Rig)
	{
		SetViewTarget(Rig);
	}

	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
}
