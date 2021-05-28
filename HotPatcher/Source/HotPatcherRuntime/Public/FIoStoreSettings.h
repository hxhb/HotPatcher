#pragma once

#include "CoreMinimal.h"
#include "FIoStoreSettings.generated.h"

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
		bool bSavePakList;
};