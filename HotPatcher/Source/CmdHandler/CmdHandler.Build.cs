// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System;
using System.Collections.Generic;
using System.IO;

public class CmdHandler : ModuleRules
{
	public CmdHandler(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		bLegacyPublicIncludePaths = false;
		OptimizeCode = CodeOptimization.InShippingBuildsOnly;
		if (Target.Version.MajorVersion < 5 && Target.Version.MinorVersion <= 21)
		{
			bUseRTTI = true;
		}
		
		PublicIncludePaths.AddRange(
			new string[] {
				Path.Combine(ModuleDirectory,"Public/CommandletBase")
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"UnrealEd",
				"Core",
                "Json",
                "SandboxFile",
                "JsonUtilities",
                "Settings",
                // ... add other public dependencies that you statically link with here ...
			}
			);
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"InputCore",
				"CoreUObject",
				"Engine",
				// ... add private dependencies that you statically link with here ...	
			}
		);
	}
}
