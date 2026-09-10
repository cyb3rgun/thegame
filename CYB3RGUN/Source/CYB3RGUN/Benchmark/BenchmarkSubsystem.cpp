// CYB3RGUN THEGAME. Flies the benchmark route and measures frame times per graphics configuration.

#include "BenchmarkSubsystem.h"
#include "CyberGameUserSettings.h"
#include "CyberSettingsOptions.h"
#include "RailPawn.h"
#include "DynamicRHI.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/App.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "RenderTimer.h"
#if WITH_EDITOR
#include "ShaderCompiler.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogBenchmark, Log, All);

namespace Bench
{
	/** Warm up laps run this many times faster than the measured lap */
	constexpr float WarmupSpeedScale = 3.0f;

	/** Shader compilation must stay idle this long before measuring */
	constexpr float ShaderIdleNeeded = 2.0f;

	/** Give up waiting for shaders after this long and measure anyway, noting it */
	constexpr float ShaderWaitLimit = 300.0f;

	constexpr float SettleSeconds = 1.5f;

	FCyberSettingsState MakeBaseline()
	{
		FCyberSettingsState State;
		if (const UCyberGameUserSettings* Settings = UCyberGameUserSettings::Get())
		{
			State = Settings->GetState();
		}
		// every heavy feature off, the remaining scalability groups stay at High for every single feature run
		State.Preset = ECyberQualityPreset::High;
		State.Features.bMegaLights = false;
		State.Features.GlobalIllumination = ECyberGIMode::Off;
		State.Features.VirtualShadowMaps = ECyberShadowQuality::Off;
		State.Features.VolumetricFog = ECyberFogQuality::Off;
		State.Features.AntiAliasing = ECyberAntiAliasing::Off;
		State.Features.bNanite = false;
		State.Features.EffectsQuality = 0;
		State.Features.ViewDistanceQuality = 0;
		State.Features.bMotionBlur = false;
		State.FrameRateLimit = 0.0f;
		State.bVSync = false;
		State.bExperimentalNaniteSkinnedMeshes = false;
		State.bExperimentalNaniteFoliage = false;
		return State;
	}

	float Average(const TArray<float>& Values)
	{
		double Sum = 0.0;
		for (const float Value : Values)
		{
			Sum += Value;
		}
		return Values.Num() > 0 ? static_cast<float>(Sum / Values.Num()) : 0.0f;
	}

	/** Mean of the slowest one percent of frames, the usual 1 percent low */
	float SlowestPercentMean(TArray<float> Values)
	{
		if (Values.IsEmpty())
		{
			return 0.0f;
		}
		Values.Sort(TGreater<float>());
		const int32 Take = FMath::Max(1, Values.Num() / 100);
		double Sum = 0.0;
		for (int32 i = 0; i < Take; ++i)
		{
			Sum += Values[i];
		}
		return static_cast<float>(Sum / Take);
	}

	FString CVarValue(const TCHAR* Name)
	{
		const IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(Name);
		return CVar ? CVar->GetString() : TEXT("missing");
	}

	int32 RemainingShaderJobs()
	{
#if WITH_EDITOR
		return GShaderCompilingManager ? GShaderCompilingManager->GetNumRemainingJobs() : 0;
#else
		return 0;
#endif
	}
}

TArray<FString> UBenchmarkSubsystem::GetConfigNames()
{
	return {
		TEXT("baseline"),
		TEXT("megalights"), TEXT("gi_lumenlite"), TEXT("gi_lumen"),
		TEXT("vsm_low"), TEXT("vsm_high"), TEXT("vsm_epic"),
		TEXT("fog_low"), TEXT("fog_high"),
		TEXT("tsr_native"), TEXT("tsr_quality"), TEXT("tsr_balanced"), TEXT("tsr_performance"),
		TEXT("nanite"), TEXT("effects_epic"), TEXT("viewdistance_epic"), TEXT("motionblur"),
		TEXT("preset_low"), TEXT("preset_medium"), TEXT("preset_high"), TEXT("preset_epic"), TEXT("preset_ultra")
	};
}

