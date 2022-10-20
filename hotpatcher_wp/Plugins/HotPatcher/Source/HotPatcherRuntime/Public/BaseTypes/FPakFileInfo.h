#pragma once

// project header
#include "FPakVersion.h"
// engine header
#include "CoreMinimal.h"
#include "FPakFileInfo.generated.h"

USTRUCT(BlueprintType)
struct FPakFileInfo
{
	GENERATED_USTRUCT_BODY()
public:

	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	FString FileName;
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	FString Hash;
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	int32 FileSize = 0;
	//UPROPERTY(EditAnywhere,BlueprintReadWrite)
	//FPakVersion PakVersion;
};

USTRUCT(BlueprintType)
struct FPakFileArray
{
	GENERATED_USTRUCT_BODY()
public:
	
	UPROPERTY()
	TArray<FPakFileInfo> PakFileInfos;
};

USTRUCT(BlueprintType)
struct FPakFilesMap
{
	GENERATED_USTRUCT_BODY()
public:
	
	UPROPERTY()
	TMap<FString,FPakFileArray> PakFilesMap;
};