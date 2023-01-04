#pragma once

#include "CoreMinimal.h"
#include "HotPatcherActionManager.h"
#include "Modules/ModuleManager.h"

class HOTPATCHEREDITOR_API FHotPatcherModBaseModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	virtual FHotPatcherModDesc GetModDesc()const { return ModDesc; };
private:
	FHotPatcherModDesc ModDesc;
};
