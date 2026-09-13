// CYB3RGUN THEGAME. Native anim instance of target bodies: a base sequence with a cross fade, reactions blended over it.

#include "TargetAnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimNodeBase.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/AnimSyncScope.h"
#include "Animation/AnimationPoseData.h"
#include "Animation/Skeleton.h"
#include "AnimationRuntime.h"
#include "Components/SkeletalMeshComponent.h"

UTargetAnimInstance::UTargetAnimInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// the base keeps its root locked like the single node mode did, montage root motion is thrown away, movement keeps the capsule
	RootMotionMode = ERootMotionMode::IgnoreRootMotion;
}

UTargetAnimInstance* UTargetAnimInstance::FromMesh(const USkeletalMeshComponent* Mesh)
{
	return Mesh ? Cast<UTargetAnimInstance>(Mesh->GetAnimInstance()) : nullptr;
}

UTargetAnimInstance* UTargetAnimInstance::Ensure(USkeletalMeshComponent* Mesh)
{
	if (!Mesh)
	{
		return nullptr;
	}
	if (UTargetAnimInstance* Existing = FromMesh(Mesh))
	{
		return Existing;
	}

	// a body saved with an older setup, or put back into single node mode by a PlayAnimation call
	Mesh->SetAnimInstanceClass(UTargetAnimInstance::StaticClass());
	return FromMesh(Mesh);
}

FAnimInstanceProxy* UTargetAnimInstance::CreateAnimInstanceProxy()
{
	return new FTargetAnimInstanceProxy(this);
}

void UTargetAnimInstance::DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy)
{
	delete static_cast<FTargetAnimInstanceProxy*>(InProxy);
}

void UTargetAnimInstance::PlayBase(UAnimSequenceBase* Sequence, bool bLooping, float PlayRate, float StartTime, float BlendTime)
{
	if (!Sequence || Sequence->IsValidAdditive())
	{
		return;
	}

	if (BlendTime > 0.0f && BaseSequence && BaseSequence.Get() != Sequence)
	{
		OutgoingSequence = BaseSequence;
	}
	BaseSequence = Sequence;
	bRequestedLooping = bLooping;
	RequestedPlayRate = PlayRate;
	RequestedTime = StartTime;
	RequestedBlendTime = BlendTime;
	bBaseRequestPending = true;
	bRateRequestPending = false;
	bTimeRequestPending = false;
}

void UTargetAnimInstance::SetBasePlayRate(float PlayRate)
{
	RequestedPlayRate = PlayRate;
	bRateRequestPending = true;
}

void UTargetAnimInstance::SetBaseTime(float Time)
{
	RequestedTime = Time;
	bTimeRequestPending = true;
}

UAnimMontage* UTargetAnimInstance::PlayReaction(UAnimSequenceBase* Reaction, float BlendIn, float BlendOut, float PlayRate)
{
	if (!Reaction)
	{
		return nullptr;
	}
	return PlaySlotAnimationAsDynamicMontage(Reaction, FAnimSlotGroup::DefaultSlotName, BlendIn, BlendOut, PlayRate, 1, -1.0f, 0.0f);
}

void FTargetAnimInstanceProxy::Initialize(UAnimInstance* InAnimInstance)
{
	// the base call clears every registered slot, the slot is registered again after it
	FAnimInstanceProxy::Initialize(InAnimInstance);
	RegisterSlotNodeWithAnimInstance(FAnimSlotGroup::DefaultSlotName);
	SlotWeights.Reset();
	Current = FTargetBaseLayer();
	Previous = FTargetBaseLayer();
	BlendDuration = 0.0f;
	BlendElapsed = 0.0f;
	CurrentLayerWeight = 1.0f;

	// a reinitialised instance, after a mesh change for example, starts the last base asked for again
	UTargetAnimInstance* Instance = CastChecked<UTargetAnimInstance>(InAnimInstance);
	if (Instance->BaseSequence)
	{
		Instance->bBaseRequestPending = true;
		Instance->RequestedBlendTime = 0.0f;
	}
}

void FTargetAnimInstanceProxy::PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds)
{
	FAnimInstanceProxy::PreUpdate(InAnimInstance, DeltaSeconds);

	UTargetAnimInstance* Instance = CastChecked<UTargetAnimInstance>(InAnimInstance);
	if (Instance->bBaseRequestPending)
	{
		UAnimSequenceBase* NewSequence = Instance->BaseSequence.Get();
		const bool bFade = Instance->RequestedBlendTime > 0.0f && Current.Sequence && Current.Sequence != NewSequence;
		Previous = bFade ? Current : FTargetBaseLayer();
		BlendDuration = bFade ? Instance->RequestedBlendTime : 0.0f;
		BlendElapsed = 0.0f;
		CurrentLayerWeight = bFade ? 0.0f : 1.0f;

		Current = FTargetBaseLayer();
		Current.Sequence = NewSequence;
		Current.bLooping = Instance->bRequestedLooping;
		Current.PlayRate = Instance->RequestedPlayRate;
		Current.Time = Instance->RequestedTime;
		Instance->bBaseRequestPending = false;
	}
	if (Instance->bRateRequestPending)
	{
		Current.PlayRate = Instance->RequestedPlayRate;
		Instance->bRateRequestPending = false;
	}
	if (Instance->bTimeRequestPending)
	{
		Current.Time = Instance->RequestedTime;
		Current.MarkerTickRecord.Reset();
		Current.DeltaTimeRecord = FDeltaTimeRecord();
		Instance->bTimeRequestPending = false;
	}
}

