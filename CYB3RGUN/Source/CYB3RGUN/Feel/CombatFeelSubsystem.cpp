// CYB3RGUN THEGAME. Hit stop, camera kicks, the kill flash and Overclock for the local player.

#include "CombatFeelSubsystem.h"
#include "CombatFeelCameraModifier.h"
#include "StyleSettings.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogCombatFeel, Log, All);

UCombatFeelSubsystem* UCombatFeelSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	return World ? World->GetSubsystem<UCombatFeelSubsystem>() : nullptr;
}

float UCombatFeelSubsystem::GetPlayerTimeScale(const UObject* WorldContext)
{
	const UCombatFeelSubsystem* Feel = Get(WorldContext);
	return Feel ? Feel->GetPlayerTimeScale() : 1.0f;
}

TStatId UCombatFeelSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UCombatFeelSubsystem, STATGROUP_Tickables);
}

bool UCombatFeelSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UCombatFeelSubsystem::Deinitialize()
{
	ReleasePawn();
	Super::Deinitialize();
}

const UStyleSettings* UCombatFeelSubsystem::GetSettings() const
{
	return UStyleSettings::Get(GetWorld());
}

bool UCombatFeelSubsystem::IsOverclockAllowed() const
{
	return GetSettings()->bOverclockAllowed;
}

bool UCombatFeelSubsystem::IsOverclockReady() const
{
	return IsOverclockAllowed() && OverclockCharge >= GetSettings()->OverclockMinToStart;
}

float UCombatFeelSubsystem::GetOverclockFraction() const
{
	return FMath::Clamp(OverclockCharge / FMath::Max(GetSettings()->OverclockMax, 1.0f), 0.0f, 1.0f);
}

bool UCombatFeelSubsystem::IsHitStopping() const
{
	return FPlatformTime::Seconds() < HitStopUntil;
}

void UCombatFeelSubsystem::SetOverclockHeld(bool bHeld)
{
	bOverclockHeld = bHeld;
}

void UCombatFeelSubsystem::SetOverclockCharge(float Charge)
{
	OverclockCharge = FMath::Clamp(Charge, 0.0f, GetSettings()->OverclockMax);
}

void UCombatFeelSubsystem::LogStatus() const
{
	const AWorldSettings* WorldSettings = GetWorld()->GetWorldSettings();
	const APawn* Pawn = DilatedPawn.Get();
	UE_LOG(LogCombatFeel, Display, TEXT("Feel: overclock %s, charge %.0f of %.0f, held %d, allowed %d, world dilation %.2f, player scale %.2f, pawn dilation %.2f"),
		bOverclockActive ? TEXT("on") : TEXT("off"), OverclockCharge, GetSettings()->OverclockMax, bOverclockHeld ? 1 : 0, IsOverclockAllowed() ? 1 : 0,
		WorldSettings ? WorldSettings->TimeDilation : 1.0f, PlayerTimeScale, Pawn ? Pawn->CustomTimeDilation : 1.0f);
}

void UCombatFeelSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// every effect here runs on the wall clock; a long gap, a pause or a hitch, counts as one short frame
	const double Now = FPlatformTime::Seconds();
	const float RealDelta = LastTickWallSeconds > 0.0 ? static_cast<float>(FMath::Min(Now - LastTickWallSeconds, 0.1)) : 0.0f;
	LastTickWallSeconds = Now;

	BindLocalPlayer();
	UpdateOverclock(RealDelta);
	ApplyTime(RealDelta);
}

