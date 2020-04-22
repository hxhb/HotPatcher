// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class AssetManagerEx : ModuleRules
{
	public AssetManagerEx(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.AddRange(
            new string[] {
				// ... add public include paths required here ...
                Path.Combine(ModuleDirectory,"Public/Flib"),
                Path.Combine(ModuleDirectory,"Public/Struct")
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
				"Core",
                "Json",
                "Projects",
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "Json"
                // "TargetPlatform",
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{	
				// ... add private dependencies that you statically link with here ...	
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
                "AssetRegistry"
				// ... add any modules that your module loads dynamically here ...
			}
			);

        OptimizeCode = CodeOptimization.InShippingBuildsOnly;
    }
}
