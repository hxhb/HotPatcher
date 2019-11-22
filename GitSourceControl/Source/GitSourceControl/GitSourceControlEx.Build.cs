// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class GitSourceControlEx : ModuleRules
{
	public GitSourceControlEx(ReadOnlyTargetRules Target) : base(Target)
	{
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"Slate",
				"SlateCore",
				"InputCore",
				"DesktopWidgets",
				"EditorStyle",
				"SourceControl",
			}
		);

		if (Target.bBuildEditor == true)
		{
			// needed to enable/disable this via experimental settings
			PrivateDependencyModuleNames.Add("CoreUObject");
			PrivateDependencyModuleNames.Add("UnrealEd");
		}

        OptimizeCode = CodeOptimization.InShippingBuildsOnly;

    }
}