bool UBenchmarkSubsystem::BuildConfig(const FString& Name, FCyberSettingsState& OutState)
{
	FCyberSettingsState State = Bench::MakeBaseline();
	FCyberFeatureSettings& F = State.Features;

	if (Name.StartsWith(TEXT("preset_")))
	{
		const FString Preset = Name.RightChop(7);
		int32 Index = 0;
		if (!FCyberSettingsOptions::ParseValue(ECyberSettingOption::Preset, Preset, Index))
		{
			return false;
		}
		FCyberSettingsOptions::SetValueIndex(ECyberSettingOption::Preset, State, Index);
		OutState = State;
		return true;
	}

	if (Name == TEXT("baseline")) {}
	else if (Name == TEXT("megalights")) { F.bMegaLights = true; }
	else if (Name == TEXT("gi_lumenlite")) { F.GlobalIllumination = ECyberGIMode::LumenLite; }
	else if (Name == TEXT("gi_lumen")) { F.GlobalIllumination = ECyberGIMode::Lumen; }
	else if (Name == TEXT("vsm_low")) { F.VirtualShadowMaps = ECyberShadowQuality::Low; }
	else if (Name == TEXT("vsm_high")) { F.VirtualShadowMaps = ECyberShadowQuality::High; }
	else if (Name == TEXT("vsm_epic")) { F.VirtualShadowMaps = ECyberShadowQuality::Epic; }
	else if (Name == TEXT("fog_low")) { F.VolumetricFog = ECyberFogQuality::Low; }
	else if (Name == TEXT("fog_high")) { F.VolumetricFog = ECyberFogQuality::High; }
	else if (Name == TEXT("tsr_native")) { F.AntiAliasing = ECyberAntiAliasing::TSRNative; }
	else if (Name == TEXT("tsr_quality")) { F.AntiAliasing = ECyberAntiAliasing::TSRQuality; }
	else if (Name == TEXT("tsr_balanced")) { F.AntiAliasing = ECyberAntiAliasing::TSRBalanced; }
	else if (Name == TEXT("tsr_performance")) { F.AntiAliasing = ECyberAntiAliasing::TSRPerformance; }
	else if (Name == TEXT("nanite")) { F.bNanite = true; F.VirtualShadowMaps = ECyberShadowQuality::Low; } // Nanite runs only with Virtual Shadow Maps, compare with vsm_low
	else if (Name == TEXT("effects_epic")) { F.EffectsQuality = 3; }
	else if (Name == TEXT("viewdistance_epic")) { F.ViewDistanceQuality = 3; }
	else if (Name == TEXT("motionblur")) { F.bMotionBlur = true; }
	else if (Name == TEXT("current"))
	{
		if (const UCyberGameUserSettings* Settings = UCyberGameUserSettings::Get())
		{
			State = Settings->GetState();
			State.FrameRateLimit = 0.0f;
			State.bVSync = false;
		}
	}
	else
	{
		return false;
	}

	OutState = State;
	return true;
}

void UBenchmarkSubsystem::RunQueue(const TArray<FString>& Entries, bool bInQuitWhenDone)
{
	Queue.Append(Entries);
	bQuitWhenDone = bQuitWhenDone || bInQuitWhenDone;
	if (Phase == EPhase::Idle)
	{
		bRestoreSettings = true;
		StartNext();
	}
}

ARailPawn* UBenchmarkSubsystem::FindRider() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	const APlayerController* PC = GameInstance ? GameInstance->GetFirstLocalPlayerController() : nullptr;
	return PC ? Cast<ARailPawn>(PC->GetPawn()) : nullptr;
}

void UBenchmarkSubsystem::StartNext()
{
	if (Queue.IsEmpty())
	{
		Finish();
		return;
	}

	const FString Entry = Queue[0];
	Queue.RemoveAt(0);
	if (!Entry.Split(TEXT(":"), &CurrentConfig, &CurrentLabel))
	{
		CurrentConfig = Entry;
		CurrentLabel = Entry;
	}

	FCyberSettingsState State;
	UCyberGameUserSettings* Settings = UCyberGameUserSettings::Get();
	if (!Settings || !BuildConfig(CurrentConfig, State))
	{
		UE_LOG(LogBenchmark, Warning, TEXT("Unknown benchmark configuration %s, skipped"), *CurrentConfig);
		StartNext();
		return;
	}

	// the rendering side only; nothing is saved while the suite runs
	Settings->SetState(State, false);
	UE_LOG(LogBenchmark, Log, TEXT("Benchmark %s: configuration %s applied, waiting for the rider"), *CurrentLabel, *CurrentConfig);

	Phase = EPhase::WaitForRider;
	PhaseSeconds = 0.0f;
}