void FTargetAnimInstanceProxy::UpdateAnimationNode(const FAnimationUpdateContext& InContext)
{
	// without a root node nothing else counts the updates, and the mesh would keep forcing extra ones
	UpdateCounter.Increment();

	// the slot weights, as FAnimNode_Slot works them out in its update
	const FName Slot = FAnimSlotGroup::DefaultSlotName;
	GetSlotWeight(Slot, SlotWeights.SlotNodeWeight, SlotWeights.SourceWeight, SlotWeights.TotalNodeWeight);
	UpdateSlotNodeWeight(Slot, SlotWeights.SlotNodeWeight, InContext.GetFinalBlendWeight());

	if (Previous.Sequence && BlendDuration > 0.0f)
	{
		BlendElapsed += InContext.GetDeltaTime();
		CurrentLayerWeight = FMath::Clamp(BlendElapsed / BlendDuration, 0.0f, 1.0f);
		if (CurrentLayerWeight >= 1.0f)
		{
			Previous = FTargetBaseLayer();
		}
	}
	else
	{
		CurrentLayerWeight = 1.0f;
	}

	// the base keeps running under a full weight reaction, its weight only decides which notifies fire
	const float SourceWeight = SlotWeights.SourceWeight;
	if (Current.Sequence)
	{
		QueueLayerTick(InContext, Current, SourceWeight * CurrentLayerWeight);
	}
	if (Previous.Sequence)
	{
		QueueLayerTick(InContext, Previous, SourceWeight * (1.0f - CurrentLayerWeight));
	}
}

void FTargetAnimInstanceProxy::QueueLayerTick(const FAnimationUpdateContext& InContext, FTargetBaseLayer& Layer, float Weight)
{
	// the same tick record the single node mode queues; the sync scope moves Layer.Time on after this update
	FAnimTickRecord TickRecord(Layer.Sequence, Layer.bLooping, Layer.PlayRate, false,
		FMath::Max(FAnimWeight::GetSmallestRelevantWeight(), Weight), Layer.Time, Layer.MarkerTickRecord);
	TickRecord.DeltaTimeRecord = &Layer.DeltaTimeRecord;
	InContext.GetMessageChecked<UE::Anim::FAnimSyncGroupScope>().AddTickRecord(TickRecord);
}

bool FTargetAnimInstanceProxy::Evaluate(FPoseContext& Output)
{
	// the slot over the base, as FAnimNode_Slot evaluates it
	if (SlotWeights.SlotNodeWeight <= ZERO_ANIMWEIGHT_THRESH)
	{
		EvaluateBase(Output);
		return true;
	}

	FPoseContext SourceContext(Output);
	if (SlotWeights.SourceWeight > ZERO_ANIMWEIGHT_THRESH)
	{
		EvaluateBase(SourceContext);
	}
	const FAnimationPoseData SourcePoseData(SourceContext);
	FAnimationPoseData OutputPoseData(Output);
	SlotEvaluatePose(FAnimSlotGroup::DefaultSlotName, SourcePoseData, SlotWeights.SourceWeight, OutputPoseData,
		SlotWeights.SlotNodeWeight, SlotWeights.TotalNodeWeight);

	// true skips the node graph, there is none
	return true;
}

void FTargetAnimInstanceProxy::EvaluateBase(FPoseContext& Output) const
{
	if (!Current.Sequence)
	{
		Output.ResetToRefPose();
		return;
	}
	if (!Previous.Sequence || CurrentLayerWeight >= 1.0f - ZERO_ANIMWEIGHT_THRESH)
	{
		SampleLayer(Current, Output);
		return;
	}

	// two scratch poses, so neither sample writes over the other
	FPoseContext CurrentPose(Output);
	FPoseContext PreviousPose(Output);
	SampleLayer(Current, CurrentPose);
	SampleLayer(Previous, PreviousPose);
	FAnimationPoseData OutputPoseData(Output);
	FAnimationRuntime::BlendTwoPosesTogether(FAnimationPoseData(CurrentPose), FAnimationPoseData(PreviousPose), CurrentLayerWeight, OutputPoseData);
}

void FTargetAnimInstanceProxy::SampleLayer(const FTargetBaseLayer& Layer, FPoseContext& Output) const
{
	// sampled like a sequence player node does it
	FAnimationPoseData PoseData(Output);
	FAnimExtractContext Extraction(static_cast<double>(Layer.Time), ShouldExtractRootMotion(), Layer.DeltaTimeRecord, Layer.bLooping);
	Extraction.InterpolationOverride = InterpolationOverride;
	Layer.Sequence->GetAnimationPose(PoseData, Extraction);
}

void FTargetAnimInstanceProxy::PostUpdate(UAnimInstance* InAnimInstance) const
{
	// the base call fires the montage notifies of the slot
	FAnimInstanceProxy::PostUpdate(InAnimInstance);

	UTargetAnimInstance* Instance = CastChecked<UTargetAnimInstance>(InAnimInstance);
	Instance->PublishedBaseTime = Current.Time;
	if (!Previous.Sequence)
	{
		Instance->OutgoingSequence = nullptr;
	}
}
