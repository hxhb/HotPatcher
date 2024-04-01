// Copyright 2019 Lipeng Zha, Inc. All Rights Reserved.

#pragma once

// engine header
#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

DECLARE_LOG_CATEGORY_EXTERN(LogCmdHandler,All,All);

class FCmdHandlerModule : public IModuleInterface
{
public:
	static FCmdHandlerModule& Get();
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
