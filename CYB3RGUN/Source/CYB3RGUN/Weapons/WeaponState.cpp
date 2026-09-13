// CYB3RGUN THEGAME. The rules of one carried weapon: rounds or air, the action, draw, reload and bloom, for every holder (D-052).

#include "WeaponState.h"
#include "WeaponDefinition.h"
#include "WeaponStatus.h"
#include "CoreGlobals.h"

void FWeaponState::Init(const UWeaponDefinition* InDefinition)
{
	Definition = InDefinition;
	Rounds = Definition ? Definition->MagazineSize : 0;
	Pressure = Definition && Definition->IsPressureFed() ? Definition->FillPressureBar : 0.0f;
	RefillsLeft = Definition && Definition->IsPressureFed() ? Definition->RefillsCarried : 0;
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
		// a refill empties one charge of the carried supply into the reservoir
		if (Definition->IsPressureFed())
		{
			Pressure = Definition->FillPressureBar;
			RefillsLeft = FMath::Max(RefillsLeft - 1, 0);
		}
		else
		{
			Rounds = Definition->MagazineSize;
		}
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
	return Definition && Action == EWeaponAction::Ready && RefireRemaining <= 0.0f && !IsEmpty();
}

bool FWeaponState::IsEmpty() const
{
	if (Definition && Definition->IsPressureFed())
	{
		return Pressure < Definition->MinFirePressureBar;
	}
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

	if (Definition->IsPressureFed())
	{
		// the valve opens on the pressure the reservoir holds now: it sets the energy, the drop and the spread (D-055)
		Shot.PressureBar = Pressure;
		Shot.EnergyShare = Definition->GetEnergyShare(Pressure);
		Shot.Damage *= Shot.EnergyShare;
		Shot.Speed = Definition->GetMuzzleSpeed(Definition->MuzzleEnergy * Shot.EnergyShare);
		Shot.GravityScale *= Definition->GetDropMultiplier(Pressure);
		Shot.ConeDegrees += Definition->GetPressureCone(Pressure);
		Pressure = FMath::Max(Pressure * (1.0f - Definition->PressureUsePerShot), 0.0f);
	}
	else
	{
		Rounds = FMath::Max(Rounds - 1, 0);
	}
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
	// a reload may take over a cycling action, which it outlasts; a draw or a running reload has to finish first
	if (!Definition || (Action != EWeaponAction::Ready && Action != EWeaponAction::Cycling))
	{
		return false;
	}

	// a refill needs a charge left in the supply and room in the reservoir
	if (Definition->IsPressureFed())
	{
		if (RefillsLeft <= 0 || Pressure >= Definition->FillPressureBar)
		{
			return false;
		}
	}
	else if (Rounds >= Definition->MagazineSize)
	{
		return false;
	}

	StartAction(EWeaponAction::Reloading, GetReloadSeconds());
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

float FWeaponState::GetReloadSeconds() const
{
	if (!Definition)
	{
		return 0.0f;
	}
	return Definition->IsPressureFed() ? Definition->RefillSeconds : Definition->ReloadSeconds;
}

FString FWeaponState::DescribeAmmo() const
{
	if (!Definition)
	{
		return FString();
	}
	if (Definition->IsPressureFed())
	{
		return FString::Printf(TEXT("%.0f of %.0f bar, %d refills"), Pressure, Definition->FillPressureBar, RefillsLeft);
	}
	return FString::Printf(TEXT("%d of %d rounds"), Rounds, Definition->MagazineSize);
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
	OutStatus.bPressureFed = Definition->IsPressureFed();
	if (OutStatus.bPressureFed)
	{
		OutStatus.PressureBar = Pressure;
		OutStatus.FillPressureBar = Definition->FillPressureBar;
		OutStatus.MinFirePressureBar = Definition->MinFirePressureBar;
		OutStatus.EnergyShare = Definition->GetEnergyShare(Pressure);
		OutStatus.RefillsLeft = RefillsLeft;
	}
	OutStatus.bEmpty = IsEmpty();
	OutStatus.bReloading = Action == EWeaponAction::Reloading;
	OutStatus.ReloadProgress = OutStatus.bReloading ? GetActionProgress() : 0.0f;
	OutStatus.bCycling = Action == EWeaponAction::Cycling;
	OutStatus.CycleProgress = OutStatus.bCycling ? GetActionProgress() : 0.0f;
	OutStatus.bSwitching = Action == EWeaponAction::Drawing;
	OutStatus.LastDryFireTime = LastDryFireTime;
}
