// CYB3RGUN THEGAME. The front end menu stack and the keys that walk it.

#include "GameMenuSubsystem.h"
#include "CyberMenuScreen.h"
#include "CyberLogoWidget.h"
#include "LevelSelectWidget.h"
#include "MainMenuGameMode.h"
#include "MainMenuWidget.h"
#include "PauseMenuWidget.h"
#include "PlayableLevelDefinition.h"
#include "SettingsMenuSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "Engine/Console.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Framework/Application/IInputProcessor.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "GameMapsSettings.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "MoviePlayer.h"
#include "UObject/UObjectGlobals.h"
#include "Widgets/SViewport.h"

DEFINE_LOG_CATEGORY_STATIC(LogGameMenu, Log, All);

/** Reads Escape, the gamepad back button and the pause key before any widget or level input sees them */
class FGameMenuInputProcessor : public IInputProcessor
{
public:

	explicit FGameMenuInputProcessor(UGameMenuSubsystem* InOwner) : Owner(InOwner) {}

	virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override {}

	virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override
	{
		UGameMenuSubsystem* Subsystem = Owner.Get();
		return Subsystem && !InKeyEvent.IsRepeat() && Subsystem->HandleKey(InKeyEvent);
	}

	virtual const TCHAR* GetDebugName() const override { return TEXT("CYB3RGUN game menu"); }

private:

	TWeakObjectPtr<UGameMenuSubsystem> Owner;
};

void UGameMenuSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	// the settings menu registers its key reader first, so Escape closes it before it could go back a screen
	Collection.InitializeDependency<USettingsMenuSubsystem>();
	Super::Initialize(Collection);

	if (FSlateApplication::IsInitialized())
	{
		InputProcessor = MakeShared<FGameMenuInputProcessor>(this);
		FSlateApplication::Get().RegisterInputPreProcessor(InputProcessor);
	}
	PreLoadMapHandle = FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &UGameMenuSubsystem::HandlePreLoadMap);
}

void UGameMenuSubsystem::Deinitialize()
{
	FCoreUObjectDelegates::PreLoadMap.Remove(PreLoadMapHandle);
	ClearStack();
	if (InputProcessor.IsValid() && FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().UnregisterInputPreProcessor(InputProcessor);
	}
	InputProcessor.Reset();
	Super::Deinitialize();
}

bool UGameMenuSubsystem::IsPlaying() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	const UWorld* World = GameInstance ? GameInstance->GetWorld() : nullptr;
	return World && World->IsGameWorld() && World->HasBegunPlay();
}

bool UGameMenuSubsystem::IsGameplayWorld() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	const UWorld* World = GameInstance ? GameInstance->GetWorld() : nullptr;
	const AGameModeBase* GameMode = World ? World->GetAuthGameMode() : nullptr;
	return World && !(GameMode && GameMode->IsA<AMainMenuGameMode>());
}

APlayerController* UGameMenuSubsystem::GetPlayerController() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetFirstLocalPlayerController() : nullptr;
}

UCyberMenuScreen* UGameMenuSubsystem::GetTopScreen() const
{
	for (int32 Index = Stack.Num() - 1; Index >= 0; --Index)
	{
		if (IsValid(Stack[Index]))
		{
			return Stack[Index];
		}
	}
	return nullptr;
}

UCyberMenuScreen* UGameMenuSubsystem::PushScreen(const TSoftClassPtr<UCyberMenuScreen>& ScreenClass, UClass* FallbackClass)
{
	APlayerController* PC = GetPlayerController();
	if (!PC)
	{
		return nullptr;
	}

	UClass* WidgetClass = ScreenClass.LoadSynchronous();
	if (!WidgetClass)
	{
		WidgetClass = FallbackClass;
	}
	UCyberMenuScreen* Screen = CreateWidget<UCyberMenuScreen>(PC, WidgetClass);
	if (!Screen)
	{
		return nullptr;
	}

	if (UCyberMenuScreen* Previous = GetTopScreen())
	{
		Previous->SetVisibility(ESlateVisibility::Collapsed);
	}
	Stack.Add(Screen);
	// under the settings menu, which sits at 1000
	Screen->AddToViewport(900 + Stack.Num());
	ApplyMenuInput(Screen);

	UE_LOG(LogGameMenu, Log, TEXT("Menu screen %s opened, %d on the stack"), *WidgetClass->GetName(), Stack.Num());
	return Screen;
}

void UGameMenuSubsystem::ClearStack()
{
	for (UCyberMenuScreen* Screen : Stack)
	{
		if (IsValid(Screen))
		{
			Screen->RemoveFromParent();
		}
	}
	Stack.Reset();
}

