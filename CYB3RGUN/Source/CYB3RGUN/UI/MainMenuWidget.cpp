// CYB3RGUN THEGAME. The title screen.

#include "MainMenuWidget.h"
#include "CyberMenuStyle.h"
#include "BrandFrame.h"
#include "CyberLogoWidget.h"
#include "GameMenuSubsystem.h"
#include "MainMenuGameMode.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
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
	FCyberMenuStyle::AddToColumn(Column, LogoMark, FMargin(58.0f, 0.0f, 0.0f, 24.0f));

	// the wordmark as the website sets it (D-081): Michroma, the cyan gradient, the 3 and the R in solid white, hairlines either side
	UHorizontalBox* Wordmark = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Wordmark"));
	auto AddToRow = [Wordmark](UWidget* Widget, float Left, float Right)
	{
		if (UHorizontalBoxSlot* RowSlot = Wordmark->AddChildToHorizontalBox(Widget))
		{
			RowSlot->SetVerticalAlignment(VAlign_Center);
			RowSlot->SetPadding(FMargin(Left, 0.0f, Right, 0.0f));
		}
	};
	AddToRow(FCyberMenuStyle::MakeRule(WidgetTree, 44.0f, true), 0.0f, 14.0f);
	for (const TPair<const TCHAR*, bool>& Part : { TPair<const TCHAR*, bool>(TEXT("CYB"), false), TPair<const TCHAR*, bool>(TEXT("3R"), true), TPair<const TCHAR*, bool>(TEXT("GUN"), false) })
	{
		UTextBlock* Piece = FCyberMenuStyle::MakeText(WidgetTree, NAME_None, FText::FromString(Part.Key), EBrandText::Wordmark, WordmarkSize, FLinearColor::White);
		if (Part.Value)
		{
			FSlateFontInfo Solid = Piece->GetFont();
			Solid.FontMaterial = nullptr;
			Piece->SetFont(Solid);
		}
		AddToRow(Piece, 0.0f, 0.0f);
	}
	AddToRow(FCyberMenuStyle::MakeRule(WidgetTree, 44.0f, false), 14.0f, 0.0f);
	FCyberMenuStyle::AddToColumn(Column, Wordmark, FMargin(0.0f));

	UTextBlock* Subtitle = FCyberMenuStyle::MakeText(WidgetTree, TEXT("Subtitle"), LOCTEXT("Subtitle", "DIGITAL SHOOTING CINEMA"), EBrandText::Label, 16, FCyberMenuStyle::BrandColor());
	FSlateFontInfo SubtitleFont = Subtitle->GetFont();
	SubtitleFont.LetterSpacing = 500;
	Subtitle->SetFont(SubtitleFont);
	FCyberMenuStyle::AddToColumn(Column, Subtitle, FMargin(58.0f, 6.0f, 0.0f, 10.0f));

	UTextBlock* Tagline = FCyberMenuStyle::MakeText(WidgetTree, TEXT("Tagline"), LOCTEXT("Tagline", "NO TRIGGER, NO SHOT."), EBrandText::Title, 22, FCyberMenuStyle::TextColor());
	FCyberMenuStyle::AddToColumn(Column, Tagline, FMargin(58.0f, 0.0f, 0.0f, 56.0f));

	UButton* PlayButton = AddMenuButton(TEXT("Play"), LOCTEXT("Play", "Play"), 22);
	UButton* SettingsButton = AddMenuButton(TEXT("Settings"), LOCTEXT("Settings", "Settings"), 22);
	UButton* QuitButton = AddMenuButton(TEXT("Quit"), LOCTEXT("Quit", "Quit"), 22);
	for (UButton* Button : { PlayButton, SettingsButton, QuitButton })
	{
		FCyberMenuStyle::AddToColumn(Column, FCyberMenuStyle::WrapWidth(WidgetTree, Button, 430.0f), FMargin(58.0f, 0.0f, 0.0f, 12.0f));
	}
	PlayButton->OnClicked.AddUniqueDynamic(this, &UMainMenuWidget::HandlePlay);
	SettingsButton->OnClicked.AddUniqueDynamic(this, &UMainMenuWidget::HandleSettings);
	QuitButton->OnClicked.AddUniqueDynamic(this, &UMainMenuWidget::HandleQuit);

	UTextBlock* Hint = FCyberMenuStyle::MakeText(WidgetTree, TEXT("Hint"), LOCTEXT("Hint", "Arrow keys or stick to move, Enter or A to choose"), EBrandText::Label, 12, FCyberMenuStyle::FaintTextColor());
	FCyberMenuStyle::AddToColumn(Column, Hint, FMargin(58.0f, 40.0f, 0.0f, 0.0f));
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
