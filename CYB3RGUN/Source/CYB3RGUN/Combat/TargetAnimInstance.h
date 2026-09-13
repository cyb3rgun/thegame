// CYB3RGUN THEGAME. Native anim instance of target bodies: a base sequence with a cross fade, reactions blended over it.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimationAsset.h"
#include "Animation/AnimTypes.h"
#include "TargetAnimInstance.generated.h"

class UAnimMontage;
class UAnimSequenceBase;
class USkeletalMeshComponent;

/** One base layer of the target proxy. Touched on the worker thread, and in PreUpdate and PostUpdate on the game thread. */
struct FTargetBaseLayer
{
	/** Kept alive by the properties of UTargetAnimInstance */
	UAnimSequenceBase* Sequence = nullptr;

	float Time = 0.0f;
	float PlayRate = 1.0f;
	bool bLooping = true;
	FDeltaTimeRecord DeltaTimeRecord;
	FMarkerTickRecord MarkerTickRecord;
};

/**
 *  Proxy without a node graph: samples one base sequence, two while it cross fades, and lays DefaultSlot over it the way
 *  FAnimNode_Slot does, so montages blend over the base without an Anim Blueprint. A plain struct on purpose.
 */
struct FTargetAnimInstanceProxy : public FAnimInstanceProxy
{
	FTargetAnimInstanceProxy() = default;
	explicit FTargetAnimInstanceProxy(UAnimInstance* InAnimInstance) : FAnimInstanceProxy(InAnimInstance) {}

protected:

	virtual void Initialize(UAnimInstance* InAnimInstance) override;
	virtual void PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds) override;
	virtual void UpdateAnimationNode(const FAnimationUpdateContext& InContext) override;
	virtual bool Evaluate(FPoseContext& Output) override;
	virtual void PostUpdate(UAnimInstance* InAnimInstance) const override;

private:

	/** Hands a layer to the sync scope, which advances its time, loops or holds it and fires its notifies */
	void QueueLayerTick(const FAnimationUpdateContext& InContext, FTargetBaseLayer& Layer, float Weight);

	void SampleLayer(const FTargetBaseLayer& Layer, FPoseContext& Output) const;

	/** The base pose: the current layer, blended with the previous one while the cross fade runs */
	void EvaluateBase(FPoseContext& Output) const;

	FTargetBaseLayer Current;

	/** The layer being faded out */
	FTargetBaseLayer Previous;

	float BlendDuration = 0.0f;
	float BlendElapsed = 0.0f;

	/** Weight of the current layer against the previous one */
	float CurrentLayerWeight = 1.0f;

	/** Slot weights worked out in the update and used by the evaluation, as FAnimNode_Slot keeps them */
	FSlotNodeWeightInfo SlotWeights;
};

/**
 *  Anim instance of every target body: door occupants, hostage takers and enemies. It replaces the single node mode those
 *  bodies used, which cannot blend a montage over a sequence. The base is a looped or held sequence with an optional cross
 *  fade, hit reactions play on DefaultSlot over it. Root motion is ignored, so a body never drifts off its hit shapes and a
 *  character's movement keeps the capsule.
 */
UCLASS(Transient, NotBlueprintable)
class CYB3RGUN_API UTargetAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:

	UTargetAnimInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** The target anim instance a body runs, null when it runs another or none */
	static UTargetAnimInstance* FromMesh(const USkeletalMeshComponent* Mesh);

	/** Switches a body to this anim instance unless it already runs it, and returns it. Null before the body is registered. */
	static UTargetAnimInstance* Ensure(USkeletalMeshComponent* Mesh);

	/** Replaces USkeletalMeshComponent::PlayAnimation. A blend time above zero cross fades from the current base. */
	void PlayBase(UAnimSequenceBase* Sequence, bool bLooping, float PlayRate = 1.0f, float StartTime = 0.0f, float BlendTime = 0.0f);

	/** Replaces USkeletalMeshComponent::SetPlayRate */
	void SetBasePlayRate(float PlayRate);

	/** Replaces USkeletalMeshComponent::SetPosition */
	void SetBaseTime(float Time);

	/** The base sequence last asked for */
	UAnimSequenceBase* GetBaseSequence() const { return BaseSequence.Get(); }

	/** Base time as of the last finished update */
	float GetBaseTime() const { return PublishedBaseTime; }

	/** Plays a reaction on DefaultSlot, blended over the base. Returns the montage made for it, null when it cannot play. */
	UAnimMontage* PlayReaction(UAnimSequenceBase* Reaction, float BlendIn, float BlendOut, float PlayRate = 1.0f);

protected:

	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
	virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy) override;

private:

	friend struct FTargetAnimInstanceProxy;

	/** Base sequence asked for, kept alive here because the proxy only holds a raw pointer */
	UPROPERTY(Transient)
	TObjectPtr<UAnimSequenceBase> BaseSequence;

	/** Sequence the proxy fades out from, kept alive until the fade has run */
	UPROPERTY(Transient)
	TObjectPtr<UAnimSequenceBase> OutgoingSequence;

	/** Requests of the game thread, taken over by the proxy in PreUpdate */
	float RequestedPlayRate = 1.0f;
	float RequestedTime = 0.0f;
	float RequestedBlendTime = 0.0f;
	bool bRequestedLooping = true;
	bool bBaseRequestPending = false;
	bool bRateRequestPending = false;
	bool bTimeRequestPending = false;

	/** Written by the proxy in PostUpdate */
	float PublishedBaseTime = 0.0f;
};
