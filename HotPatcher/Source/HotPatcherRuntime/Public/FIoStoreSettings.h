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
	// global.utoc file
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FFilePath GlobalContainers;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bGenerateDiffPatch;
	// -PatchSource=E:\UnrealProjects\StarterContent\Releases\1.0\WindowsNoEditor\StarterContent-WindowsNoEditor*.utoc -GenerateDiffPatch
	UPROPERTY(EditAnywhere,BlueprintReadWrite,meta=(EditCondition="bGenerateDiffPatch"))
	FFilePath PatchSource;
};

USTRUCT(BlueprintType)
struct FIoStoreSettings
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
		bool bIoStore = false;
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
		bool bAllowBulkDataInIoStore = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TArray<FString> IoStorePakListOptions;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TArray<FString> IoStoreCommandletOptions;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TMap<ETargetPlatform,FIoStorePlatformContainers> PlatformContainers;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		bool bStoragePakList = true;
};