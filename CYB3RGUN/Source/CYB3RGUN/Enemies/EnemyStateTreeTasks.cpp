// CYB3RGUN THEGAME. State tree tasks for enemy behaviour: idle, acquire, approach, attack, die.

#include "EnemyStateTreeTasks.h"
#include "CyberEnemy.h"
#include "EnemyDefinition.h"
#include "AIController.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"
#include "StateTreeExecutionContext.h"

// Idle --------------------------------------------------------------------

EStateTreeRunStatus FEnemyIdleTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.Elapsed = 0.0f;
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FEnemyIdleTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	if (!Data.Enemy)
	{
		return EStateTreeRunStatus::Failed;
	}

	const float IdleTime = Data.Enemy->GetDefinition() ? Data.Enemy->GetDefinition()->IdleTime : 0.5f;
	Data.Elapsed += DeltaTime;
	return Data.Elapsed >= IdleTime ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Running;
}

// Acquire -----------------------------------------------------------------

EStateTreeRunStatus FEnemyAcquireTargetTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	return TryAcquire(Context.GetInstanceData(*this));
}

EStateTreeRunStatus FEnemyAcquireTargetTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	return TryAcquire(Context.GetInstanceData(*this));
}

EStateTreeRunStatus FEnemyAcquireTargetTask::TryAcquire(FInstanceDataType& Data)
{
	if (!Data.Enemy)
	{
		return EStateTreeRunStatus::Failed;
	}

	// the first player is the only target for now, a perception based pick can replace this later
	APawn* Player = UGameplayStatics::GetPlayerPawn(Data.Enemy, 0);
	if (Player && !Player->IsPendingKillPending())
	{
		Data.Enemy->SetTarget(Player);
		return EStateTreeRunStatus::Succeeded;
	}

	return EStateTreeRunStatus::Running;
}

// Approach ----------------------------------------------------------------

void FEnemyApproachTask::RequestMove(FInstanceDataType& Data)
{
	AActor* Target = Data.Enemy->GetTarget();
	const UEnemyDefinition* Definition = Data.Enemy->GetDefinition();
	if (!Target || !Definition || !Data.AIController)
	{
		return;
	}

	// stop a little short of the attack range so the attack state starts with the target in reach
	const float AcceptanceRadius = Definition->AttackRange * 0.6f;
	Data.AIController->MoveToActor(Target, AcceptanceRadius, true, true, false, nullptr, true);
	Data.RepathTimer = 0.0f;
}

EStateTreeRunStatus FEnemyApproachTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	if (!Data.Enemy || !Data.Enemy->GetTarget() || !Data.AIController)
	{
		return EStateTreeRunStatus::Failed;
	}

	Data.BurstTimer = 0.0f;
	Data.bBursting = false;
	if (const UEnemyDefinition* Definition = Data.Enemy->GetDefinition())
	{
		Data.Enemy->SetMoveSpeed(Definition->MoveSpeed);
	}

	RequestMove(Data);
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FEnemyApproachTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	if (!Data.Enemy || Data.Enemy->IsDead() || !Data.AIController)
	{
		return EStateTreeRunStatus::Failed;
	}

	AActor* Target = Data.Enemy->GetTarget();
	if (!Target)
	{
		return EStateTreeRunStatus::Failed;
	}

	if (Data.Enemy->IsTargetInAttackRange())
	{
		Data.AIController->StopMovement();
		return EStateTreeRunStatus::Succeeded;
	}

	// re-issue the move when path following went idle, which happens before the nav mesh around the player exists
	Data.RepathTimer += DeltaTime;
	const UPathFollowingComponent* PathFollowing = Data.AIController->GetPathFollowingComponent();
	const bool bMoving = PathFollowing && PathFollowing->GetStatus() == EPathFollowingStatus::Moving;
	if (!bMoving && Data.RepathTimer >= Data.RepathInterval)
	{
		RequestMove(Data);
	}

	// sprinters close distance in bursts, everyone else walks
	const UEnemyDefinition* Definition = Data.Enemy->GetDefinition();
	if (Definition && Definition->BurstSpeed > 0.0f)
	{
		Data.BurstTimer += DeltaTime;
		if (!Data.bBursting && Data.BurstTimer >= Definition->BurstCooldown)
		{
			Data.bBursting = true;
			Data.BurstTimer = 0.0f;
			Data.Enemy->SetMoveSpeed(Definition->BurstSpeed);
		}
		else if (Data.bBursting && Data.BurstTimer >= Definition->BurstDuration)
		{
			Data.bBursting = false;
			Data.BurstTimer = 0.0f;
			Data.Enemy->SetMoveSpeed(Definition->MoveSpeed);
		}
	}

	return EStateTreeRunStatus::Running;
}

void FEnemyApproachTask::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	if (Data.AIController)
	{
		Data.AIController->StopMovement();
	}
	if (Data.Enemy && Data.Enemy->GetDefinition())
	{
		Data.Enemy->SetMoveSpeed(Data.Enemy->GetDefinition()->MoveSpeed);
	}
}

// Attack ------------------------------------------------------------------

EStateTreeRunStatus FEnemyAttackTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	if (!Data.Enemy || !Data.Enemy->GetTarget() || !Data.Enemy->GetDefinition())
	{
		return EStateTreeRunStatus::Failed;
	}

	Data.Countdown = Data.Enemy->GetDefinition()->AttackWindup;
	if (Data.AIController)
	{
		Data.AIController->SetFocus(Data.Enemy->GetTarget());
	}
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FEnemyAttackTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	if (!Data.Enemy || Data.Enemy->IsDead())
	{
		return EStateTreeRunStatus::Failed;
	}

	const UEnemyDefinition* Definition = Data.Enemy->GetDefinition();
	if (!Definition || !Data.Enemy->GetTarget())
	{
		return EStateTreeRunStatus::Failed;
	}

	// the target walked away, go back to approaching
	if (!Data.Enemy->IsTargetInAttackRange(Data.LeaveRangeScale))
	{
		return EStateTreeRunStatus::Succeeded;
	}

	Data.Countdown -= DeltaTime;
	if (Data.Countdown <= 0.0f)
	{
		Data.Enemy->PerformAttack();
		Data.Countdown = Definition->AttackCooldown + Definition->AttackWindup;
	}

	return EStateTreeRunStatus::Running;
}

void FEnemyAttackTask::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	if (Data.AIController)
	{
		Data.AIController->ClearFocus(EAIFocusPriority::Gameplay);
	}
}

// Die ---------------------------------------------------------------------

EStateTreeRunStatus FEnemyDieTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	if (Data.AIController)
	{
		Data.AIController->StopMovement();
		Data.AIController->ClearFocus(EAIFocusPriority::Gameplay);
	}
	// the enemy actor removes itself after its linger time, this state just holds
	return EStateTreeRunStatus::Running;
}
