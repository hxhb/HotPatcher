// Copyright 2019 Lipeng Zha, Inc. All Rights Reserved.

#include "HotPatcherCommands.h"

#define LOCTEXT_NAMESPACE "FHotPatcherModule"

void FHotPatcherCommands::RegisterCommands()
{
	UI_COMMAND(PluginAction, "HotPatcher", "Hot Patch Manager", EUserInterfaceActionType::Button, FInputGesture());
}

#undef LOCTEXT_NAMESPACE
