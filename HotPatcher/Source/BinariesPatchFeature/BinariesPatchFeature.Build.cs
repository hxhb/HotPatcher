// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

using System;
using System.IO;
using UnrealBuildTool;

public class BinariesPatchFeature : ModuleRules
{
	public BinariesPatchFeature(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				Path.Combine(EngineDirectory,"Source/Runtime/Launch"),
				Path.Combine(ModuleDirectory,"Public"),
				Path.Combine(ModuleDirectory,"../HotPatcherRuntime/Public/Templates")
				// ... add public include paths required here ...
			}
			);

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"Projects",
				"Json",
				"JsonUtilities",
				// ... add other public dependencies that you statically link with here ...
			}
			);
		bLegacyPublicIncludePaths = false;
		OptimizeCode = CodeOptimization.InShippingBuildsOnly;
	}
}