void UGameMenuSubsystem::ApplyMenuInput(UCyberMenuScreen* Screen)
{
	APlayerController* PC = GetPlayerController();
	if (!PC || !Screen)
	{
		return;
	}

	// menus take every key, the level behind them gets none
	FInputModeUIOnly InputMode;
	if (UWidget* Focus = Screen->GetDefaultFocus())
	{
		InputMode.SetWidgetToFocus(Focus->TakeWidget());
	}
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PC->SetInputMode(InputMode);
	PC->SetShowMouseCursor(true);
	Screen->FocusDefault();
}

void UGameMenuSubsystem::ApplyGameInput()
{
	if (APlayerController* PC = GetPlayerController())
	{
		PC->SetInputMode(FInputModeGameOnly());
		PC->SetShowMouseCursor(false);
	}

	const UGameInstance* GameInstance = GetGameInstance();
	UWorld* World = GameInstance ? GameInstance->GetWorld() : nullptr;
	if (bPausedByMenu && World)
	{
		UGameplayStatics::SetGamePaused(World, false);
	}
	bPausedByMenu = false;
}

void UGameMenuSubsystem::ShowMainMenu()
{
	ClearStack();
	PushScreen(MainMenuClass, UMainMenuWidget::StaticClass());
}

void UGameMenuSubsystem::OpenLevelSelect()
{
	PushScreen(LevelSelectClass, ULevelSelectWidget::StaticClass());
}

void UGameMenuSubsystem::OpenPauseMenu()
{
	if (HasScreens() || !IsGameplayWorld())
	{
		return;
	}

	const UGameInstance* GameInstance = GetGameInstance();
	UWorld* World = GameInstance ? GameInstance->GetWorld() : nullptr;
	if (!PushScreen(PauseMenuClass, UPauseMenuWidget::StaticClass()))
	{
		return;
	}
	// the level waits while the pause menu shows, a rail ride or a wave does not run on without the player
	bPausedByMenu = World && !UGameplayStatics::IsGamePaused(World) && UGameplayStatics::SetGamePaused(World, true);
}

void UGameMenuSubsystem::OpenSettings()
{
	const UGameInstance* GameInstance = GetGameInstance();
	if (USettingsMenuSubsystem* Settings = GameInstance ? GameInstance->GetSubsystem<USettingsMenuSubsystem>() : nullptr)
	{
		Settings->OpenMenu();
	}
}

bool UGameMenuSubsystem::Back()
{
	Stack.RemoveAll([](const TObjectPtr<UCyberMenuScreen>& Screen) { return !IsValid(Screen); });
	UCyberMenuScreen* Top = GetTopScreen();
	if (!Top || !Top->CanGoBack())
	{
		return false;
	}

	Stack.Pop();
	Top->RemoveFromParent();

	if (UCyberMenuScreen* Next = GetTopScreen())
	{
		Next->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		ApplyMenuInput(Next);
	}
	else
	{
		ApplyGameInput();
	}

	UE_LOG(LogGameMenu, Log, TEXT("Menu back, %d on the stack"), Stack.Num());
	return true;
}

