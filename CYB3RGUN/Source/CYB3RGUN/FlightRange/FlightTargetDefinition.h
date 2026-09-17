// CYB3RGUN THEGAME. Everything a flying target is, as data (D-079).

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "FlightTargetDefinition.generated.h"

class UMaterialInterface;
class USoundBase;
class UStaticMesh;
class UTexture2D;

/** How a target moves across the field of view (D-078) */
UENUM(BlueprintType)
enum class EFlightPathType : uint8
{
	/** Level flight across the field with a gentle bob */
	Straight,
	/** Launched upward, the climb slows under its own gravity on a ballistic arc */
	Rising,
	/** Enters high and drops, accelerating, then pulls out above the ground */
	Diving,
	/** Slow level flight with erratic sideways and vertical jinks */
	Flutter
};

/** What a hit does to a target */
UENUM(BlueprintType)
enum class EFlightHitBehaviour : uint8
{
	/** Stops flying and falls with a spin */
	Fall,
	/** Disappears at once, for clay and drones that burst */
	Vanish
};

/** One way a target can fly, with its values. A definition lists the paths it may take and how often. */
USTRUCT(BlueprintType)
struct FFlightPathSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Path")
	EFlightPathType Type = EFlightPathType::Straight;

	/** Relative chance this path is picked among the definition's paths */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Path", meta = (ClampMin = 0.0))
	float Weight = 1.0f;

	/** Multiplies the definition's flight speed on this path */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Path", meta = (ClampMin = 0.1))
	float SpeedScale = 1.0f;

	/** Start height above the player's ground for targets entering from the sides */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Path", meta = (Units = "cm"))
	float HeightMin = 400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Path", meta = (Units = "cm"))
	float HeightMax = 1200.0f;

	/** Rising: upward speed at launch. Diving: downward speed when it enters. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Path", meta = (ClampMin = 0.0, Units = "cm/s"))
	float VerticalSpeed = 0.0f;

	/** Rising: slows the climb. Diving: speeds up the drop. In cm per second squared. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Path", meta = (ClampMin = 0.0))
	float Gravity = 0.0f;

	/** Diving: the height the dive pulls out at */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Path", meta = (ClampMin = 0.0, Units = "cm"))
	float LevelOutHeight = 400.0f;

	/** Diving: seconds the pull out takes to bring the vertical speed to zero */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Path", meta = (ClampMin = 0.05, Units = "s"))
	float PullOutSeconds = 0.6f;

	/** Straight and Rising: height of the slow bob. Flutter: size of the jinks. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Path", meta = (ClampMin = 0.0, Units = "cm"))
	float WobbleAmplitude = 40.0f;

	/** Base frequency of the bob or the jinks */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Path", meta = (ClampMin = 0.0, Units = "Hz"))
	float WobbleFrequency = 0.6f;
};

/** One packed sheet: the grid of cells over the whole texture and how many of them hold a frame (D-093) */
USTRUCT(BlueprintType)
struct FFlightSpriteSheet
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sheet")
	TObjectPtr<UTexture2D> Texture;

	/** Cells across the texture, the whole grid, not only the used ones: the width of the sheet divided by the cell */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sheet", meta = (ClampMin = 1))
	int32 Columns = 4;

	/** Cells down the texture, the whole grid */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sheet", meta = (ClampMin = 1))
	int32 Rows = 4;

	/** Cells that hold a frame, counted from the top left along the rows */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sheet", meta = (ClampMin = 1))
	int32 Frames = 1;

	bool IsValid() const { return Texture != nullptr && Columns > 0 && Rows > 0 && Frames > 0; }
};

/** A run of frames of the flight sheet that loops on its own. The pipeline keeps the artist's order, the loops cut it up. */
USTRUCT(BlueprintType)
struct FFlightSpriteLoop
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Loop", meta = (ClampMin = 0))
	int32 FirstFrame = 0;

	/** Frames in the loop, zero runs to the end of the sheet */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Loop", meta = (ClampMin = 0))
	int32 FrameCount = 0;

	/** Relative chance this loop is picked for a target */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Loop", meta = (ClampMin = 0.0))
	float Weight = 1.0f;
};

/**
 *  A target drawn as a flat sprite that faces the camera, from two packed sheets: one loop of flight frames and one
 *  crash sequence (D-093). The art holds one profile view; a target flying the other way mirrors the same sheet, so a
 *  character needs no second set of frames. The sheets come out of the model repository's pipeline, which also writes
 *  the grid numbers below.
 */
USTRUCT(BlueprintType)
struct FFlightSpriteBody
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sprite")
	FFlightSpriteSheet Flight;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sprite")
	FFlightSpriteSheet Crash;

	/** A masked material with a Sheet texture parameter and the Columns, Rows, Frame and Mirror scalars */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sprite")
	TObjectPtr<UMaterialInterface> Material;

	/** The quad the sprite is drawn on: a flat unit plane. Empty takes the engine's plane. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sprite")
	TObjectPtr<UStaticMesh> Quad;

	/** Width of the sprite in the world at size 1. The cells are square, so this is its height as well. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sprite", meta = (ClampMin = 1.0, Units = "cm"))
	float Width = 100.0f;

	/** Moves the quad off the target's centre, in cell widths: right on the screen and up */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sprite")
	FVector2D Offset = FVector2D::ZeroVector;

	/** Frames per second of the flight loop, the same at every distance and size */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sprite", meta = (ClampMin = 0.1, Units = "Hz"))
	float FrameRate = 14.0f;

	/** The loops of the flight sheet, one picked per target. Empty runs the whole sheet. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sprite")
	TArray<FFlightSpriteLoop> Loops;

	/** Frames per second of the crash sheet while the target falls */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sprite", meta = (ClampMin = 0.1, Units = "Hz"))
	float CrashFrameRate = 7.0f;

	/** Crash frames played by the fall; the cells after them are the knockout pose a second hit switches to */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sprite", meta = (ClampMin = 1))
	int32 FallFrames = 2;

	bool IsValid() const { return Flight.IsValid() && Material != nullptr; }
};

