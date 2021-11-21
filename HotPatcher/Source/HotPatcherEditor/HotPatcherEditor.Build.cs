// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;

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
				"UnrealEd",
				"UMG",
				"UMGEditor",
				"Core",
                "Json",
                "ContentBrowser",
                "SandboxFile",
                "JsonUtilities",
                "TargetPlatform",
                "PropertyEditor",
                "DesktopPlatform",
                "Projects",
                "Settings",
                "HTTP",
                "AssetRegistry",
                "AssetManagerEx",
                "PakFileUtilities",
                "HotPatcherRuntime",
                "BinariesPatchFeature"
                // ... add other public dependencies that you statically link with here ...
			}
			);
		
		// only in UE5
		if (Target.Version.MajorVersion > 4)
		{
			PublicDependencyModuleNames.AddRange(new string[]
			{
				"DeveloperToolSettings"
			});
		}
		
		if (Target.Version.MajorVersion > 4 || Target.Version.MinorVersion > 23)
		{
			PublicDependencyModuleNames.Add("ToolMenus");
		}

		System.Func<string, bool,bool> AddPublicDefinitions = (string MacroName,bool bEnable) =>
		{
			PublicDefinitions.Add(string.Format("{0}={1}",MacroName, bEnable ? 1 : 0));
			return true;
		};
		
		AddPublicDefinitions("GENERATE_ASSET_REGISTRY_DATA", false);
		
		bool bIOStoreSupport = Target.Version.MajorVersion > 4 || Target.Version.MinorVersion > 25;
		if (bIOStoreSupport)
		{
			PublicDependencyModuleNames.AddRange(new string[]
			{
				"IoStoreUtilities"
			});
		}
		AddPublicDefinitions("WITH_IO_STORE_SUPPORT", bIOStoreSupport);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
                "Core",
                "UnrealEd",
                "Projects",
                "DesktopPlatform",
				"InputCore",
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
		
		switch (Target.Configuration)
		{
			case UnrealTargetConfiguration.DebugGame:
			{
				PublicDefinitions.Add("WITH_HOTPATCHER_DEBUGGAME");
				break;
			}
			case UnrealTargetConfiguration.Development:
			{
				PublicDefinitions.Add("WITH_HOTPATCHER_DEVELOPMENT");
				break;
			}
		};
		
		PublicDefinitions.AddRange(new string[]
		{
			"ENABLE_COOK_ENGINE_MAP=0",
			"ENABLE_COOK_PLUGIN_MAP=0"
		});
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
		if (Target.Version.MajorVersion < 5 && Target.Version.MinorVersion <= 21)
		{
			bUseRTTI = true;
		}


		BuildVersion Version;
		BuildVersion.TryRead(BuildVersion.GetDefaultFileName(), out Version);
		// PackageContext

		AddPublicDefinitions("WITH_EDITOR_SECTION", Version.MajorVersion > 4 || Version.MinorVersion > 24);
		
		System.Console.WriteLine("MajorVersion {0} MinorVersion: {1} PatchVersion {2}",Target.Version.MajorVersion,Target.Version.MinorVersion,Target.Version.PatchVersion);
		bLegacyPublicIncludePaths = false;
		OptimizeCode = CodeOptimization.InShippingBuildsOnly;

		// Game feature
		bool bEnableGameFeature = false;
		if (bEnableGameFeature || (Target.Version.MajorVersion > 4 || Target.Version.MinorVersion > 26))
		{
			PublicDefinitions.AddRange(new string[]
			{
				"ENGINE_GAME_FEATURE"
			});
			PublicDependencyModuleNames.AddRange(new string[]
			{
				// "GameFeatures",
				// "ModularGameplay",
			});
		}
		
		PublicDefinitions.AddRange(new string[]
		{
			"TOOL_NAME=\"HotPatcher\"",
			"CURRENT_VERSION_ID=70",
			"REMOTE_VERSION_FILE=\"https://imzlp.com/opensource/version.json\""
		});

		bool bEnablePackageContext = true;
		AddPublicDefinitions("WITH_PACKAGE_CONTEXT", (Version.MajorVersion > 4 || Version.MinorVersion > 24) && bEnablePackageContext);
		if (Version.MajorVersion > 4 || Version.MajorVersion > 26)
		{
			PublicDependencyModuleNames.AddRange(new string[]
			{
				"IoStoreUtilities",
				"UnrealEd"
			});
		}
		
		if (Version.MajorVersion > 4 && Version.MinorVersion > 0)
		{
			PublicIncludePaths.AddRange(new List<string>()
			{
				// Path.Combine(EngineDirectory,"Source/Developer/IoStoreUtilities/Internal"),
				// Path.Combine(EngineDirectory,"Source/Editor/UnrealEd/Private/Cooker"),
				// Path.Combine(EngineDirectory,"Source/Editor/UnrealEd/Private"),
				// Path.Combine(EngineDirectory,"Source/Runtime/CoreUObject/Internal"),
				Path.Combine(ModuleDirectory,"../CookerWriterForUE5")
			});
		}
	}
}
