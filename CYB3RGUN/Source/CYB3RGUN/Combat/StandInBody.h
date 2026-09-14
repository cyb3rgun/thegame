// CYB3RGUN THEGAME. Primitive stand ins for target bodies whose closed tier character model is absent (D-068).

#pragma once

#include "CoreMinimal.h"

class UMaterialInterface;
class UObject;
class USkeletalMeshComponent;
struct FHitResult;

DECLARE_LOG_CATEGORY_EXTERN(LogClosedContent, Log, All);

/**
 *  The open tier runs without the closed tier (D-067, D-068). A target body whose character model is missing gets a
 *  primitive stand in instead of turning invisible and unshootable: a cylinder torso and a sphere head attached to the body,
 *  shown, painted and switched shootable together with it. A shot on a part is handed on as a shot on the body at the bone
 *  the part stands for, so zones, scoring, disarms and rescues keep working; animations have nothing to play and are
 *  skipped. One line per kind of body goes to LogClosedContent.
 */
struct CYB3RGUN_API FStandInBody
{
	/** Loads content that may be absent, without the error a constructor finder raises for a missing asset. Null when absent. */
	static UObject* LoadOptional(UClass* Class, const TCHAR* Path);

	template <typename T>
	static T* LoadOptional(const TCHAR* Path)
	{
		return Cast<T>(LoadOptional(T::StaticClass(), Path));
	}

	/**
	 *  Gives a body without a character model its stand in, once, sized for a figure of Height centimetres at scale 1 with
	 *  its feet at the body's origin. Logs one line per kind, named by What. True when the body stands in.
	 */
	static bool Ensure(USkeletalMeshComponent* Body, const FString& What, float Height = 180.0f);

	/** Where a held object sits on a stand in, in the body's space */
	static FTransform GetHandTransform();

	/** Writes the one line for a kind of content that is absent and what stands in for it, once per kind per session */
	static void ReportAbsent(const FString& What, const FString& StandIn);

	/** True when the body shows a stand in */
	static bool IsStandingIn(const USkeletalMeshComponent* Body);

	/** A hit on a stand in part as a hit on its body at the bone the part stands for; any other hit comes back unchanged */
	static FHitResult Redirect(const FHitResult& Hit);

	/** Switches the stand in parts between shootable and not, like the body */
	static void SetShootable(USkeletalMeshComponent* Body, bool bShootable);

	/** Paints the stand in parts */
	static void Paint(USkeletalMeshComponent* Body, UMaterialInterface* Material);

	/** The point on the stand in a shot at a bone aims for: the head sphere for the head, the torso otherwise */
	static bool GetBonePoint(const USkeletalMeshComponent* Body, FName Bone, FVector& OutPoint);
};
