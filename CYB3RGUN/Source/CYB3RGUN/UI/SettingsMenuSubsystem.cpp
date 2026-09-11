// CYB3RGUN THEGAME. Opens the settings menu from any level with one key.

#include "SettingsMenuSubsystem.h"
#include "SettingsMenuWidget.h"
#include "GameMenuSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Framework/Application/IInputProcessor.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogSettingsMenu, Log, All);

/** Watches every key press before widgets see it, so the menu key works with any controller and any focus */
class FSettingsMenuInputProcessor : public IInputProcessor
{
public:

	explicit FSettingsMenuInputProcessor(USettingsMenuSubsystem* InOwner) : Owner(InOwner) {}

	virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override {}

	virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override
	{
		USettingsMenuSubsystem* Subsystem = Owner.Get();
		if (!Subsystem || !Subsystem->IsPlaying() || InKeyEvent.IsRepeat())
		{
			return false;
		}

		const FKey Key = InKeyEvent.GetKey();
		if (Key == EKeys::F10 || Key == EKeys::Gamepad_Special_Right)
		{
			Subsystem->ToggleMenu();
			return true;
		}
		if (Key == EKeys::Escape && Subsystem->IsMenuOpen())
		{
			Subsystem->CloseMenu();
			return true;
		}
		return false;
	}

	virtual const TCHAR* GetDebugName() const override { return TEXT("CYB3RGUN settings menu"); }

private:

	TWeakObjectPtr<USettingsMenuSubsystem> Owner;
};

void USettingsMenuSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (FSlateApplication::IsInitialized())
	{
		InputProcessor = MakeShared<FSettingsMenuInputProcessor>(this);
		FSlateApplication::Get().RegisterInputPreProcessor(InputProcessor);
	}
}

void USettingsMenuSubsystem::Deinitialize()
{
	CloseMenu();
	if (InputProcessor.IsValid() && FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().UnregisterInputPreProcessor(InputProcessor);
	}
	InputProcessor.Reset();
	Super::Deinitialize();
}

bool USettingsMenuSubsystem::IsPlaying() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	const UWorld* World = GameInstance ? GameInstance->GetWorld() : nullptr;
	return World && World->IsGameWorld() && World->HasBegunPlay();
}

void USettingsMenuSubsystem::ToggleMenu()
{
	if (IsMenuOpen())
	{
		CloseMenu();
	}
	else
	{
		OpenMenu();
	}
}

void USettingsMenuSubsystem::OpenMenu()
{
	UGameInstance* GameInstance = GetGameInstance();
	APlayerController* PC = GameInstance ? GameInstance->GetFirstLocalPlayerController() : nullptr;
	if (Menu || !PC)
	{
		return;
	}

	UClass* WidgetClass = MenuClass.LoadSynchronous();
	if (!WidgetClass)
	{
		WidgetClass = USettingsMenuWidget::StaticClass();
	}

	Menu = CreateWidget<USettingsMenuWidget>(PC, WidgetClass);
	if (!Menu)
	{
		return;
	}
	Menu->AddToViewport(1000);

	// over a menu screen, that screen steps aside and comes back when the settings close
	UGameMenuSubsystem* GameMenu = GameInstance->GetSubsystem<UGameMenuSubsystem>();
	if (GameMenu && GameMenu->HasScreens())
	{
		GameMenu->SuspendTop();
	}

	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(Menu->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	PC->SetInputMode(InputMode);
	PC->SetShowMouseCursor(true);

	// the world waits while the player reads the menu, so a rail ride or a wave does not run on without them,
	// only the main menu's background keeps moving
	UWorld* World = GameInstance->GetWorld();
	const bool bMayPause = !GameMenu || GameMenu->IsGameplayWorld();
	bPausedByMenu = bMayPause && World && !UGameplayStatics::IsGamePaused(World) && UGameplayStatics::SetGamePaused(World, true);

	UE_LOG(LogSettingsMenu, Log, TEXT("Settings menu opened with %s"), *WidgetClass->GetName());
}

void USettingsMenuSubsystem::CloseMenu()
{
	if (!Menu)
	{
		return;
	}

	Menu->RemoveFromParent();
	Menu = nullptr;

	UGameInstance* GameInstance = GetGameInstance();
	UGameMenuSubsystem* GameMenu = GameInstance ? GameInstance->GetSubsystem<UGameMenuSubsystem>() : nullptr;
	if (GameMenu && GameMenu->HasScreens())
	{
		// back to the menu screen the settings were opened from
		GameMenu->ResumeTop();
	}
	else if (APlayerController* PC = GameInstance ? GameInstance->GetFirstLocalPlayerController() : nullptr)
	{
		PC->SetInputMode(FInputModeGameOnly());
		PC->SetShowMouseCursor(false);
	}

	UWorld* World = GameInstance ? GameInstance->GetWorld() : nullptr;
	if (bPausedByMenu && World)
	{
		UGameplayStatics::SetGamePaused(World, false);
	}
	bPausedByMenu = false;

	UE_LOG(LogSettingsMenu, Log, TEXT("Settings menu closed"));
}

#if !UE_BUILD_SHIPPING

static FAutoConsoleCommandWithWorld GSettingsMenuCommand(
	TEXT("Settings.Menu"),
	TEXT("Opens or closes the settings menu, like F10."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		if (USettingsMenuSubsystem* Subsystem = GameInstance ? GameInstance->GetSubsystem<USettingsMenuSubsystem>() : nullptr)
		{
			Subsystem->ToggleMenu();
		}
	}));

#endif