void UGameMenuSubsystem::SuspendTop()
{
	if (UCyberMenuScreen* Top = GetTopScreen())
	{
		Top->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UGameMenuSubsystem::ResumeTop()
{
	if (UCyberMenuScreen* Top = GetTopScreen())
	{
		Top->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		ApplyMenuInput(Top);
	}
}

void UGameMenuSubsystem::StartLevel(const UPlayableLevelDefinition* Level)
{
	const UGameInstance* GameInstance = GetGameInstance();
	UWorld* World = GameInstance ? GameInstance->GetWorld() : nullptr;
	if (!World || !Level || Level->Map.IsNull())
	{
		return;
	}

	UE_LOG(LogGameMenu, Log, TEXT("Starting %s, map %s"), *Level->GetDisplayText().ToString(), *Level->Map.ToString());
	ClearStack();
	ApplyGameInput();
	UGameplayStatics::OpenLevelBySoftObjectPtr(World, Level->Map);
}

void UGameMenuSubsystem::ReturnToMainMenu()
{
	const UGameInstance* GameInstance = GetGameInstance();
	UWorld* World = GameInstance ? GameInstance->GetWorld() : nullptr;
	const FSoftObjectPath MenuMap(UGameMapsSettings::GetGameDefaultMap());
	if (!World || MenuMap.IsNull())
	{
		return;
	}

	UE_LOG(LogGameMenu, Log, TEXT("Returning to the main menu, map %s"), *MenuMap.ToString());
	ClearStack();
	ApplyGameInput();
	UGameplayStatics::OpenLevelBySoftObjectPtr(World, TSoftObjectPtr<UWorld>(MenuMap));
}

void UGameMenuSubsystem::QuitGame()
{
	const UGameInstance* GameInstance = GetGameInstance();
	UE_LOG(LogGameMenu, Log, TEXT("Quit from the main menu"));
	UKismetSystemLibrary::QuitGame(GameInstance ? GameInstance->GetWorld() : nullptr, GetPlayerController(), EQuitPreference::Quit, false);
}

void UGameMenuSubsystem::HandlePreLoadMap(const FString& MapName)
{
	// the screens belong to the world that is being left
	ClearStack();
	bPausedByMenu = false;

	// the spinning arc of the mark covers the load. The movie player draws it on its own thread in a standalone game, the
	// editor has no loading screen (D-048)
	if (!GIsEditor && !IsRunningDedicatedServer())
	{
		FLoadingScreenAttributes LoadingScreen;
		LoadingScreen.bAutoCompleteWhenLoadingCompletes = true;
		LoadingScreen.MinimumLoadingScreenDisplayTime = 0.5f;
		LoadingScreen.WidgetLoadingScreen = SNew(SCyberLoadingScreen);
		GetMoviePlayer()->SetupLoadingScreen(LoadingScreen);
	}
}

bool UGameMenuSubsystem::IsViewportFocused() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	const UGameViewportClient* Viewport = GameInstance ? GameInstance->GetGameViewportClient() : nullptr;
	const TSharedPtr<SViewport> ViewportWidget = Viewport ? Viewport->GetGameViewportWidget() : nullptr;
	const TSharedPtr<SWidget> Focused = FSlateApplication::Get().GetKeyboardFocusedWidget();
	return !Focused.IsValid() || (ViewportWidget.IsValid() && Focused.Get() == ViewportWidget.Get());
}

bool UGameMenuSubsystem::HandleKey(const FKeyEvent& KeyEvent)
{
	if (!IsPlaying())
	{
		return false;
	}

	UGameInstance* GameInstance = GetGameInstance();

	// the settings menu reads its keys first and keeps them while it is open
	const USettingsMenuSubsystem* Settings = GameInstance->GetSubsystem<USettingsMenuSubsystem>();
	if (Settings && Settings->IsMenuOpen())
	{
		return false;
	}

	// the developer console keeps Escape for closing itself
	const UGameViewportClient* Viewport = GameInstance->GetGameViewportClient();
	if (Viewport && Viewport->ViewportConsole && Viewport->ViewportConsole->ConsoleActive())
	{
		return false;
	}

	const FKey Key = KeyEvent.GetKey();
	if (UCyberMenuScreen* Top = GetTopScreen())
	{
		if (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right)
		{
			// one screen back at most: the root main menu stays, Escape never leaves the game
			Back();
			return true;
		}

		// after a click beside the buttons, the first navigation key brings the focus back
		const bool bNavigation = Key == EKeys::Up || Key == EKeys::Down || Key == EKeys::Left || Key == EKeys::Right || Key == EKeys::Tab
			|| Key == EKeys::Gamepad_DPad_Up || Key == EKeys::Gamepad_DPad_Down || Key == EKeys::Gamepad_DPad_Left || Key == EKeys::Gamepad_DPad_Right
			|| Key == EKeys::Gamepad_LeftStick_Up || Key == EKeys::Gamepad_LeftStick_Down || Key == EKeys::Gamepad_LeftStick_Left || Key == EKeys::Gamepad_LeftStick_Right;
		if (bNavigation && !Top->HasFocusedDescendants() && IsViewportFocused())
		{
			Top->FocusDefault();
			return true;
		}
		return false;
	}

	if (Key == EKeys::Escape || Key == EKeys::Gamepad_Special_Left)
	{
		// outside a gameplay level Escape does nothing, it never quits
		OpenPauseMenu();
		return true;
	}
	return false;
}

#if !UE_BUILD_SHIPPING

static FAutoConsoleCommandWithWorld GGameMenuBackCommand(
	TEXT("Menu.Back"),
	TEXT("Leaves the top menu screen, like Escape."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		if (UGameMenuSubsystem* Subsystem = GameInstance ? GameInstance->GetSubsystem<UGameMenuSubsystem>() : nullptr)
		{
			Subsystem->Back();
		}
	}));

static FAutoConsoleCommandWithWorld GGameMenuPauseCommand(
	TEXT("Menu.Pause"),
	TEXT("Opens the pause menu in a gameplay level, like Escape."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		if (UGameMenuSubsystem* Subsystem = GameInstance ? GameInstance->GetSubsystem<UGameMenuSubsystem>() : nullptr)
		{
			Subsystem->OpenPauseMenu();
		}
	}));

#endif
