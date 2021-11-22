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
				Path.Combine(ModuleDirectory,"Public")
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
			PublicDependencyModuleNames.Add("TargetPlatform");
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
                "AssetManagerEx",
                "BinariesPatchFeature"
				// ... add other public dependencies that you statically link with here ...
			}
			);

		BuildVersion Version;
		BuildVersion.TryRead(BuildVersion.GetDefaultFileName(), out Version);
		// PackageContext
		System.Func<string, bool,bool> AddPublicDefinitions = (string MacroName,bool bEnable) =>
		{
			PublicDefinitions.Add(string.Format("{0}={1}",MacroName, bEnable ? 1 : 0));
			return true;
		};

		AddPublicDefinitions("WITH_EDITOR_SECTION", Version.MajorVersion > 4 || Version.MinorVersion > 24);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore"
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
		bLegacyPublicIncludePaths = false;

		OptimizeCode = CodeOptimization.InShippingBuildsOnly;
	}
}
