// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System;
using System.Collections.Generic;
using System.IO;

public class HotPatcherCore : ModuleRules
{
	public HotPatcherCore(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		bLegacyPublicIncludePaths = true;
		OptimizeCode = CodeOptimization.InShippingBuildsOnly;

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
				"UMG",
				"UMGEditor",
				"Core",
                "Json",
                "LevelSequence",
                "ContentBrowser",
                "SandboxFile",
                "JsonUtilities",
                "TargetPlatform",
                "DesktopPlatform",
                "Projects",
                "Settings",
                "HTTP",
                "RHI",
                "EngineSettings",
                "AssetRegistry",
                "PakFileUtilities",
                "HotPatcherRuntime",
                "BinariesPatchFeature"
                // ... add other public dependencies that you statically link with here ...
			}
			);
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"DesktopPlatform",
				"InputCore",
				"CoreUObject",
				"Engine",
				"Sockets",
				"DerivedDataCache"
				// ... add private dependencies that you statically link with here ...	
			}
		);
		
		if (Target.Version.MajorVersion > 4 || Target.Version.MinorVersion > 21)
		{
			PrivateDependencyModuleNames.Add("RenderCore");
		}
		else
		{
			PrivateDependencyModuleNames.Add("ShaderCore");
		}

		if (Target.Version.MajorVersion > 4 || Target.Version.MinorVersion > 23)
		{
			PublicDependencyModuleNames.AddRange(new string[]{
				"TraceLog"
			});
		}

		
		// // only in UE5
		if (Target.Version.MajorVersion > 4)
		{
			PublicDependencyModuleNames.AddRange(new string[]
			{
				"DeveloperToolSettings"
			});
		}
		
		switch (Target.Configuration)
		{
			case UnrealTargetConfiguration.Debug:
			{
				PublicDefinitions.Add("COMPILER_CONFIGURATION_NAME=\"Debug\"");
				break;
			}
			case UnrealTargetConfiguration.DebugGame:
			{
				PublicDefinitions.Add("COMPILER_CONFIGURATION_NAME=\"DebugGame\"");
				break;
			}
			case UnrealTargetConfiguration.Development:
			{
				PublicDefinitions.Add("COMPILER_CONFIGURATION_NAME=\"Development\"");
				break;
			}
			default:
			{
				PublicDefinitions.Add("COMPILER_CONFIGURATION_NAME=\"\"");
				break;
			}
		};
		
		System.Func<string, bool,bool> AddPublicDefinitions = (string MacroName,bool bEnable) =>
		{
			PublicDefinitions.Add(string.Format("{0}={1}",MacroName, bEnable ? 1 : 0));
			return true;
		};
		
		bool bIOStoreSupport = Target.Version.MajorVersion > 4 || Target.Version.MinorVersion > 25;
		if (bIOStoreSupport)
		{
			PublicDependencyModuleNames.AddRange(new string[]
			{
				"IoStoreUtilities"
			});
		}
		AddPublicDefinitions("WITH_IO_STORE_SUPPORT", bIOStoreSupport);
		AddPublicDefinitions("ENABLE_COOK_LOG", true);
		AddPublicDefinitions("ENABLE_COOK_ENGINE_MAP", false);
		AddPublicDefinitions("ENABLE_COOK_PLUGIN_MAP", false);
		BuildVersion Version;
		BuildVersion.TryRead(BuildVersion.GetDefaultFileName(), out Version);
		AddPublicDefinitions("WITH_EDITOR_SECTION", Version.MajorVersion > 4 || Version.MinorVersion > 24);
		System.Console.WriteLine("MajorVersion {0} MinorVersion: {1} PatchVersion {2}",Target.Version.MajorVersion,Target.Version.MinorVersion,Target.Version.PatchVersion);
		
		// !!! Please make sure to modify the engine if necessary, otherwise it will cause a crash
		AddPublicDefinitions("SUPPORT_NO_DDC", true);
		
		// Game feature
		bool bEnableGameFeature = true;
		if (bEnableGameFeature || (Target.Version.MajorVersion > 4 || Target.Version.MinorVersion > 26))
		{
			PublicDependencyModuleNames.AddRange(new string[]
			{
				// "GameFeatures",
				// "ModularGameplay",
			});
		}

		bool bEnablePackageContext = true;
		AddPublicDefinitions("WITH_PACKAGE_CONTEXT", (Version.MajorVersion > 4 || Version.MinorVersion > 23) && bEnablePackageContext);
		if (Version.MajorVersion > 4 || Version.MinorVersion > 26)
		{
			PublicDependencyModuleNames.AddRange(new string[]
			{
				"IoStoreUtilities",
				// "UnrealEd"
			});
		}
		
		bool bGenerateChunksManifest = false;
		AddPublicDefinitions("GENERATE_CHUNKS_MANIFEST", bGenerateChunksManifest);
		if (bGenerateChunksManifest)
		{
			PublicIncludePaths.AddRange(new string[]
			{
				Path.Combine(EngineDirectory,"Source/Editor/UnrealEd/Public"),
				Path.Combine(EngineDirectory,"Source/Editor/UnrealEd/Private"),
			});
		}
		
		if (Version.MajorVersion > 4)
		{
			PublicIncludePaths.AddRange(new List<string>()
			{
				// Path.Combine(EngineDirectory,"Source/Developer/IoStoreUtilities/Internal"),
				// Path.Combine(EngineDirectory,"Source/Editor/UnrealEd/Private/Cooker"),
				// Path.Combine(EngineDirectory,"Source/Editor/UnrealEd/Private"),
				Path.Combine(EngineDirectory,"Source/Runtime/CoreUObject/Internal/Serialization"),
			});
		}
		
		PublicDefinitions.AddRange(new string[]
		{
			"TOOL_NAME=\"HotPatcher\"",
			"CURRENT_VERSION_ID=81",
			"CURRENT_PATCH_ID=0",
			"REMOTE_VERSION_FILE=\"https://imzlp.com/opensource/version.json\""
		});
	}
}
