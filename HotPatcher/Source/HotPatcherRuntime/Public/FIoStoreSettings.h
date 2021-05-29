#pragma once

#include "CoreMinimal.h"

#include "ETargetPlatform.h"

#include "FIoStoreSettings.generated.h"

USTRUCT(BlueprintType)
struct FIoStorePlatformContainers
{
	GENERATED_USTRUCT_BODY()
	// global.utoc file
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FFilePath GlobalContainers;
};

USTRUCT(BlueprintType)
struct FIoStoreSettings
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
		bool bIoStore;
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
		bool bAllowBulkDataInIoStore;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TArray<FString> IoStorePakListOptions;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TArray<FString> IoStoreCommandletOptions;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TMap<ETargetPlatform,FIoStorePlatformContainers> PlatformContainers;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		bool bStoragePakList;
};