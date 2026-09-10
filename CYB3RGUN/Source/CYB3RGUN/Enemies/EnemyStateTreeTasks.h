// CYB3RGUN THEGAME. State tree tasks for enemy behaviour: idle, acquire, approach, attack, die.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "EnemyStateTreeTasks.generated.h"

class ACyberEnemy;
class AAIController;

/** Instance data shared by the simple enemy tasks: the enemy is bound from the tree context */
USTRUCT()
struct FEnemyTaskInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = Context)
	TObjectPtr<ACyberEnemy> Enemy;

	UPROPERTY(Transient)
	float Elapsed = 0.0f;
};

/** Stands still for the definition's idle time, then succeeds */
USTRUCT(meta = (DisplayName = "Enemy Idle", Category = "CYB3RGUN|Enemy"))
struct FEnemyIdleTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FEnemyTaskInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
};

/** Picks the local player pawn as target, succeeds once it has one */
USTRUCT(meta = (DisplayName = "Enemy Acquire Target", Category = "CYB3RGUN|Enemy"))
struct FEnemyAcquireTargetTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FEnemyTaskInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;

private:
	static EStateTreeRunStatus TryAcquire(FInstanceDataType& Data);
};

USTRUCT()
struct FEnemyApproachTaskInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = Context)
	TObjectPtr<ACyberEnemy> Enemy;

	UPROPERTY(EditAnywhere, Category = Context)
	TObjectPtr<AAIController> AIController;

	/** Seconds between move requests when the last one did not stick, for example before the nav mesh exists */
	UPROPERTY(EditAnywhere, Category = Parameter, meta = (ClampMin = 0.1))
	float RepathInterval = 0.5f;

	UPROPERTY(Transient)
	float RepathTimer = 0.0f;

	UPROPERTY(Transient)
	float BurstTimer = 0.0f;

	UPROPERTY(Transient)
	bool bBursting = false;
};

/** Walks toward the target on the nav mesh, with speed bursts when the definition has them. Succeeds in attack range. */
USTRUCT(meta = (DisplayName = "Enemy Approach Target", Category = "CYB3RGUN|Enemy"))
struct FEnemyApproachTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FEnemyApproachTaskInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

private:
	static void RequestMove(FInstanceDataType& Data);
};

USTRUCT()
struct FEnemyAttackTaskInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = Context)
	TObjectPtr<ACyberEnemy> Enemy;

	UPROPERTY(EditAnywhere, Category = Context)
	TObjectPtr<AAIController> AIController;

	/** Attack range multiplier before the enemy gives up and approaches again */
	UPROPERTY(EditAnywhere, Category = Parameter, meta = (ClampMin = 1.0))
	float LeaveRangeScale = 1.4f;

	/** Seconds until the next attack lands, negative while cooling down */
	UPROPERTY(Transient)
	float Countdown = 0.0f;
};

/** Attacks the target with the definition's windup and cooldown. Succeeds when the target moves out of range. */
USTRUCT(meta = (DisplayName = "Enemy Attack Target", Category = "CYB3RGUN|Enemy"))
struct FEnemyAttackTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FEnemyAttackTaskInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

/** Terminal state after death: stops movement and stays until the body is removed */
USTRUCT(meta = (DisplayName = "Enemy Die", Category = "CYB3RGUN|Enemy"))
struct FEnemyDieTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	FEnemyDieTask()
	{
		bShouldCallTick = false;
	}

	using FInstanceDataType = FEnemyApproachTaskInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};
