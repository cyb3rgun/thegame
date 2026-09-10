// CYB3RGUN THEGAME. AI controller that runs an enemy's state tree.

#include "CyberEnemyController.h"
#include "CyberEnemy.h"
#include "EnemyDefinition.h"
#include "EnemyTypes.h"
#include "Components/StateTreeAIComponent.h"
#include "StateTree.h"

DEFINE_LOG_CATEGORY_STATIC(LogCyberEnemyAI, Log, All);

ACyberEnemyController::ACyberEnemyController()
{
	StateTreeAI = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTreeAI"));
	StateTreeAI->SetStartLogicAutomatically(false);

	bAttachToPawn = true;
}

void ACyberEnemyController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	const ACyberEnemy* Enemy = Cast<ACyberEnemy>(InPawn);
	const UEnemyDefinition* Definition = Enemy ? Enemy->GetDefinition() : nullptr;

	if (!Definition || !Definition->Behavior)
	{
		UE_LOG(LogCyberEnemyAI, Warning, TEXT("%s possessed %s without a behaviour state tree"), *GetName(), *GetNameSafe(InPawn));
		return;
	}

	StateTreeAI->SetStateTree(Definition->Behavior);
	StateTreeAI->StartLogic();
}

void ACyberEnemyController::OnUnPossess()
{
	StateTreeAI->StopLogic(TEXT("Unpossessed"));
	Super::OnUnPossess();
}

void ACyberEnemyController::NotifyDied()
{
	StateTreeAI->SendStateTreeEvent(TAG_Enemy_Died);
}
