// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class GameFeaturePackerEditor : ModuleRules
{
	public GameFeaturePackerEditor(ReadOnlyTargetRules Target) : base(Target)
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
				"JsonUtilities",
				"Projects",
				"TargetPlatform",
				"DesktopPlatform",
				"EditorStyle",
				"HotPatcherRuntime",
				"HotPatcherCore",
				"HotPatcherEditor"
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				// ... add private dependencies that you statically link with here ...	
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
		System.Func<string, bool,bool> AddPublicDefinitions = (string MacroName,bool bEnable) =>
		{
			PublicDefinitions.Add(string.Format("{0}={1}",MacroName, bEnable ? 1 : 0));
			return true;
		};
		
		bool bEnableGameFeature = false;
		AddPublicDefinitions("ENGINE_GAME_FEATURE", bEnableGameFeature || (Target.Version.MajorVersion > 4 || Target.Version.MinorVersion > 26));
		
		PublicDefinitions.AddRange(new string[]
		{
			"MOD_NAME=TEXT(\"GameFeaturePacker\")",
			"MOD_VERSION=1.0",
			"IS_INTERNAL_MODE=true"
		});
	}
}
