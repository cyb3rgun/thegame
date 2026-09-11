// CYB3RGUN THEGAME. Effects and lights for shots and hits, adjustable under Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "ShotFeedbackSettings.generated.h"

class UNiagaraSystem;
class UMaterialInterface;

/**
 *  What a shot looks like: a muzzle flash with a short bright light where the shot leaves, sparks with a
 *  brief light where it lands. Both lights are movable, unshadowed point lights that fade out by themselves.
 */
UCLASS(Config=Game, DefaultConfig, meta = (DisplayName = "Shot Feedback"))
class CYB3RGUN_API UShotFeedbackSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:

	UShotFeedbackSettings();

	/** Off turns every muzzle flash and impact effect off, lights included */
	UPROPERTY(Config, EditAnywhere, Category="Feedback")
	bool bEnabled = true;

	UPROPERTY(Config, EditAnywhere, Category="Muzzle")
	TSoftObjectPtr<UNiagaraSystem> MuzzleFlashSystem;

	UPROPERTY(Config, EditAnywhere, Category="Muzzle")
	FLinearColor MuzzleLightColor = FLinearColor(1.0f, 0.62f, 0.3f);

	/** Brightness of the muzzle light at the moment of the shot */
	UPROPERTY(Config, EditAnywhere, Category="Muzzle", meta = (ClampMin = 0.0, Units = "Lumens"))
	float MuzzleLightIntensity = 6000.0f;

	UPROPERTY(Config, EditAnywhere, Category="Muzzle", meta = (ClampMin = 10.0, Units = "cm"))
	float MuzzleLightRadius = 700.0f;

	/** Seconds from full brightness to dark */
	UPROPERTY(Config, EditAnywhere, Category="Muzzle", meta = (ClampMin = 0.01, Units = "s"))
	float MuzzleLightDuration = 0.07f;

	UPROPERTY(Config, EditAnywhere, Category="Impact")
	TSoftObjectPtr<UNiagaraSystem> ImpactSystem;

	UPROPERTY(Config, EditAnywhere, Category="Impact")
	FLinearColor ImpactLightColor = FLinearColor(1.0f, 0.55f, 0.22f);

	/** Brightness of the hit light at the moment of the hit */
	UPROPERTY(Config, EditAnywhere, Category="Impact", meta = (ClampMin = 0.0, Units = "Lumens"))
	float ImpactLightIntensity = 2000.0f;

	UPROPERTY(Config, EditAnywhere, Category="Impact", meta = (ClampMin = 10.0, Units = "cm"))
	float ImpactLightRadius = 300.0f;

	/** Seconds from full brightness to dark */
	UPROPERTY(Config, EditAnywhere, Category="Impact", meta = (ClampMin = 0.01, Units = "s"))
	float ImpactLightDuration = 0.12f;

	/** Distance the hit light sits off the surface, so it lights the surface instead of sitting inside it */
	UPROPERTY(Config, EditAnywhere, Category="Impact", meta = (ClampMin = 0.0, Units = "cm"))
	float ImpactLightOffset = 12.0f;

	/** Mark left on world surfaces where a shot lands, a deferred decal material */
	UPROPERTY(Config, EditAnywhere, Category="Impact")
	TSoftObjectPtr<UMaterialInterface> ImpactDecalMaterial;

	/** Width of the mark */
	UPROPERTY(Config, EditAnywhere, Category="Impact", meta = (ClampMin = 0.0, Units = "cm"))
	float ImpactDecalSize = 7.0f;

	/** Seconds the mark stays before it starts to fade */
	UPROPERTY(Config, EditAnywhere, Category="Impact", meta = (ClampMin = 0.0, Units = "s"))
	float ImpactDecalLifetime = 12.0f;

	/** Seconds the mark takes to fade */
	UPROPERTY(Config, EditAnywhere, Category="Impact", meta = (ClampMin = 0.0, Units = "s"))
	float ImpactDecalFadeSeconds = 2.0f;

	static const UShotFeedbackSettings* Get() { return GetDefault<UShotFeedbackSettings>(); }
};
