#pragma once
// project
#include "ETargetPlatform.h"

// engine header
#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "FIoStoreSettings.generated.h"

// -Output=E:\UnrealProjects\StarterContent\Package\DLC2\WindowsNoEditor\StarterContent\Content\Paks\StarterContent-WindowsNoEditor_0_P.utoc
// -ContainerName=StarterContent
// -PatchSource=E:\UnrealProjects\StarterContent\Releases\1.0\WindowsNoEditor\StarterContent-WindowsNoEditor*.utoc
// -GenerateDiffPatch
// -ResponseFile="C:\Users\visionsmile\AppData\Roaming\Unreal Engine\AutomationTool\Logs\E+UnrealEngine+Launcher+UE_4.26\PakListIoStore_StarterContent.txt"

USTRUCT(BlueprintType)
struct FIoStorePlatformContainers
{
	GENERATED_USTRUCT_BODY()
	// Saved/StagedBuilds/Windows
	// Saved/StagedBuilds/Android_ASTC
	// │  Manifest_NonUFSFiles_Win64.txt
	// │  ThirdPerson_UE5.exe
	// ├─Engine
	// └─ThirdPerson_UE5
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="")
	FDirectoryPath BasePackageStagedRootDir;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="")
	bool bGenerateDiffPatch = false;
	
	// global.utoc file
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="")
	FFilePath GlobalContainersOverride;
	// -PatchSource=E:\UnrealProjects\StarterContent\Releases\1.0\WindowsNoEditor\StarterContent-WindowsNoEditor*.utoc -GenerateDiffPatch
	UPROPERTY(EditAnywhere,BlueprintReadWrite,meta=(EditCondition="bGenerateDiffPatch"), Category="")
	FFilePath PatchSourceOverride;
};

USTRUCT(BlueprintType)
struct FIoStoreSettings
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category="")
		bool bIoStore = false;
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category="")
		bool bAllowBulkDataInIoStore = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="")
		TArray<FString> IoStorePakListOptions;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="")
		TArray<FString> IoStoreCommandletOptions;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="")
		TMap<ETargetPlatform,FIoStorePlatformContainers> PlatformContainers;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="")
		bool bStoragePakList = true;
	// Metadata/BulkDataInfo.ubulkmanifest
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="")
		bool bStorageBulkDataInfo = true;
};