void UBenchmarkSubsystem::BeginLap(ARailPawn* Rider, float Speed)
{
	Rider->SetSpeed(Speed);
	Rider->RestartRide(0.0f);
	PhaseSeconds = 0.0f;
}

void UBenchmarkSubsystem::Tick(float DeltaTime)
{
	if (Phase == EPhase::Idle)
	{
		return;
	}

	PhaseSeconds += DeltaTime;
	ARailPawn* Rider = FindRider();

	switch (Phase)
	{
	case EPhase::WaitForRider:
		if (Rider && Rider->HasActorBegunPlay())
		{
			if (RouteSpeed <= 0.0f)
			{
				RouteSpeed = Rider->GetSpeed();
			}
			BeginLap(Rider, RouteSpeed * Bench::WarmupSpeedScale);
			Phase = EPhase::Warmup;
		}
		else if (PhaseSeconds > 60.0f)
		{
			UE_LOG(LogBenchmark, Error, TEXT("Benchmark %s: no rail pawn to fly the route, is this Lvl_Benchmark?"), *CurrentLabel);
			Queue.Reset();
			Finish();
		}
		break;

	case EPhase::Warmup:
		if (!Rider || Rider->IsFinished())
		{
			Phase = EPhase::WaitShaders;
			PhaseSeconds = 0.0f;
			ShaderIdleSeconds = 0.0f;
		}
		break;

	case EPhase::WaitShaders:
		ShaderIdleSeconds = Bench::RemainingShaderJobs() == 0 ? ShaderIdleSeconds + DeltaTime : 0.0f;
		if (ShaderIdleSeconds >= Bench::ShaderIdleNeeded || PhaseSeconds > Bench::ShaderWaitLimit)
		{
			if (PhaseSeconds > Bench::ShaderWaitLimit)
			{
				UE_LOG(LogBenchmark, Warning, TEXT("Benchmark %s: shaders still compiling after %.0f s, measuring anyway"), *CurrentLabel, PhaseSeconds);
			}
			Phase = EPhase::Settle;
			PhaseSeconds = 0.0f;
		}
		break;

	case EPhase::Settle:
		if (PhaseSeconds >= Bench::SettleSeconds && Rider)
		{
			FrameMs.Reset();
			GpuMs.Reset();
			GameMs.Reset();
			BeginLap(Rider, RouteSpeed);
			Phase = EPhase::Measure;
		}
		break;

	case EPhase::Measure:
		if (!Rider || Rider->IsFinished())
		{
			Report();
			StartNext();
		}
		else
		{
			FrameMs.Add(static_cast<float>(FApp::GetDeltaTime() * 1000.0));
			GpuMs.Add(static_cast<float>(FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles(0))));
			GameMs.Add(static_cast<float>(FPlatformTime::ToMilliseconds(GGameThreadTime)));
		}
		break;

	default:
		break;
	}
}

void UBenchmarkSubsystem::Report()
{
	const float AvgMs = Bench::Average(FrameMs);
	const float LowMs = Bench::SlowestPercentMean(FrameMs);
	const float Gpu = Bench::Average(GpuMs);
	const float Game = Bench::Average(GameMs);

	FIntPoint Size(0, 0);
	if (GEngine && GEngine->GameViewport)
	{
		FVector2D ViewportSize;
		GEngine->GameViewport->GetViewportSize(ViewportSize);
		Size = FIntPoint(FMath::RoundToInt(ViewportSize.X), FMath::RoundToInt(ViewportSize.Y));
	}

	const FString Startup = FString::Printf(TEXT("skinned=%s foliage=%s assemblies=%s"),
		*Bench::CVarValue(TEXT("r.Nanite.AllowSkinnedMeshes")), *Bench::CVarValue(TEXT("r.Nanite.Foliage")), *Bench::CVarValue(TEXT("r.Nanite.AllowAssemblies")));

	UE_LOG(LogBenchmark, Log, TEXT("BENCH|%s|config=%s|frames=%d|avg_ms=%.2f|avg_fps=%.1f|low1_ms=%.2f|low1_fps=%.1f|gpu_ms=%.2f|game_ms=%.2f|res=%dx%d|screen_pct=%s|%s"),
		*CurrentLabel, *CurrentConfig, FrameMs.Num(), AvgMs, AvgMs > 0.0f ? 1000.0f / AvgMs : 0.0f, LowMs, LowMs > 0.0f ? 1000.0f / LowMs : 0.0f,
		Gpu, Game, Size.X, Size.Y, *Bench::CVarValue(TEXT("r.ScreenPercentage")), *Startup);

	const FString CsvPath = FPaths::ProjectSavedDir() / TEXT("Benchmark/bench_results.csv");
	if (!FPaths::FileExists(CsvPath))
	{
		FFileHelper::SaveStringToFile(TEXT("time,label,config,frames,avg_ms,avg_fps,low1_ms,low1_fps,gpu_ms,game_ms,width,height,startup\n"), *CsvPath);
	}
	const FString Row = FString::Printf(TEXT("%s,%s,%s,%d,%.2f,%.1f,%.2f,%.1f,%.2f,%.2f,%d,%d,%s\n"),
		*FDateTime::Now().ToString(), *CurrentLabel, *CurrentConfig, FrameMs.Num(), AvgMs, AvgMs > 0.0f ? 1000.0f / AvgMs : 0.0f,
		LowMs, LowMs > 0.0f ? 1000.0f / LowMs : 0.0f, Gpu, Game, Size.X, Size.Y, *Startup);
	FFileHelper::SaveStringToFile(Row, *CsvPath, FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get(), FILEWRITE_Append);
}

