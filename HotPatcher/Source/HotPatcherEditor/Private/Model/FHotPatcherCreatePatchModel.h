#pragma once

#include "CoreMinimal.h"

namespace EHotPatcherActionModes
{
	enum Type
	{
		ByPatch,
		ByRelease
	};
}

class FHotPatcherCreatePatchModel
{
public:
	
	void SetPatcherMode(EHotPatcherActionModes::Type InPatcherMode)
	{
		PatcherMode = InPatcherMode;
	}
	EHotPatcherActionModes::Type GetPatcherMode()
	{
		return PatcherMode;
	}

private:

	EHotPatcherActionModes::Type PatcherMode;
};