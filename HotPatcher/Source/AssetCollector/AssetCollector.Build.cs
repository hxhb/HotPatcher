// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

using System;
using System.IO;
using UnrealBuildTool;

public class AssetCollector : ModuleRules
{
	public AssetCollector(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				Path.Combine(EngineDirectory,"Source/Runtime/Launch"),
				Path.Combine(ModuleDirectory,"Public")
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
				"UnrealEd",
				"Projects",
				"TargetPlatform"
				// ... add other public dependencies that you statically link with here ...
			}
			);
		bLegacyPublicIncludePaths = false;
		OptimizeCode = CodeOptimization.InShippingBuildsOnly;
	}
}
