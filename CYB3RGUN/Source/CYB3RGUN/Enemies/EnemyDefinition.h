// CYB3RGUN THEGAME. Data asset describing one enemy type.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "EnemyTypes.h"
#include "EnemyDefinition.generated.h"

class ACyberEnemy;
class UAnimSequenceBase;
class UMaterialInterface;
class USkeletalMesh;
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

	/** Skeletal body on the standard mannequin skeleton. When set it replaces the placeholder parts. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visuals|Body")
	TObjectPtr<USkeletalMesh> BodyMesh;

	/** Material put on every slot of the body */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visuals|Body")
	TObjectPtr<UMaterialInterface> BodyMaterial;

	/** Size against the mannequin. Keep the body inside the capsule, shots hit the capsule. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visuals|Body", meta = (ClampMin = 0.5, ClampMax = 2.0))
	float BodyScale = 1.0f;

	/** Looped while standing */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visuals|Body")
	TObjectPtr<UAnimSequenceBase> IdleAnimation;

	/** Looped while moving, played faster or slower with the ground speed */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visuals|Body")
	TObjectPtr<UAnimSequenceBase> MoveAnimation;

	/** Ground speed at which the move animation plays at its authored rate */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visuals|Body", meta = (ClampMin = 10.0, Units = "cm/s"))
	float MoveAnimationSpeed = 150.0f;

	/** Played once per attack */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visuals|Body")
	TObjectPtr<UAnimSequenceBase> AttackAnimation;

	/** One is picked at random on death, the body holds its last frame until it is removed */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visuals|Body")
	TArray<TObjectPtr<UAnimSequenceBase>> DeathAnimations;

	/** Placeholder silhouette built from engine shapes, used without a body mesh, positions relative to the feet */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visuals")
	TArray<FEnemyShapePart> Parts;

	/** Material applied to every placeholder part */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visuals")
	TObjectPtr<UMaterialInterface> Material;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Audio")
	FEnemySoundSet Sounds;

	/** State tree that drives this enemy. Authored in the editor, started by the enemy's AI controller. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Behaviour")
	TObjectPtr<UStateTree> Behavior;
};
