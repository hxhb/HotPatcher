// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

using System;
using System.IO;
using UnrealBuildTool;

public class HotPatcherRuntime : ModuleRules
{
	public HotPatcherRuntime(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				Path.Combine(EngineDirectory,"Source/Runtime/Launch"),
				Path.Combine(ModuleDirectory,"Public"),
				Path.Combine(ModuleDirectory,"Public/BaseTypes"),
				Path.Combine(ModuleDirectory,"Public/Templates")
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);

		if (Target.bBuildEditor)
		{
			PublicDependencyModuleNames.AddRange(new string[]
			{
				"TargetPlatform"
			});
		}
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"RHI",
				"Core",
                "Projects",
                "Json",
                "JsonUtilities",
                "PakFile",
                "AssetRegistry",
                "BinariesPatchFeature"
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
				"HTTP",
				"Sockets"
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
		
		BuildVersion Version;
		BuildVersion.TryRead(BuildVersion.GetDefaultFileName(), out Version);
		// PackageContext
		System.Func<string, bool,bool> AddPublicDefinitions = (string MacroName,bool bEnable) =>
		{
			PublicDefinitions.Add(string.Format("{0}={1}",MacroName, bEnable ? 1 : 0));
			return true;
		};

		AddPublicDefinitions("WITH_EDITOR_SECTION", Version.MajorVersion > 4 || Version.MinorVersion > 24);
		bool bForceSingleThread = (Version.MajorVersion == 4 && Version.MinorVersion < 25) ||
		                          (Version.MajorVersion == 5 && Version.MinorVersion >= 1);
		AddPublicDefinitions("FORCE_SINGLE_THREAD",bForceSingleThread);
		
		bool bEnableAssetDependenciesDebugLog = true;
		AddPublicDefinitions("ASSET_DEPENDENCIES_DEBUG_LOG", bEnableAssetDependenciesDebugLog);
		
		bool bCustomAssetGUID = false;

		if (Version.MajorVersion > 4) { bCustomAssetGUID = true; }
		if(bCustomAssetGUID)
		{
			PublicDefinitions.Add("CUSTOM_ASSET_GUID");	
		}
		AddPublicDefinitions("WITH_UE5", Version.MajorVersion > 4);
		
		AddPublicDefinitions("AUTOLOAD_SHADERLIB_AT_RUNTIME", true);
		bLegacyPublicIncludePaths = false;
		OptimizeCode = CodeOptimization.InShippingBuildsOnly;
	}
}
