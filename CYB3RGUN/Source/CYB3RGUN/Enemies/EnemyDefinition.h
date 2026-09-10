// CYB3RGUN THEGAME. Data asset describing one enemy type.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "EnemyTypes.h"
#include "EnemyDefinition.generated.h"

class ACyberEnemy;
class UMaterialInterface;
class UStateTree;

/**
 *  Everything that makes one enemy type: stats, silhouette, sounds, family and behaviour.
 *  Enemies read this at spawn; nothing about a type is hard coded in the actor.
 */
UCLASS(BlueprintType)
class CYB3RGUN_API UEnemyDefinition : public UDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Identity")
	FText DisplayName;

	/** Enemy.Family.Undead or Enemy.Family.Machine */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Identity", meta = (Categories = "Enemy.Family"))
	FGameplayTag FamilyTag;

	/** Actor class spawned for this type, ACyberEnemy unless a subclass is needed */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Identity")
	TSubclassOf<ACyberEnemy> EnemyClass;

	/** Points awarded to the killer */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Identity", meta = (ClampMin = 0))
	int32 ScoreValue = 100;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats", meta = (ClampMin = 1.0))
	float MaxHealth = 100.0f;

	/** Walking speed in cm per second */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats", meta = (ClampMin = 0.0, Units = "cm/s"))
	float MoveSpeed = 150.0f;

	/** Speed during a burst, 0 disables bursts */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats", meta = (ClampMin = 0.0, Units = "cm/s"))
	float BurstSpeed = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats", meta = (ClampMin = 0.0, Units = "s"))
	float BurstDuration = 0.6f;

	/** Pause between bursts */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats", meta = (ClampMin = 0.0, Units = "s"))
	float BurstCooldown = 2.0f;

	/** Damage dealt to the target per attack */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats", meta = (ClampMin = 0.0))
	float Damage = 10.0f;

	/** Distance from the enemy to the target at which it attacks */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats", meta = (ClampMin = 10.0, Units = "cm"))
	float AttackRange = 120.0f;

	/** Seconds from starting an attack to the damage landing */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats", meta = (ClampMin = 0.0, Units = "s"))
	float AttackWindup = 0.4f;

	/** Seconds between attacks */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats", meta = (ClampMin = 0.1, Units = "s"))
	float AttackCooldown = 1.5f;

	/** Seconds the enemy stands after spawning before it acquires a target */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats", meta = (ClampMin = 0.0, Units = "s"))
	float IdleTime = 0.5f;

	/** Seconds the body stays before it is removed */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats", meta = (ClampMin = 0.0, Units = "s"))
	float DeathLinger = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Collision", meta = (ClampMin = 10.0, Units = "cm"))
	float CapsuleRadius = 34.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Collision", meta = (ClampMin = 10.0, Units = "cm"))
	float CapsuleHalfHeight = 88.0f;

	/** Placeholder silhouette built from engine shapes, positions relative to the feet */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visuals")
	TArray<FEnemyShapePart> Parts;

	/** Material applied to every part */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visuals")
	TObjectPtr<UMaterialInterface> Material;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Audio")
	FEnemySoundSet Sounds;

	/** State tree that drives this enemy. Authored in the editor, started by the enemy's AI controller. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Behaviour")
	TObjectPtr<UStateTree> Behavior;
};
