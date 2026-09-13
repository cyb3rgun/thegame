// CYB3RGUN THEGAME. The rules of one carried weapon: rounds, the action, draw, reload and bloom, for every holder (D-052).

#include "WeaponState.h"
#include "WeaponDefinition.h"
#include "WeaponStatus.h"
#include "CoreGlobals.h"

void FWeaponState::Init(const UWeaponDefinition* InDefinition)
{
	Definition = InDefinition;
	Rounds = Definition ? Definition->MagazineSize : 0;
	Action = EWeaponAction::Ready;
	ActionElapsed = 0.0f;
	ActionDuration = 0.0f;
	RefireRemaining = 0.0f;
	Bloom = 0.0f;
}

void FWeaponState::Tick(float Delta)
{
	if (!Definition)
	{
		return;
	}

	RefireRemaining = FMath::Max(RefireRemaining - Delta, 0.0f);
	Bloom = FMath::Max(Bloom - Definition->BloomRecoveryDegreesPerSecond * Delta, 0.0f);

	if (Action != EWeaponAction::Ready && GFrameCounter != ActionStartFrame)
	{
		ActionElapsed += Delta;
		if (ActionElapsed >= ActionDuration)
		{
			FinishAction();
		}
	}
}

void FWeaponState::StartAction(EWeaponAction NewAction, float Duration)
{
	Action = NewAction;
	ActionElapsed = 0.0f;
	ActionDuration = FMath::Max(Duration, 0.0f);
	ActionStartFrame = GFrameCounter;
	if (ActionDuration <= 0.0f)
	{
		FinishAction();
	}
}

void FWeaponState::FinishAction()
{
	if (Action == EWeaponAction::Reloading && Definition)
	{
		Rounds = Definition->MagazineSize;
	}
	Action = EWeaponAction::Ready;
	ActionElapsed = 0.0f;
	ActionDuration = 0.0f;
}

void FWeaponState::Draw()
{
	if (Definition)
	{
		StartAction(EWeaponAction::Drawing, Definition->EquipSeconds);
	}
}

bool FWeaponState::CanFire() const
{
	return Definition && Action == EWeaponAction::Ready && RefireRemaining <= 0.0f && Rounds > 0;
}

bool FWeaponState::IsEmpty() const
{
	return Rounds <= 0;
}

FWeaponShot FWeaponState::Fire()
{
	FWeaponShot Shot;
	if (!Definition)
	{
		return Shot;
	}

	Shot.Damage = Definition->Damage;
	Shot.Speed = Definition->GetMuzzleSpeed(Definition->MuzzleEnergy);
	Shot.GravityScale = Definition->DropScale;
	Shot.ConeDegrees = Definition->AccuracyConeDegrees + Bloom;
	Shot.Pellets = Definition->Pellets;

	Rounds = FMath::Max(Rounds - 1, 0);
	Bloom = FMath::Min(Bloom + Definition->BloomPerShotDegrees, Definition->MaxBloomDegrees);

	// a single shot weapon works its action after every shot, the others only wait their refire time
	if (Definition->FireMode == EWeaponFireMode::SingleShot)
	{
		StartAction(EWeaponAction::Cycling, Definition->CycleSeconds);
	}
	else
	{
		RefireRemaining = Definition->RefireSeconds;
	}
	return Shot;
}

bool FWeaponState::StartReload()
{
	if (!Definition || Action != EWeaponAction::Ready || Rounds >= Definition->MagazineSize)
	{
		return false;
	}
	StartAction(EWeaponAction::Reloading, Definition->ReloadSeconds);
	return true;
}

void FWeaponState::CancelReload()
{
	if (Action == EWeaponAction::Reloading)
	{
		Action = EWeaponAction::Ready;
		ActionElapsed = 0.0f;
		ActionDuration = 0.0f;
	}
}

float FWeaponState::GetActionProgress() const
{
	return ActionDuration > 0.0f ? FMath::Clamp(ActionElapsed / ActionDuration, 0.0f, 1.0f) : 0.0f;
}

void FWeaponState::FillStatus(FWeaponStatus& OutStatus) const
{
	if (!Definition)
	{
		return;
	}

	OutStatus.WeaponName = Definition->DisplayName;
	OutStatus.Subtitle = Definition->Subtitle;
	OutStatus.MakerMark = Definition->MakerMark;
	OutStatus.Rounds = Rounds;
	OutStatus.MagazineSize = Definition->MagazineSize;
	OutStatus.bReloading = Action == EWeaponAction::Reloading;
	OutStatus.ReloadProgress = OutStatus.bReloading ? GetActionProgress() : 0.0f;
	OutStatus.bCycling = Action == EWeaponAction::Cycling;
	OutStatus.CycleProgress = OutStatus.bCycling ? GetActionProgress() : 0.0f;
	OutStatus.bSwitching = Action == EWeaponAction::Drawing;
	OutStatus.LastDryFireTime = LastDryFireTime;
}
