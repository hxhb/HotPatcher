// Copyright 2019 Lipeng Zha, Inc. All Rights Reserved.

#include "HotPatcherCommands.h"

#define LOCTEXT_NAMESPACE "FHotPatcherModule"

void FHotPatcherCommands::RegisterCommands()
{
	UI_COMMAND(PluginAction, "HotPatcher", "Hot Patch Manager", EUserInterfaceActionType::Button, FInputChord{});
	UI_COMMAND(CookSelectedAction, "Cook Actions...", "Cook the selected assets", EUserInterfaceActionType::Button, FInputChord{});
	UI_COMMAND(CookAndPakSelectedAction, "Cook And Pak Actions...", "Cook and Pak the selected assets", EUserInterfaceActionType::Button, FInputChord{});
	UI_COMMAND(AddToPakSettingsAction, "Add To Patch Settings...", "Add the selected assets to Patch Settings", EUserInterfaceActionType::Button, FInputChord{});
}

#undef LOCTEXT_NAMESPACE
