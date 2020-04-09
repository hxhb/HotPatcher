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
	int32 FileSize;
	//UPROPERTY(EditAnywhere,BlueprintReadWrite)
	//FPakVersion PakVersion;
};