void UCombatFeelSubsystem::BindLocalPlayer()
{
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC || !PC->IsLocalController())
	{
		return;
	}

	if (LocalController.Get() != PC)
	{
		LocalController = PC;
		BoundStyle = nullptr;
		CameraModifier = nullptr;
	}

	if (!BoundStyle.IsValid())
	{
		if (UStyleScoringComponent* Style = UStyleScoringComponent::Get(PC))
		{
			Style->OnStyleEvent.AddUniqueDynamic(this, &UCombatFeelSubsystem::HandleStyleEvent);
			BoundStyle = Style;
		}
	}

	if (!CameraModifier.IsValid() && PC->PlayerCameraManager)
	{
		UCameraModifier* Existing = PC->PlayerCameraManager->FindCameraModifierByClass(UCombatFeelCameraModifier::StaticClass());
		CameraModifier = Cast<UCombatFeelCameraModifier>(Existing ? Existing : PC->PlayerCameraManager->AddNewCameraModifier(UCombatFeelCameraModifier::StaticClass()));
	}

	// the player scale follows possession
	if (DilatedPawn.Get() != PC->GetPawn())
	{
		ReleasePawn();
		DilatedPawn = PC->GetPawn();
	}
}

void UCombatFeelSubsystem::ReleasePawn()
{
	if (APawn* Pawn = DilatedPawn.Get())
	{
		Pawn->CustomTimeDilation = 1.0f;
	}
	DilatedPawn = nullptr;
}

void UCombatFeelSubsystem::HandleStyleEvent(EStyleEvent Event, int32 Points, AActor* Target)
{
	const UStyleSettings* Settings = GetSettings();
	float Charge = 0.0f;

	switch (Event)
	{
	case EStyleEvent::Hit:
		Charge = Settings->OverclockPerHit;
		Kick(Settings->LightShake, Target);
		break;
	case EStyleEvent::Kill:
		Charge = Settings->OverclockPerKill;
		StartHitStop(Settings->HitStopSeconds);
		Kick(Settings->MediumShake, Target);
		if (UCombatFeelCameraModifier* Modifier = CameraModifier.Get())
		{
			Modifier->Flash(Settings->KillFlashSeconds, Settings->KillFlashStrength);
		}
		break;
	case EStyleEvent::Headshot:
		// follows the kill it belongs to: the stop runs a little longer and the heavy kick joins in
		Charge = Settings->OverclockPerHeadshot;
		StartHitStop(Settings->HeadshotHitStopSeconds);
		Kick(Settings->HeavyShake, Target);
		break;
	case EStyleEvent::ControlledPair:
		Charge = Settings->OverclockPerControlledPair;
		break;
	case EStyleEvent::Rescue:
		Charge = Settings->OverclockPerRescue;
		Kick(Settings->HeavyShake, Target);
		break;
	case EStyleEvent::Penalty:
		// a hit on an innocent ends Overclock and empties it, like the meter (D-044)
		OverclockCharge = 0.0f;
		bOverclockActive = false;
		Kick(Settings->HeavyShake, Target);
		break;
	default:
		break;
	}

	if (Charge > 0.0f && Settings->bOverclockAllowed)
	{
		OverclockCharge = FMath::Min(OverclockCharge + Charge, Settings->OverclockMax);
	}
}

void UCombatFeelSubsystem::StartHitStop(float Seconds)
{
	// the stop starts on the next tick, when the world is actually held
	PendingHitStopSeconds = FMath::Max(PendingHitStopSeconds, Seconds);
}

void UCombatFeelSubsystem::Kick(const FStyleShake& Shake, AActor* Target)
{
	UCombatFeelCameraModifier* Modifier = CameraModifier.Get();
	const APlayerController* PC = LocalController.Get();
	if (!Modifier || !PC || !PC->PlayerCameraManager || Shake.Degrees <= 0.0f)
	{
		return;
	}

	// the kick turns the view a little toward where the target stands, and upward when it is dead ahead
	FVector2D Direction(0.0f, 1.0f);
	if (Target)
	{
		const FVector Local = PC->PlayerCameraManager->GetCameraRotation().UnrotateVector(Target->GetActorLocation() - PC->PlayerCameraManager->GetCameraLocation()).GetSafeNormal();
		const FVector2D Toward(Local.Y, Local.Z + 0.5f);
		if (!Toward.IsNearlyZero())
		{
			Direction = Toward.GetSafeNormal();
		}
	}
	Modifier->AddKick(Direction, Shake.Degrees, Shake.DecaySeconds, Shake.Frequency);
}

