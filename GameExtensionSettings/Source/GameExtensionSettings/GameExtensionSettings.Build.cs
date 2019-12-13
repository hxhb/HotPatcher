// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System.IO;

public class GameExtensionSettings : ModuleRules
{
    private string ModulePath
    {
        get { return ModuleDirectory; }
    }

    private string ThridPartyPath
    {
        get { return Path.Combine(ModulePath, "ThirdParty/"); }
    }

	public GameExtensionSettings(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "Slate",
            "SlateCore",
            "Settings"
        });

		PrivateDependencyModuleNames.AddRange(new string[] {});

        // // Thrid Party Library
        // PublicSystemIncludePaths.AddRange(
        //     new string[]
        //     {
        //            Path.Combine(ThridPartyPath,"mpackLib")
        //     }
        // );

        // add thrid party static linkage library
        PublicLibraryPaths.AddRange(
            new string[]
            {
                //Path.Combine(ThridPartyPath,"XXXX.lib");
            }
        );

        DynamicallyLoadedModuleNames.AddRange(
                new string[]
                {
                    "AssetRegistry"
                }
        );
        /*  
         *  Add Runtime Dependencies,and stoge to package folder
         *  Notice:Pleace check platform info (x86/x64) and Configuration info (Debug/Release)
         */
        //RuntimeDependencies.Add(
        //    new RuntimeDependency(
        //        Path.Combine(ThridPartyPath, "XXXX.dll")
        //    )
        //);

        // Uncomment if you are using Slate UI
        // PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");

        // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true

        OptimizeCode = CodeOptimization.InShippingBuildsOnly;
    }
}
