#pragma once

#include "CoreMinimal.h"
#include "FHotPatcherModelBase.h"

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

struct HOTPATCHEREDITOR_API FHotPatcherCreatePatchModel: public FHotPatcherModelBase
{
public:
	virtual FName GetModelName()const override{ return TEXT("Patch"); }
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