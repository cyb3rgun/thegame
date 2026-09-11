// CYB3RGUN THEGAME. Console commands that drive Overclock for automated verification.
// Not compiled into shipping builds.

#include "CombatFeelSubsystem.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"

#if !UE_BUILD_SHIPPING

static FAutoConsoleCommandWithWorld GFeelStatusCommand(
	TEXT("Feel.Status"),
	TEXT("Logs Overclock charge and state, world dilation and the player's scale."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		if (const UCombatFeelSubsystem* Feel = UCombatFeelSubsystem::Get(World))
		{
			Feel->LogStatus();
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GFeelOverclockCommand(
	TEXT("Feel.Overclock"),
	TEXT("1 holds the Overclock input, 0 releases it, like the E key or the left shoulder button."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (UCombatFeelSubsystem* Feel = UCombatFeelSubsystem::Get(World))
		{
			Feel->SetOverclockHeld(Args.Num() == 0 || FCString::Atoi(*Args[0]) != 0);
			Feel->LogStatus();
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GFeelChargeCommand(
	TEXT("Feel.Charge"),
	TEXT("Sets the Overclock charge, for tests."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		UCombatFeelSubsystem* Feel = UCombatFeelSubsystem::Get(World);
		if (Feel && Args.Num() > 0)
		{
			Feel->SetOverclockCharge(FCString::Atof(*Args[0]));
			Feel->LogStatus();
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GFeelDamageCommand(
	TEXT("Feel.Damage"),
	TEXT("Plays the projected HUD glitch as if the player took this much damage, 10 without an argument. No damage is dealt."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (UCombatFeelSubsystem* Feel = UCombatFeelSubsystem::Get(World))
		{
			Feel->NotifyPlayerDamaged(Args.Num() > 0 ? FCString::Atof(*Args[0]) : 10.0f);
		}
	}));

#endif
