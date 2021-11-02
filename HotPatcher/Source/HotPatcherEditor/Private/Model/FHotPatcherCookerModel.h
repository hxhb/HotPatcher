#pragma once

#include "CoreMinimal.h"

namespace EHotPatcherCookActionMode
{
	enum Type
	{
		ByOriginal,
		ByMultiProcess
	};
}

class FHotPatcherCookerModel
{
public:
	
	void SetCookerMode(EHotPatcherCookActionMode::Type InCookerMode)
	{
		CookerMode = InCookerMode;
	}
	EHotPatcherCookActionMode::Type GetCookerMode()
	{
		return CookerMode;
	}

private:

	EHotPatcherCookActionMode::Type CookerMode;
};