/** One primitive piece of a target's body, placed around a pivot that flaps when the piece is a wing */
USTRUCT(BlueprintType)
struct FFlightTargetPart
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Part")
	TObjectPtr<UStaticMesh> Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Part")
	TObjectPtr<UMaterialInterface> Material;

	/** Where the pivot sits in the body, forward is +X and up is +Z */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Part", meta = (Units = "cm"))
	FVector Pivot = FVector::ZeroVector;

	/** The mesh relative to its pivot */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Part")
	FTransform Transform;

	/** 1 flaps the pivot as a right wing, -1 as a left wing, 0 keeps it still */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Part", meta = (ClampMin = -1.0, ClampMax = 1.0))
	float FlapSign = 0.0f;
};

/**
 *  A flying target as data: its body, size, speed, the paths it takes, its score, its sound and what a hit does to it.
 *  The flight range runs birds today; clay, drones or anything else fly the same module with another definition (D-079).
 */
UCLASS(BlueprintType)
class CYB3RGUN_API UFlightTargetDefinition : public UDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Target")
	FText DisplayName;

	/** A single mesh for the whole body, for a finished model. Empty uses the primitive parts below. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Body")
	TObjectPtr<UStaticMesh> Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Body")
	TObjectPtr<UMaterialInterface> MeshMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Body")
	FTransform MeshTransform;

	/** The body built from primitives, used while Mesh is empty */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Body")
	TArray<FFlightTargetPart> Parts;

	/** Sheets and material of a sprite body. A valid sprite replaces the mesh and the primitive parts (D-093). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Body")
	FFlightSpriteBody Sprite;

	/** Uniform scale of the body, picked per target between these */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Body", meta = (ClampMin = 0.1))
	float SizeMin = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Body", meta = (ClampMin = 0.1))
	float SizeMax = 1.2f;

	/** Radius of the body's hit sphere at size 1 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hit", meta = (ClampMin = 1.0, Units = "cm"))
	float BodyHitRadius = 22.0f;

	/** Radius of the head's hit sphere at size 1, zero for a target without a head zone */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hit", meta = (ClampMin = 0.0, Units = "cm"))
	float HeadHitRadius = 8.0f;

	/** Head sphere centre in the body at size 1 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hit", meta = (Units = "cm"))
	FVector HeadOffset = FVector(26.0f, 0.0f, 6.0f);

	/** Flight speed, picked per target between these and scaled by the path */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Flight", meta = (ClampMin = 10.0, Units = "cm/s"))
	float SpeedMin = 700.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Flight", meta = (ClampMin = 10.0, Units = "cm/s"))
	float SpeedMax = 1100.0f;

	/** The paths this target flies */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Flight")
	TArray<FFlightPathSettings> Paths;

	/** Wing beat amplitude either side of level */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Flight", meta = (ClampMin = 0.0, ClampMax = 89.0, Units = "Degrees"))
	float FlapDegrees = 35.0f;

	/** Wing beats per second at size 1; smaller bodies beat faster */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Flight", meta = (ClampMin = 0.0, Units = "Hz"))
	float FlapFrequency = 4.0f;

	/** Points before the difficulty factor of the flight range settings */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Score", meta = (ClampMin = 0))
	int32 BaseScore = 10;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Behaviour")
	EFlightHitBehaviour HitBehaviour = EFlightHitBehaviour::Fall;

	/** Falling: gravity on the body after a hit, in cm per second squared */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Behaviour", meta = (ClampMin = 0.0))
	float FallGravity = 1500.0f;

	/** Falling: tumble rate */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Behaviour", meta = (ClampMin = 0.0, Units = "DegreesPerSecond"))
	float FallSpinDegreesPerSecond = 540.0f;

	/** Falling: share of the flight speed the body keeps as it drops */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Behaviour", meta = (ClampMin = 0.0, ClampMax = 1.0))
	float FallCarry = 0.35f;

	/** Played where the target is hit */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sound")
	TObjectPtr<USoundBase> HitSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sound", meta = (ClampMin = 0.1, ClampMax = 4.0))
	float HitPitch = 1.0f;

	/** Played where the target enters the field */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sound")
	TObjectPtr<USoundBase> LaunchSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sound", meta = (ClampMin = 0.1, ClampMax = 4.0))
	float LaunchPitch = 1.0f;

	/** A path by weight, or null when the definition lists none */
	const FFlightPathSettings* PickPath(const TArray<EFlightPathType>& Allowed) const;

	/** A flight loop by weight, or the whole sheet when the sprite lists none */
	FFlightSpriteLoop PickSpriteLoop() const;
};