void UCombatFeelSubsystem::UpdateOverclock(float RealDelta)
{
	const UStyleSettings* Settings = GetSettings();
	if (!Settings->bOverclockAllowed)
	{
		bOverclockActive = false;
		OverclockCharge = 0.0f;
		return;
	}

	if (!bOverclockActive && bOverclockHeld && OverclockCharge >= Settings->OverclockMinToStart)
	{
		bOverclockActive = true;
		UE_LOG(LogCombatFeel, Log, TEXT("Overclock on with %.0f charge"), OverclockCharge);
	}

	if (bOverclockActive)
	{
		OverclockCharge = FMath::Max(OverclockCharge - Settings->OverclockDrainPerSecond * RealDelta, 0.0f);
		if (!bOverclockHeld || OverclockCharge <= 0.0f)
		{
			bOverclockActive = false;
			UE_LOG(LogCombatFeel, Log, TEXT("Overclock off with %.0f charge"), OverclockCharge);
		}
	}
}

void UCombatFeelSubsystem::ApplyTime(float RealDelta)
{
	const UStyleSettings* Settings = GetSettings();
	AWorldSettings* WorldSettings = GetWorld()->GetWorldSettings();
	if (!WorldSettings)
	{
		return;
	}

	// ease into and out of the Overclock slowdown
	const float BlendStep = Settings->OverclockBlendSeconds > 0.0f ? RealDelta / Settings->OverclockBlendSeconds : 1.0f;
	OverclockBlend = FMath::Clamp(OverclockBlend + (bOverclockActive ? BlendStep : -BlendStep), 0.0f, 1.0f);

	float WorldDilation = FMath::Lerp(1.0f, Settings->OverclockWorldDilation, OverclockBlend);
	PlayerTimeScale = FMath::Lerp(1.0f, Settings->OverclockPlayerScale, OverclockBlend);

	// a hit stop holds everything, the player included. Its clock starts here, on the first frame it holds
	const double Now = FPlatformTime::Seconds();
	if (PendingHitStopSeconds > 0.0f)
	{
		if (Now >= HitStopUntil)
		{
			HitStopStarted = Now;
		}
		HitStopUntil = FMath::Max(HitStopUntil, Now + PendingHitStopSeconds);
		PendingHitStopSeconds = 0.0f;
		bHitStopLogged = false;
	}
	if (Now < HitStopUntil)
	{
		WorldDilation = Settings->HitStopDilation;
		PlayerTimeScale = Settings->HitStopDilation;
	}
	else if (!bHitStopLogged)
	{
		bHitStopLogged = true;
		UE_LOG(LogCombatFeel, Verbose, TEXT("Hit stop held the world for %.0f ms"), (Now - HitStopStarted) * 1000.0);
	}

	if (!FMath::IsNearlyEqual(WorldSettings->TimeDilation, WorldDilation, 0.0001f))
	{
		UGameplayStatics::SetGlobalTimeDilation(GetWorld(), WorldDilation);
	}

	// the player's pawn runs at the player scale: its own dilation undoes the world's
	if (APawn* Pawn = DilatedPawn.Get())
	{
		Pawn->CustomTimeDilation = WorldDilation > KINDA_SMALL_NUMBER ? PlayerTimeScale / WorldDilation : 1.0f;
	}

	if (UCombatFeelCameraModifier* Modifier = CameraModifier.Get())
	{
		Modifier->SetOverclockLook(OverclockBlend * Settings->OverclockLookStrength);
	}
}

void UCombatFeelSubsystem::NotifyPlayerDamaged(float Amount)
{
	if (Amount > 0.0f)
	{
		DamageGlitchAt = FPlatformTime::Seconds();
		UE_LOG(LogCombatFeel, Verbose, TEXT("The player took %.0f damage, the HUD glitches"), Amount);
	}
}

float UCombatFeelSubsystem::GetDamageGlitch() const
{
	const float Seconds = UStyleSettings::Get(this)->DamageGlitchSeconds;
	if (Seconds <= 0.0f)
	{
		return 0.0f;
	}
	const float Age = static_cast<float>(FPlatformTime::Seconds() - DamageGlitchAt);
	return FMath::Clamp(1.0f - Age / Seconds, 0.0f, 1.0f);
}
