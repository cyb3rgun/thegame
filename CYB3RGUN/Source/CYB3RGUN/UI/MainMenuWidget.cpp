// CYB3RGUN THEGAME. The title screen.

#include "MainMenuWidget.h"
#include "CyberMenuStyle.h"
#include "CyberLogoWidget.h"
#include "GameMenuSubsystem.h"
#include "MainMenuGameMode.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"

#define LOCTEXT_NAMESPACE "MainMenu"

void UMainMenuWidget::BuildLayout()
{
	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("MainCanvas"));
	WidgetTree->RootWidget = Canvas;

	// a dark band on the left keeps the text readable, the rest of the screen belongs to the night range
	UBorder* Band = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Band"));
	Band->SetBrushColor(FCyberMenuStyle::PanelColor());
	Band->SetPadding(FMargin(96.0f, 0.0f, 64.0f, 0.0f));
	Band->SetVerticalAlignment(VAlign_Center);
	if (UCanvasPanelSlot* BandSlot = Canvas->AddChildToCanvas(Band))
	{
		BandSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 1.0f));
		BandSlot->SetOffsets(FMargin(0.0f, 0.0f, BandWidth, 0.0f));
	}

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MainColumn"));
	Band->SetContent(Column);

	// the mark powers on as the menu appears, its arc spins while the night range behind the menu still streams in (D-048)
	LogoMark = WidgetTree->ConstructWidget<UCyberLogoWidget>(UCyberLogoWidget::StaticClass(), TEXT("LogoMark"));
	FCyberMenuStyle::AddToColumn(Column, LogoMark, FMargin(0.0f, 0.0f, 0.0f, 24.0f));

	UTextBlock* Title = FCyberMenuStyle::MakeText(WidgetTree, TEXT("Title"), LOCTEXT("Title", "CYB3RGUN"), 96, FCyberMenuStyle::TextColor());
	Title->SetFont(FCyberMenuStyle::MakeFont(96, TEXT("Bold"), 120));
	FCyberMenuStyle::AddToColumn(Column, Title, FMargin(0.0f));

	UTextBlock* Tagline = FCyberMenuStyle::MakeText(WidgetTree, TEXT("Tagline"), LOCTEXT("Tagline", "NO TRIGGER, NO SHOT."), 22, FCyberMenuStyle::BrandColor());
	Tagline->SetFont(FCyberMenuStyle::MakeFont(22, TEXT("Bold"), 320));
	FCyberMenuStyle::AddToColumn(Column, Tagline, FMargin(4.0f, 0.0f, 0.0f, 64.0f));

	UButton* PlayButton = AddMenuButton(TEXT("Play"), LOCTEXT("Play", "Play"), 28);
	UButton* SettingsButton = AddMenuButton(TEXT("Settings"), LOCTEXT("Settings", "Settings"), 28);
	UButton* QuitButton = AddMenuButton(TEXT("Quit"), LOCTEXT("Quit", "Quit"), 28);
	for (UButton* Button : { PlayButton, SettingsButton, QuitButton })
	{
		FCyberMenuStyle::AddToColumn(Column, FCyberMenuStyle::WrapWidth(WidgetTree, Button, 380.0f), FMargin(0.0f, 0.0f, 0.0f, 12.0f));
	}
	PlayButton->OnClicked.AddUniqueDynamic(this, &UMainMenuWidget::HandlePlay);
	SettingsButton->OnClicked.AddUniqueDynamic(this, &UMainMenuWidget::HandleSettings);
	QuitButton->OnClicked.AddUniqueDynamic(this, &UMainMenuWidget::HandleQuit);

	UTextBlock* Hint = FCyberMenuStyle::MakeText(WidgetTree, TEXT("Hint"), LOCTEXT("Hint", "Arrow keys or stick to move, Enter or A to choose"), 14, FCyberMenuStyle::DimTextColor(), TEXT("Regular"));
	FCyberMenuStyle::AddToColumn(Column, Hint, FMargin(0.0f, 48.0f, 0.0f, 0.0f));
}

void UMainMenuWidget::HandlePlay()
{
	if (UGameMenuSubsystem* Menu = GetGameMenu())
	{
		Menu->OpenLevelSelect();
	}
}

void UMainMenuWidget::HandleSettings()
{
	if (UGameMenuSubsystem* Menu = GetGameMenu())
	{
		Menu->OpenSettings();
	}
}

void UMainMenuWidget::HandleQuit()
{
	if (UGameMenuSubsystem* Menu = GetGameMenu())
	{
		Menu->QuitGame();
	}
}

void UMainMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// the arc is the loading indicator: it spins until the streamed background is on screen
	if (LogoMark)
	{
		const AMainMenuGameMode* Menu = GetWorld() ? Cast<AMainMenuGameMode>(GetWorld()->GetAuthGameMode()) : nullptr;
		LogoMark->SetLoading(Menu && !Menu->IsBackgroundShown());
	}
}

#undef LOCTEXT_NAMESPACE
