// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

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
				"RHI",
				"Core",
                "Projects",
                "Json",
                "JsonUtilities",
                "PakFile",
                "AssetRegistry",
                "AssetManagerEx"
				// ... add other public dependencies that you statically link with here ...
			}
			);

		// PackageContext
		{
			bool bEnablePackageContext = false;
			BuildVersion Version;
			if (BuildVersion.TryRead(BuildVersion.GetDefaultFileName(), out Version))
			{
				if (Version.MajorVersion > 4 || Version.MinorVersion > 24)
				{
					PublicDefinitions.Add("WITH_PACKAGE_CONTEXT=1");
					bEnablePackageContext = true;
				}
			}
		
			if(!bEnablePackageContext)
			{
				PublicDefinitions.Add("WITH_PACKAGE_CONTEXT=0");
			}
		}
		
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
