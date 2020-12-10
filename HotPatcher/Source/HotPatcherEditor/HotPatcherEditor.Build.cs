// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System;
using System.IO;

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
                "ContentBrowser",
                "SandboxFile",
                "JsonUtilities",
                "TargetPlatform",
                "PropertyEditor",
                "AssetManagerEx",
                "PakFileUtilities",
                "HotPatcherRuntime"
				// ... add other public dependencies that you statically link with here ...
			}
			);
		if (Target.Version.MinorVersion > 23)
		{
			PublicDependencyModuleNames.Add("ToolMenus");
		}

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
				"RenderCore"
				// ... add private dependencies that you statically link with here ...	
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
		if (Target.Version.MinorVersion <= 21)
		{
			bUseRTTI = true;
		}
		System.Console.WriteLine("MajorVersion {0} MinorVersion: {1} PatchVersion {2}",Target.Version.MajorVersion,Target.Version.MinorVersion,Target.Version.PatchVersion);
		bLegacyPublicIncludePaths = false;
		OptimizeCode = CodeOptimization.InShippingBuildsOnly;
        
	}
}
