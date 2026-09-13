// CYB3RGUN THEGAME. Console command that damages the local player for automated verification.
// Not compiled into shipping builds.

#include "Engine/World.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"

#if !UE_BUILD_SHIPPING

DEFINE_LOG_CATEGORY_STATIC(LogPlayerDebug, Log, All);

static FAutoConsoleCommandWithWorldAndArgs GPlayerDamageCommand(
	TEXT("Player.Damage"),
	TEXT("Deals damage to the local player through the engine damage path. Argument: amount, 100 without one."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		const APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		APawn* Pawn = PC ? PC->GetPawn() : nullptr;
		if (!Pawn)
		{
			UE_LOG(LogPlayerDebug, Log, TEXT("Player.Damage: no player pawn"));
			return;
		}
		const float Amount = Args.Num() > 0 ? FCString::Atof(*Args[0]) : 100.0f;
		const float Applied = UGameplayStatics::ApplyDamage(Pawn, Amount, nullptr, nullptr, UDamageType::StaticClass());
		UE_LOG(LogPlayerDebug, Log, TEXT("Player.Damage: %.0f asked, %.0f taken by %s"), Amount, Applied, *Pawn->GetName());
	}));

#endif
