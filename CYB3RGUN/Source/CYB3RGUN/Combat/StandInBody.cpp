// CYB3RGUN THEGAME. Primitive stand ins for target bodies whose closed tier character model is absent (D-068).

#include "StandInBody.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/HitResult.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"

DEFINE_LOG_CATEGORY(LogClosedContent);

namespace StandInParts
{
	/** Every part carries this tag first and the bone it stands for second */
	const FName PartTag(TEXT("StandInPart"));

	const FName HeadBone(TEXT("head"));
	const FName ChestBone(TEXT("spine_03"));

	/** Kinds of body already reported, so each kind logs once per session */
	TSet<FString>& Reported()
	{
		static TSet<FString> Kinds;
		return Kinds;
	}

	UStaticMeshComponent* MakePart(USkeletalMeshComponent* Body, const TCHAR* Mesh, FName Bone, const FVector& Location, const FVector& Scale)
	{
		UStaticMesh* Shape = LoadObject<UStaticMesh>(nullptr, Mesh);
		UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(Body->GetOwner(), NAME_None, RF_Transient);
		Part->SetStaticMesh(Shape);
		Part->ComponentTags.Add(PartTag);
		Part->ComponentTags.Add(Bone);
		Part->SetupAttachment(Body);
		Part->SetRelativeLocation(Location);
		Part->SetRelativeScale3D(Scale);

		// the same target setup as the body's physics asset bodies: world static, blocking every shot, off until shown
		Part->SetCollisionObjectType(ECC_WorldStatic);
		Part->SetCollisionResponseToAllChannels(ECR_Block);
		Part->SetCollisionEnabled(Body->IsCollisionEnabled() ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
		Part->SetGenerateOverlapEvents(false);
		Part->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
		Part->SetVisibility(Body->IsVisible());
		Part->RegisterComponent();
		return Part;
	}

	void ForEachPart(const USkeletalMeshComponent* Body, TFunctionRef<void(UStaticMeshComponent*)> Visit)
	{
		if (!Body)
		{
			return;
		}
		for (USceneComponent* Child : Body->GetAttachChildren())
		{
			UStaticMeshComponent* Part = Cast<UStaticMeshComponent>(Child);
			if (Part && Part->ComponentTags.Num() >= 2 && Part->ComponentTags[0] == PartTag)
			{
				Visit(Part);
			}
		}
	}
}

UObject* FStandInBody::LoadOptional(UClass* Class, const TCHAR* Path)
{
	// only ask for the object when its package is there, so an absent closed tier raises no load error
	const FString PackageName = FPackageName::ObjectPathToPackageName(FString(Path));
	if (!FPackageName::DoesPackageExist(PackageName))
	{
		return nullptr;
	}
	return StaticLoadObject(Class, nullptr, Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
}

bool FStandInBody::Ensure(USkeletalMeshComponent* Body, const FString& What, float Height)
{
	if (!Body || Body->GetSkeletalMeshAsset())
	{
		return false;
	}
	if (IsStandingIn(Body))
	{
		return true;
	}

	// the engine's basic shapes are 100 cm across with their pivot in the middle
	const float Torso = Height * 0.72f;
	const float Head = Height * 0.15f;
	StandInParts::MakePart(Body, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"), StandInParts::ChestBone, FVector(0.0f, 0.0f, Torso * 0.5f), FVector(0.45f, 0.3f, Torso / 100.0f));
	StandInParts::MakePart(Body, TEXT("/Engine/BasicShapes/Sphere.Sphere"), StandInParts::HeadBone, FVector(0.0f, 0.0f, Torso + Head * 0.55f), FVector(Head / 100.0f));

	// whatever the model held in a hand socket moves to the stand in's hand, a socket on a body without a model warns every frame
	TArray<USceneComponent*> Held(Body->GetAttachChildren());
	for (USceneComponent* Child : Held)
	{
		if (Child && !Child->GetAttachSocketName().IsNone())
		{
			Child->AttachToComponent(Body, FAttachmentTransformRules::KeepRelativeTransform, NAME_None);
			Child->SetRelativeTransform(GetHandTransform());
		}
	}

	ReportAbsent(What + TEXT(" has no character model"), TEXT("a primitive torso and head"));
	return true;
}

FTransform FStandInBody::GetHandTransform()
{
	// in front of the torso at chest height, pointing the way the body faces, which is +Y as for the mannequin
	return FTransform(FRotator(0.0f, 90.0f, 0.0f), FVector(-15.0f, 32.0f, 118.0f));
}

void FStandInBody::ReportAbsent(const FString& What, const FString& StandIn)
{
	if (!StandInParts::Reported().Contains(What))
	{
		StandInParts::Reported().Add(What);
		UE_LOG(LogClosedContent, Warning, TEXT("Closed tier content absent: %s, standing in with %s (D-068)"), *What, *StandIn);
	}
}

bool FStandInBody::IsStandingIn(const USkeletalMeshComponent* Body)
{
	bool bFound = false;
	StandInParts::ForEachPart(Body, [&bFound](UStaticMeshComponent*) { bFound = true; });
	return bFound;
}

FHitResult FStandInBody::Redirect(const FHitResult& Hit)
{
	const UPrimitiveComponent* Component = Hit.GetComponent();
	if (!Component || Component->ComponentTags.Num() < 2 || Component->ComponentTags[0] != StandInParts::PartTag)
	{
		return Hit;
	}

	USkeletalMeshComponent* Body = Cast<USkeletalMeshComponent>(Component->GetAttachParent());
	if (!Body)
	{
		return Hit;
	}

	FHitResult OnBody = Hit;
	OnBody.Component = Body;
	OnBody.BoneName = Component->ComponentTags[1];
	return OnBody;
}

void FStandInBody::SetShootable(USkeletalMeshComponent* Body, bool bShootable)
{
	StandInParts::ForEachPart(Body, [bShootable](UStaticMeshComponent* Part)
	{
		Part->SetCollisionEnabled(bShootable ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	});
}

void FStandInBody::Paint(USkeletalMeshComponent* Body, UMaterialInterface* Material)
{
	if (!Material)
	{
		return;
	}
	StandInParts::ForEachPart(Body, [Material](UStaticMeshComponent* Part) { Part->SetMaterial(0, Material); });
}

bool FStandInBody::GetBonePoint(const USkeletalMeshComponent* Body, FName Bone, FVector& OutPoint)
{
	const FName Wanted = Bone == StandInParts::HeadBone ? StandInParts::HeadBone : StandInParts::ChestBone;
	bool bFound = false;
	StandInParts::ForEachPart(Body, [&](UStaticMeshComponent* Part)
	{
		if (!bFound && Part->ComponentTags[1] == Wanted)
		{
			OutPoint = Part->GetComponentLocation();
			bFound = true;
		}
	});
	return bFound;
}
