// Copyright 2019 Lipeng Zha, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "HotPatcherStyle.h"

class FHotPatcherCommands : public TCommands<FHotPatcherCommands>
{
public:

	FHotPatcherCommands()
		: TCommands<FHotPatcherCommands>(TEXT("HotPatcher"), NSLOCTEXT("Contexts", "HotPatcher", "HotPatcher Plugin"), NAME_None, FHotPatcherStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > PluginAction;
};
