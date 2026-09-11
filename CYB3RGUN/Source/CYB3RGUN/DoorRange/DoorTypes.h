// CYB3RGUN THEGAME. Door module shared types.

#pragma once

#include "CoreMinimal.h"
#include "DoorTypes.generated.h"

class UMaterialInterface;
class USoundBase;

/** What stands behind a door when it opens. */
UENUM(BlueprintType)
enum class EDoorOccupant : uint8
{
	Empty,
	Hostile,
	Friendly,
	/** A hostile machine behind a hostage, only a strip of it shows */
	HostageTaker
};

/** Lifecycle of a single door slot. */
UENUM(BlueprintType)
enum class EDoorState : uint8
{
	Closed,
	Opening,
	Showing,
	Closing
};

/** Events the range reports to feedback and HUD listeners. */
UENUM(BlueprintType)
enum class EDoorRangeEvent : uint8
{
	HostileHit,
	FriendlyHit,
	HostileEscaped,
	HostileDrawn,
	WaveStarted,
	WaveEnded,
	RangeFinished,
	HostageRescued,
	HostageHit
};

/** Pacing and mix for one wave. Later waves usually get shorter windows and more hostiles. */
USTRUCT(BlueprintType)
struct FDoorWaveSettings
{
	GENERATED_BODY()

	/** Seconds a drawn hostile or a friendly stays exposed with the door fully open */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = 0.0, Units = "s"))
	float ExposureWindow = 2.0f;

	/** Seconds a hostile takes to rise and draw after the door is open. Shots before the draw do not count. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = 0.0, Units = "s"))
	float TelegraphDuration = 0.5f;

	/** Seconds between two door openings */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = 0.05, Units = "s"))
	float TimeBetweenOpenings = 1.0f;

	/** Share of openings that show a hostile */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = 0.0, ClampMax = 1.0))
	float HostileShare = 0.6f;

	/** Share of openings that show nothing. The rest are friendlies. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = 0.0, ClampMax = 1.0))
	float EmptyShare = 0.1f;

	/** Door openings in this wave */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = 1))
	int32 Openings = 12;

	/** Maximum doors open at the same time */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = 1, ClampMax = 12))
	int32 VisibleDoors = 3;

	/** Chance that this wave holds one hostage taker. It takes the place of a friendly, or of an empty door when there is none. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = 0.0, ClampMax = 1.0))
	float HostageTakerChance = 0.35f;
};

/** Everything a door slot needs to know for one opening. */
USTRUCT(BlueprintType)
struct FDoorOpenParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EDoorOccupant Occupant = EDoorOccupant::Empty;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UMaterialInterface> OccupantMaterial = nullptr;

	/** Material on the hostage when the occupant is a hostage taker */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UMaterialInterface> HostageMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float OpenDuration = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ExposureWindow = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CloseDuration = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float TelegraphDuration = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<USoundBase> DoorSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DoorPitch = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<USoundBase> TelegraphSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float TelegraphPitch = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<USoundBase> DrawSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DrawPitch = 1.0f;
};

/** Results of one range session. */
USTRUCT(BlueprintType)
struct FDoorRangeStats
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	int32 FinalScore = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 HostilesTotal = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 HostilesHit = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 HostilesEscaped = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 FriendliesHit = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 HostagesRescued = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 HostagesHit = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 WavesPlayed = 0;
};
