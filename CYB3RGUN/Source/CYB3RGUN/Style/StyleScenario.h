// CYB3RGUN THEGAME. A scenario that brings its own style values.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "StyleScenario.generated.h"

class UStyleSettings;

UINTERFACE(MinimalAPI)
class UStyleScenario : public UInterface
{
	GENERATED_BODY()
};

/**
 *  Implemented by game modes. A scenario that needs different style values, for example a precision scenario
 *  without Overclock, returns its own asset; null keeps the project default.
 */
class CYB3RGUN_API IStyleScenario
{
	GENERATED_BODY()

public:

	virtual const UStyleSettings* GetStyleSettings() const = 0;
};
