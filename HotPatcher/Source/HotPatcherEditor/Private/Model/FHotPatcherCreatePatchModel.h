#pragma once

#include "CoreMinimal.h"

namespace EHotPatcherActionModes
{
	enum Type
	{
		ByPatch,
		ByRelease,
		ByShaderPatch,
		ByGameFeature
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