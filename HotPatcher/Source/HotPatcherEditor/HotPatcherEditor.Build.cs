// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class HotPatcherEditor : ModuleRules
{
	public HotPatcherEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
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
				"Core",
                "Json",
                "TargetPlatform",
                "PropertyEditor",
                "AssetManagerEx",
                "PakFileUtilities"
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
                "Core",
                "Projects",
                "DesktopPlatform",
				"InputCore",
				"UnrealEd",
                "EditorStyle",
                "LevelEditor",
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"HotPatcherRuntime"
				// ... add private dependencies that you statically link with here ...	
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);

        OptimizeCode = CodeOptimization.InShippingBuildsOnly;
        
	}
}
