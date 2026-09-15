// CYB3RGUN THEGAME. The graphics settings menu.

#include "SettingsMenuWidget.h"
#include "CyberMenuStyle.h"
#include "SettingsMenuRow.h"
#include "SettingsMenuSubsystem.h"
#include "SettingsMeasuredCosts.h"
#include "CyberGameUserSettings.h"
#include "CyberSettingsOptions.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "DynamicRHI.h"
#include "Engine/GameInstance.h"
#include "Misc/App.h"
#include "BrandFrame.h"

#define LOCTEXT_NAMESPACE "SettingsMenu"

TSharedRef<SWidget> USettingsMenuWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget && !HasAnyFlags(RF_ClassDefaultObject))
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

void USettingsMenuWidget::BuildLayout()
{
	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("MenuCanvas"));
	WidgetTree->RootWidget = Canvas;

	auto MakeButton = [this](const FName& Name, const FText& Text)
	{
		return FCyberMenuStyle::MakeButton(WidgetTree, Name, Text, 16);
	};

	// the many rows need a nearly opaque plate to stay readable over any scene
	UBrandFrame* Panel = FCyberMenuStyle::MakeFrame(WidgetTree, TEXT("MenuPanel"), FMargin(36.0f, 28.0f));
	Panel->bPanelFill = false;
	Panel->SetBrushColor(FCyberMenuStyle::PanelSolidColor());
	if (UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(Panel))
	{
		PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		PanelSlot->SetAutoSize(true);
	}

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MenuColumn"));
	Panel->SetContent(Column);

	// header: title on the left, live frame rate on the right
	UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Header"));
	Header->AddChildToHorizontalBox(FCyberMenuStyle::MakeText(WidgetTree, TEXT("Title"), LOCTEXT("Title", "SETTINGS"), EBrandText::Heading, 26, FCyberMenuStyle::BrandColor()));
	if (UHorizontalBoxSlot* SpacerSlot = Header->AddChildToHorizontalBox(WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass(), TEXT("HeaderSpacer"))))
	{
		SpacerSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}
	FrameRateText = FCyberMenuStyle::MakeText(WidgetTree, TEXT("FrameRate"), FText::GetEmpty(), EBrandText::Readout, 15, FCyberMenuStyle::BrandColor());
	if (UHorizontalBoxSlot* RateSlot = Header->AddChildToHorizontalBox(FrameRateText))
	{
		RateSlot->SetVerticalAlignment(VAlign_Center);
	}
	if (UVerticalBoxSlot* HeaderSlot = Column->AddChildToVerticalBox(Header))
	{
		HeaderSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 16.0f));
	}

	// one row per option, in a scroll box for small screens
	UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("RowScroll"));
	RowBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RowBox"));
	Scroll->AddChild(RowBox);
	Column->AddChildToVerticalBox(Scroll);

	// where the grey cost figures next to the options come from
	NoteText = FCyberMenuStyle::MakeText(WidgetTree, TEXT("CostNote"), FSettingsMeasuredCosts::GetSourceNote(), EBrandText::Body, 14, FCyberMenuStyle::DimTextColor());
	if (UVerticalBoxSlot* NoteSlot = Column->AddChildToVerticalBox(NoteText))
	{
		NoteSlot->SetPadding(FMargin(0.0f, 10.0f, 0.0f, 0.0f));
	}

	for (int32 i = 0; i < static_cast<int32>(ECyberSettingOption::Count); ++i)
	{
		USettingsMenuRow* Row = CreateWidget<USettingsMenuRow>(this, USettingsMenuRow::StaticClass());
		Row->Setup(this, static_cast<ECyberSettingOption>(i));
		if (UVerticalBoxSlot* RowSlot = RowBox->AddChildToVerticalBox(Row))
		{
			RowSlot->SetPadding(FMargin(0.0f, 3.0f));
		}
		Rows.Add(Row);
	}

	StatusText = FCyberMenuStyle::MakeText(WidgetTree, TEXT("Status"), FText::GetEmpty(), EBrandText::Body, 17, FCyberMenuStyle::CounterColor());
	if (UVerticalBoxSlot* StatusSlot = Column->AddChildToVerticalBox(StatusText))
	{
		StatusSlot->SetPadding(FMargin(0.0f, 14.0f, 0.0f, 10.0f));
	}

	// footer buttons
	UHorizontalBox* Footer = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Footer"));
	ResetButton = MakeButton(TEXT("Reset"), LOCTEXT("Reset", "Reset to defaults"));
	ApplyButton = MakeButton(TEXT("Apply"), LOCTEXT("Apply", "Apply"));
	CloseButton = MakeButton(TEXT("Close"), LOCTEXT("Close", "Close"));
	for (UButton* Button : { ResetButton.Get(), ApplyButton.Get(), CloseButton.Get() })
	{
		if (UHorizontalBoxSlot* ButtonSlot = Footer->AddChildToHorizontalBox(Button))
		{
			ButtonSlot->SetPadding(FMargin(0.0f, 0.0f, 16.0f, 0.0f));
		}
	}
	Column->AddChildToVerticalBox(Footer);

	ResetButton->OnClicked.AddUniqueDynamic(this, &USettingsMenuWidget::ResetToDefaults);
	ApplyButton->OnClicked.AddUniqueDynamic(this, &USettingsMenuWidget::ApplyPending);
	CloseButton->OnClicked.AddUniqueDynamic(this, &USettingsMenuWidget::CloseMenu);
}

void USettingsMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (const UCyberGameUserSettings* Settings = UCyberGameUserSettings::Get())
	{
		Pending = Settings->GetState();
		SetStatus(Settings->IsRestartRequired() ? LOCTEXT("RestartPending", "An experimental option waits for a restart.") : FText::GetEmpty());
	}
	RefreshRows();

	if (Rows.Num() > 0 && Rows[0] && Rows[0]->GetNextButton())
	{
		Rows[0]->GetNextButton()->SetKeyboardFocus();
	}
}

