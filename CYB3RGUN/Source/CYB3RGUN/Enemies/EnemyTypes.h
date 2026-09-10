// CYB3RGUN THEGAME. Shared enemy types and native tags.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "NativeGameplayTags.h"
#include "EnemyTypes.generated.h"

class UStaticMesh;
class USoundBase;

/** Sent to the enemy state tree when the enemy dies */
CYB3RGUN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Enemy_Died);

/** Enemy families, used as the family tag on definitions */
CYB3RGUN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Enemy_Family_Undead);
CYB3RGUN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Enemy_Family_Machine);

/** Where damage came from. Every source flows through the same entry point on the enemy. */
UENUM(BlueprintType)
enum class EEnemyDamageSource : uint8
{
	Weapon,
	Fire,
	Explosion,
	Other
};

/** One placeholder shape of an enemy silhouette, relative to the enemy's feet */
USTRUCT(BlueprintType)
struct FEnemyShapePart
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UStaticMesh> Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector Location = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FRotator Rotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector Scale = FVector::OneVector;
};

/** Placeholder sound set for one enemy type */
USTRUCT(BlueprintType)
struct FEnemySoundSet
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<USoundBase> Spawn;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<USoundBase> Hurt;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<USoundBase> Attack;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<USoundBase> Death;

	/** Pitch multiplier applied to all four, so one shared placeholder sample can still differ per enemy */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = 0.1, ClampMax = 4.0))
	float Pitch = 1.0f;
};
