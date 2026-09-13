// CYB3RGUN THEGAME. Console commands that drive the player's weapon for automated verification.
// Not compiled into shipping builds.

#include "WeaponStatus.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Misc/OutputDeviceNull.h"

#if !UE_BUILD_SHIPPING

DEFINE_LOG_CATEGORY_STATIC(LogWeaponDebug, Log, All);

namespace WeaponDebug
{
	APawn* GetPlayerPawn(UWorld* World)
	{
		const APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		return PC ? PC->GetPawn() : nullptr;
	}

	/** Calls one of the pawn's input handlers by name, the way the input would */
	void CallHandler(UWorld* World, const TCHAR* Handler)
	{
		if (APawn* Pawn = GetPlayerPawn(World))
		{
			FOutputDeviceNull Ar;
			Pawn->CallFunctionByNameWithArguments(Handler, Ar, nullptr, true);
		}
	}
}

static FAutoConsoleCommandWithWorld GWeaponStatusCommand(
	TEXT("Weapon.Status"),
	TEXT("Logs the local player's weapon: rounds, magazine, reload and switch state."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		const IWeaponStatusSource* Source = Cast<IWeaponStatusSource>(WeaponDebug::GetPlayerPawn(World));
		FWeaponStatus Status;
		if (Source && Source->GetWeaponStatus(Status))
		{
			UE_LOG(LogWeaponDebug, Log, TEXT("Weapon.Status: %s, %s, %d/%d, reloading %d at %.0f%%, cycling %d at %.0f%%, switching %d, weapon %d of %d"),
				*Status.WeaponName.ToString(), *Status.Subtitle.ToString(), Status.Rounds, Status.MagazineSize, Status.bReloading ? 1 : 0, Status.ReloadProgress * 100.0f,
				Status.bCycling ? 1 : 0, Status.CycleProgress * 100.0f, Status.bSwitching ? 1 : 0, Status.WeaponIndex + 1, Status.WeaponCount);
		}
		else
		{
			UE_LOG(LogWeaponDebug, Log, TEXT("Weapon.Status: the player holds no weapon"));
		}
	}));

static FAutoConsoleCommandWithWorld GWeaponReloadCommand(
	TEXT("Weapon.Reload"),
	TEXT("Presses reload for the local player, like the reload key."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		WeaponDebug::CallHandler(World, TEXT("DoReload"));
	}));

static FAutoConsoleCommandWithWorld GWeaponSwitchCommand(
	TEXT("Weapon.Switch"),
	TEXT("Switches the local player to the next weapon, like the switch key."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		WeaponDebug::CallHandler(World, TEXT("DoSwitchWeapon"));
	}));

#endif