void USettingsMenuWidget::StepOption(ECyberSettingOption Option, int32 Direction)
{
	if (!FCyberSettingsOptions::IsAvailable(Option, Pending))
	{
		return;
	}

	const int32 Count = FCyberSettingsOptions::GetValueCount(Option);
	if (Count <= 0)
	{
		return;
	}
	const int32 Index = (FCyberSettingsOptions::GetValueIndex(Option, Pending) + Direction + Count) % Count;
	FCyberSettingsOptions::SetValueIndex(Option, Pending, Index);

	if (Option == ECyberSettingOption::Preset && Pending.Preset == ECyberQualityPreset::Cinematic)
	{
		SetStatus(LOCTEXT("CinematicNote", "Cinematic uses the engine's film quality level, meant for screenshots and video capture, not for play."));
	}
	else
	{
		SetStatus(LOCTEXT("Unapplied", "Changes are not applied yet."));
	}
	RefreshRows();
}

void USettingsMenuWidget::ApplyPending()
{
	UCyberGameUserSettings* Settings = UCyberGameUserSettings::Get();
	if (!Settings)
	{
		return;
	}

	Settings->SetState(Pending, true);
	Pending = Settings->GetState();
	SetStatus(Settings->IsRestartRequired() ? LOCTEXT("AppliedRestart", "Applied. Experimental options take effect after a restart.") : LOCTEXT("Applied", "Applied and saved."));
	RefreshRows();
}

void USettingsMenuWidget::ResetToDefaults()
{
	if (const UCyberGameUserSettings* Settings = UCyberGameUserSettings::Get())
	{
		Pending = Settings->GetDefaultState();
		SetStatus(LOCTEXT("Defaults", "Defaults loaded. Press Apply to use them."));
		RefreshRows();
	}
}

void USettingsMenuWidget::CloseMenu()
{
	const UGameInstance* GameInstance = GetGameInstance();
	if (USettingsMenuSubsystem* Subsystem = GameInstance ? GameInstance->GetSubsystem<USettingsMenuSubsystem>() : nullptr)
	{
		Subsystem->CloseMenu();
	}
	else
	{
		RemoveFromParent();
	}
}

void USettingsMenuWidget::RefreshRows()
{
	for (USettingsMenuRow* Row : Rows)
	{
		if (!Row)
		{
			continue;
		}

		const ECyberSettingOption Option = Row->GetOption();
		const bool bAvailable = FCyberSettingsOptions::IsAvailable(Option, Pending);
		Row->SetVisibility(bAvailable ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

		const int32 ValueIndex = FCyberSettingsOptions::GetValueIndex(Option, Pending);
		FText Value = FCyberSettingsOptions::GetValueDisplayLabel(Option, ValueIndex);
		FText Cost = FSettingsMeasuredCosts::Describe(Option, ValueIndex);
		if (Option == ECyberSettingOption::Preset && Pending.Features != UCyberGameUserSettings::GetPresetFeatures(Pending.Preset))
		{
			// the plain label keeps the row short; the capture note only fits on an unmodified preset
			Value = FText::Format(LOCTEXT("Custom", "{0} (custom)"), FCyberSettingsOptions::GetValueLabel(Option, ValueIndex));
			// a measured preset time no longer describes a customised preset
			Cost = FText::GetEmpty();
		}
		if (Option == ECyberSettingOption::Nanite && Pending.Features.bNanite && !FCyberSettingsOptions::IsNaniteActive(Pending.Features))
		{
			Value = LOCTEXT("NaniteNeedsVsm", "On, needs virtual shadows");
			Cost = FText::GetEmpty();
		}
		Row->Refresh(FCyberSettingsOptions::GetOptionLabel(Option), Value, FCyberSettingsOptions::IsExperimental(Option), Cost);
	}
}

void USettingsMenuWidget::SetStatus(const FText& Text)
{
	if (StatusText)
	{
		StatusText->SetText(Text);
	}
}

void USettingsMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// the focused button is framed in the brand colour, as in every other menu
	for (UButton* Button : { ResetButton.Get(), ApplyButton.Get(), CloseButton.Get() })
	{
		FCyberMenuStyle::UpdateHighlight(Button);
	}
	for (USettingsMenuRow* Row : Rows)
	{
		if (Row)
		{
			FCyberMenuStyle::UpdateHighlight(Row->GetPrevButton());
			FCyberMenuStyle::UpdateHighlight(Row->GetNextButton());
		}
	}

	// real frame time, unaffected by pause or time dilation
	FrameAccumulator += static_cast<float>(FApp::GetDeltaTime());
	GpuAccumulator += static_cast<float>(FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles(0)));
	++FrameCount;
	if (FrameAccumulator >= FrameRateUpdateSeconds && FrameRateText)
	{
		// frames per second and frame time over the last quarter second, plus the GPU's share of the frame
		const float Ms = 1000.0f * FrameAccumulator / FrameCount;
		const float GpuMs = GpuAccumulator / FrameCount;
		FrameRateText->SetText(FText::FromString(FString::Printf(TEXT("%d FPS   %.1f ms   GPU %.1f ms"), FMath::RoundToInt(1000.0f / FMath::Max(Ms, 0.01f)), Ms, GpuMs)));
		FrameAccumulator = 0.0f;
		GpuAccumulator = 0.0f;
		FrameCount = 0;
	}
}

FReply USettingsMenuWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape || InKeyEvent.GetKey() == EKeys::Gamepad_FaceButton_Right)
	{
		CloseMenu();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

#undef LOCTEXT_NAMESPACE
