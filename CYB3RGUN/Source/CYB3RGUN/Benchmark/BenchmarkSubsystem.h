// CYB3RGUN THEGAME. Flies the benchmark route and measures frame times per graphics configuration (D-026).

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "CyberSettingsTypes.h"
#include "BenchmarkSubsystem.generated.h"

class ARailPawn;

/**
 *  Runs a queue of named configurations. Each configuration is applied through the settings object, then
 *  the rail pawn flies the route once fast to warm shaders, the subsystem waits for shader compilation to
 *  settle, and a second lap at the route speed is measured. Results go to the log as one BENCH line each
 *  and are appended to Saved/Benchmark/bench_results.csv.
 */
UCLASS()
class CYB3RGUN_API UBenchmarkSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:

	/** Every configuration the suite knows, in suite order */
	static TArray<FString> GetConfigNames();

	/** The night run (D-032): the six presets and Ultra at 150 and 200 percent resolution scale, each between two baseline laps */
	static TArray<FString> GetNightSuite();

	/** Builds a named configuration on top of the benchmark baseline. False for an unknown name. */
	static bool BuildConfig(const FString& Name, FCyberSettingsState& OutState);

	/** Queues configurations. A queue entry may be name:label to report under another label. */
	void RunQueue(const TArray<FString>& Entries, bool bQuitWhenDone);

	bool IsRunning() const { return Phase != EPhase::Idle; }

	//~ FTickableGameObject
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual ETickableTickType GetTickableTickType() const override;
	virtual bool IsTickableWhenPaused() const override { return false; }
	virtual UWorld* GetTickableGameObjectWorld() const override;

protected:

	enum class EPhase : uint8
	{
		Idle,
		WaitForRider,
		Warmup,
		WaitShaders,
		Settle,
		Measure
	};

	EPhase Phase = EPhase::Idle;
	TArray<FString> Queue;
	FString CurrentConfig;
	FString CurrentLabel;
	bool bQuitWhenDone = false;
	float PhaseSeconds = 0.0f;
	float ShaderIdleSeconds = 0.0f;
	float RouteSpeed = 0.0f;
	bool bRestoreSettings = false;

	TArray<float> FrameMs;
	TArray<float> GpuMs;
	TArray<float> GameMs;

	ARailPawn* FindRider() const;
	void StartNext();
	void BeginLap(ARailPawn* Rider, float Speed);
	void Report();
	void Finish();
};
