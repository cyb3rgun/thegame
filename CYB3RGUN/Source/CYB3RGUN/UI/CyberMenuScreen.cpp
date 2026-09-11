// CYB3RGUN THEGAME. Base of the front end screens.

#include "CyberMenuScreen.h"
#include "CyberMenuStyle.h"
#include "GameMenuSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Engine/GameInstance.h"
#include "Framework/Application/SlateApplication.h"

TSharedRef<SWidget> UCyberMenuScreen::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget && !HasAnyFlags(RF_ClassDefaultObject))
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

void UCyberMenuScreen::NativeConstruct()
{
	Super::NativeConstruct();
	FocusDefault();
}

void UCyberMenuScreen::FocusDefault()
{
	// a screen that was just added or shown again may not take the focus in this frame, NativeTick asks again
	FocusAttempts = 10;
	// wherever the cursor rests now is where it starts, only moving it from here counts
	bCursorKnown = false;
	if (DefaultFocus)
	{
		DefaultFocus->SetKeyboardFocus();
	}
}

UWidget* UCyberMenuScreen::GetDefaultFocus() const
{
	return DefaultFocus;
}

void UCyberMenuScreen::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (FocusAttempts > 0)
	{
		--FocusAttempts;
		if (HasFocusedDescendants())
		{
			FocusAttempts = 0;
		}
		else if (DefaultFocus)
		{
			DefaultFocus->SetKeyboardFocus();
		}
	}

	// only a mouse that moves takes the focus, a resting one leaves it where the keys put it
	bool bMouseMoved = false;
	if (FSlateApplication::IsInitialized())
	{
		const FVector2D CursorPosition = FSlateApplication::Get().GetCursorPos();
		bMouseMoved = bCursorKnown && !CursorPosition.Equals(LastCursorPosition, 1.0);
		LastCursorPosition = CursorPosition;
		bCursorKnown = true;
	}

	for (UButton* Button : MenuButtons)
	{
		if (!Button)
		{
			continue;
		}
		if (bMouseMoved && Button->IsHovered() && !Button->HasKeyboardFocus())
		{
			Button->SetKeyboardFocus();
		}
		FCyberMenuStyle::UpdateHighlight(Button);
	}
}

UButton* UCyberMenuScreen::AddMenuButton(const FName& Name, const FText& Label, int32 Size)
{
	UButton* Button = FCyberMenuStyle::MakeButton(WidgetTree, Name, Label, Size);
	RegisterButton(Button);
	return Button;
}

void UCyberMenuScreen::RegisterButton(UButton* Button)
{
	if (!Button)
	{
		return;
	}
	MenuButtons.AddUnique(Button);
	if (!DefaultFocus)
	{
		DefaultFocus = Button;
	}
}

UGameMenuSubsystem* UCyberMenuScreen::GetGameMenu() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<UGameMenuSubsystem>() : nullptr;
}
