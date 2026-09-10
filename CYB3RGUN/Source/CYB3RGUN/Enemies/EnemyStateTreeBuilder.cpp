// CYB3RGUN THEGAME. Editor console command that bootstraps the zombie state tree asset.
// The tree stays a normal asset afterwards and is authored in the StateTree editor like any other.

#if WITH_EDITOR

#include "CyberEnemy.h"
#include "CyberEnemyController.h"
#include "EnemyStateTreeTasks.h"
#include "EnemyTypes.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/StateTreeAIComponentSchema.h"
#include "HAL/IConsoleManager.h"
#include "Misc/PackageName.h"
#include "StateTree.h"
#include "StateTreeCompilerLog.h"
#include "StateTreeEditingSubsystem.h"
#include "StateTreeEditorData.h"
#include "StateTreeState.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UObject/UnrealType.h"

DEFINE_LOG_CATEGORY_STATIC(LogEnemyStateTreeBuilder, Log, All);

namespace
{
	void SetClassProperty(UObject* Object, const TCHAR* PropertyName, UClass* Value)
	{
		if (FClassProperty* Property = FindFProperty<FClassProperty>(Object->GetClass(), PropertyName))
		{
			Property->SetObjectPropertyValue_InContainer(Object, Value);
		}
		else
		{
			UE_LOG(LogEnemyStateTreeBuilder, Warning, TEXT("Property %s not found on %s"), PropertyName, *Object->GetClass()->GetName());
		}
	}

	void BuildZombieStateTree(const TArray<FString>& Args)
	{
		const FString PackageName = Args.Num() > 0 ? Args[0] : TEXT("/Game/CYB3RGUN/Enemies/StateTrees/ST_Zombie");
		const FString AssetName = FPackageName::GetShortName(PackageName);

		UPackage* Package = CreatePackage(*PackageName);
		Package->FullyLoad();

		UStateTree* Tree = FindObject<UStateTree>(Package, *AssetName);
		const bool bExisted = Tree != nullptr;
		if (!Tree)
		{
			Tree = NewObject<UStateTree>(Package, *AssetName, RF_Public | RF_Standalone | RF_Transactional);
		}

		// fresh editor data every time, the command is the single source for the bootstrap layout
		UStateTreeEditorData* Data = NewObject<UStateTreeEditorData>(Tree, TEXT("EditorData"), RF_Transactional);
		UStateTreeAIComponentSchema* Schema = NewObject<UStateTreeAIComponentSchema>(Data, NAME_None, RF_Transactional);
		SetClassProperty(Schema, TEXT("ContextActorClass"), ACyberEnemy::StaticClass());
		SetClassProperty(Schema, TEXT("AIControllerClass"), ACyberEnemyController::StaticClass());
		// the schema derives its context data descriptors from those classes in PostLoad and in the editor change chain,
		// neither of which runs for a freshly constructed object, so refresh them the way a loaded asset would
		Schema->PostLoad();
		Data->Schema = Schema;
		Tree->EditorData = Data;

		UStateTreeState& Root = Data->AddRootState();
		UStateTreeState& Idle = Root.AddChildState(TEXT("Idle"));
		UStateTreeState& Acquire = Root.AddChildState(TEXT("Acquire"));
		UStateTreeState& Approach = Root.AddChildState(TEXT("Approach"));
		UStateTreeState& Attack = Root.AddChildState(TEXT("Attack"));
		UStateTreeState& Die = Root.AddChildState(TEXT("Die"));

		Idle.AddTask<FEnemyIdleTask>();
		Acquire.AddTask<FEnemyAcquireTargetTask>();
		Approach.AddTask<FEnemyApproachTask>();
		Attack.AddTask<FEnemyAttackTask>();
		Die.AddTask<FEnemyDieTask>();

		Idle.AddTransition(EStateTreeTransitionTrigger::OnStateCompleted, EStateTreeTransitionType::GotoState, &Acquire);
		Acquire.AddTransition(EStateTreeTransitionTrigger::OnStateSucceeded, EStateTreeTransitionType::GotoState, &Approach);
		Acquire.AddTransition(EStateTreeTransitionTrigger::OnStateFailed, EStateTreeTransitionType::GotoState, &Idle);
		Approach.AddTransition(EStateTreeTransitionTrigger::OnStateSucceeded, EStateTreeTransitionType::GotoState, &Attack);
		Approach.AddTransition(EStateTreeTransitionTrigger::OnStateFailed, EStateTreeTransitionType::GotoState, &Acquire);
		Attack.AddTransition(EStateTreeTransitionTrigger::OnStateSucceeded, EStateTreeTransitionType::GotoState, &Approach);
		Attack.AddTransition(EStateTreeTransitionTrigger::OnStateFailed, EStateTreeTransitionType::GotoState, &Acquire);

		// death interrupts any state
		Root.AddTransition(EStateTreeTransitionTrigger::OnEvent, TAG_Enemy_Died, EStateTreeTransitionType::GotoState, &Die);

		FStateTreeCompilerLog Log;
		const bool bCompiled = UStateTreeEditingSubsystem::CompileStateTree(Tree, Log);
		Log.DumpToLog(LogEnemyStateTreeBuilder);

		if (!bExisted)
		{
			FAssetRegistryModule::AssetCreated(Tree);
		}
		Package->MarkPackageDirty();

		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		const FString FileName = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
		const bool bSaved = UPackage::SavePackage(Package, Tree, *FileName, SaveArgs);

		UE_LOG(LogEnemyStateTreeBuilder, Log, TEXT("Zombie state tree %s: compiled %d, saved %d, file %s"), *PackageName, bCompiled ? 1 : 0, bSaved ? 1 : 0, *FileName);
	}
}

static FAutoConsoleCommand GBuildZombieStateTreeCommand(
	TEXT("Zombie.BuildStateTree"),
	TEXT("Creates or rebuilds the bootstrap zombie state tree asset: Idle, Acquire, Approach, Attack, Die. Optional argument: package path."),
	FConsoleCommandWithArgsDelegate::CreateStatic(&BuildZombieStateTree));

#endif
