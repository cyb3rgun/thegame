// CYB3RGUN THEGAME. The front end: a lit night scene behind the main menu, no player pawn.

#include "MainMenuGameMode.h"
#include "MainMenuPlayerController.h"
#include "DoorSlot.h"
#include "DoorTypes.h"
#include "Engine/LevelStreamingDynamic.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInterface.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogMainMenu, Log, All);

AMainMenuGameMode::AMainMenuGameMode()
{
	// the menu is watched, not played: no pawn and no spectator, the camera rig is the view
	DefaultPawnClass = nullptr;
	SpectatorClass = nullptr;
	PlayerControllerClass = AMainMenuPlayerController::StaticClass();

	BackgroundLevel = TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/CYB3RGUN/Maps/Lvl_DoorRange_Night.Lvl_DoorRange_Night")));
	AmbientHostileMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/CYB3RGUN/Enemies/Bodies/MI_Body_Hostile.MI_Body_Hostile")));
	AmbientFriendlyMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/CYB3RGUN/Enemies/Bodies/MI_Body_Friendly.MI_Body_Friendly")));
}

bool AMainMenuGameMode::PlayerCanRestart_Implementation(APlayerController* Player)
{
	return false;
}

void AMainMenuGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (!BackgroundLevel.IsNull())
	{
		bool bLoaded = false;
		Background = ULevelStreamingDynamic::LoadLevelInstanceBySoftObjectPtr(this, BackgroundLevel, FVector::ZeroVector, FRotator::ZeroRotator, bLoaded);
		UE_LOG(LogMainMenu, Log, TEXT("Menu background %s %s"), *BackgroundLevel.ToString(), bLoaded ? TEXT("streams in") : TEXT("could not be loaded"));
	}

	if (AmbientDoorInterval > 0.0f)
	{
		GetWorldTimerManager().SetTimer(AmbientTimer, this, &AMainMenuGameMode::OpenAmbientDoor, AmbientDoorInterval, true, AmbientDoorInterval);
	}
}

void AMainMenuGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(AmbientTimer);
	Super::EndPlay(EndPlayReason);
}

void AMainMenuGameMode::OpenAmbientDoor()
{
	// the doors arrive with the streamed background, until then there is nothing to open
	TArray<ADoorSlot*> Closed;
	for (TActorIterator<ADoorSlot> It(GetWorld()); It; ++It)
	{
		if (It->IsAvailable())
		{
			Closed.Add(*It);
		}
	}
	if (Closed.Num() == 0)
	{
		return;
	}

	FDoorOpenParams Params;
	const bool bHostile = FMath::FRand() < AmbientHostileShare;
	Params.Occupant = bHostile ? EDoorOccupant::Hostile : EDoorOccupant::Friendly;
	Params.OccupantMaterial = (bHostile ? AmbientHostileMaterial : AmbientFriendlyMaterial).LoadSynchronous();
	Params.OpenDuration = 1.2f;
	Params.TelegraphDuration = 1.0f;
	Params.ExposureWindow = AmbientExposure;
	Params.CloseDuration = 1.2f;
	Closed[FMath::RandRange(0, Closed.Num() - 1)]->Open(Params);
}
