// CYB3RGUN THEGAME. AI controller that runs an enemy's state tree.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "CyberEnemyController.generated.h"

class UStateTreeAIComponent;

/**
 *  Possesses a CyberEnemy and runs the state tree named in its definition.
 *  Holds no behaviour of its own; everything authorable lives in the tree.
 */
UCLASS()
class CYB3RGUN_API ACyberEnemyController : public AAIController
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UStateTreeAIComponent* StateTreeAI;

public:

	ACyberEnemyController();

	/** Tells the state tree that the pawn died so it can move to its death state */
	void NotifyDied();

	UStateTreeAIComponent* GetStateTreeAI() const { return StateTreeAI; }

protected:

	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
};
