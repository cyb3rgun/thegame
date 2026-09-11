// CYB3RGUN THEGAME. The rail's crosshair: the logo crosshair, under its own class for WBP_RailCrosshair.

#pragma once

#include "CoreMinimal.h"
#include "LogoCrosshairWidget.h"
#include "RailCrosshairWidget.generated.h"

/** The rail module's crosshair. All of it is the logo crosshair (D-048), the class stays so WBP_RailCrosshair and GM_RailTest keep their references. */
UCLASS()
class CYB3RGUN_API URailCrosshairWidget : public ULogoCrosshairWidget
{
	GENERATED_BODY()
};
