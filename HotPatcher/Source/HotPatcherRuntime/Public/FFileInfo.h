#pragma once

#include "CoreMinimal.h"
#include "FFileInfo.generated.h"

USTRUCT(BlueprintType)
struct FFileInfo
{
	GENERATED_USTRUCT_BODY()
public:

	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	FString FileName;
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	FString Hash;
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	int32 FileSize;
};