void UBenchmarkSubsystem::Finish()
{
	Phase = EPhase::Idle;
	UE_LOG(LogBenchmark, Log, TEXT("Benchmark suite finished"));

	// return to what the player saved
	if (bRestoreSettings)
	{
		if (UCyberGameUserSettings* Settings = UCyberGameUserSettings::Get())
		{
			Settings->LoadSettings(true);
			Settings->ApplyNonResolutionSettings();
		}
		bRestoreSettings = false;
	}

	if (bQuitWhenDone)
	{
		FPlatformMisc::RequestExit(false, TEXT("Bench.Suite finished"));
	}
}

TStatId UBenchmarkSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UBenchmarkSubsystem, STATGROUP_Tickables);
}

ETickableTickType UBenchmarkSubsystem::GetTickableTickType() const
{
	return HasAnyFlags(RF_ClassDefaultObject) ? ETickableTickType::Never : ETickableTickType::Always;
}

UWorld* UBenchmarkSubsystem::GetTickableGameObjectWorld() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetWorld() : nullptr;
}

#if !UE_BUILD_SHIPPING

namespace BenchCommands
{
	UBenchmarkSubsystem* Get(UWorld* World)
	{
		UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		return GameInstance ? GameInstance->GetSubsystem<UBenchmarkSubsystem>() : nullptr;
	}
}

static FAutoConsoleCommandWithWorldAndArgs GBenchRunCommand(
	TEXT("Bench.Run"),
	TEXT("Bench.Run [configuration[:label]]. Measures one configuration, the current settings without an argument. Writes a BENCH line to the log."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (UBenchmarkSubsystem* Bench = BenchCommands::Get(World))
		{
			Bench->RunQueue({ Args.Num() > 0 ? Args[0] : FString(TEXT("current")) }, false);
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GBenchSuiteCommand(
	TEXT("Bench.Suite"),
	TEXT("Bench.Suite <all|configuration[:label]...> [quit]. Measures each configuration in turn, quit exits when done."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		UBenchmarkSubsystem* Bench = BenchCommands::Get(World);
		if (!Bench)
		{
			return;
		}
		TArray<FString> Entries;
		bool bQuit = false;
		for (const FString& Arg : Args)
		{
			if (Arg.Equals(TEXT("quit"), ESearchCase::IgnoreCase))
			{
				bQuit = true;
			}
			else if (Arg.Equals(TEXT("all"), ESearchCase::IgnoreCase))
			{
				Entries.Append(UBenchmarkSubsystem::GetConfigNames());
			}
			else
			{
				Entries.Add(Arg);
			}
		}
		Bench->RunQueue(Entries, bQuit);
	}));

static FAutoConsoleCommand GBenchListCommand(
	TEXT("Bench.List"),
	TEXT("Lists the benchmark configurations."),
	FConsoleCommandDelegate::CreateLambda([]()
	{
		UE_LOG(LogBenchmark, Log, TEXT("Benchmark configurations: %s"), *FString::Join(UBenchmarkSubsystem::GetConfigNames(), TEXT(" ")));
	}));

#endif
