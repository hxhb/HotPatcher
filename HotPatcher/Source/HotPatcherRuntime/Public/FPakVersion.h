#pragma once

#include "CoreMinimal.h"
#include "FPakVersion.generated.h"

USTRUCT(BlueprintType)
struct FPakVersion
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
		FString VersionId;
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
		FString BaseVersionId;
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
		FString Date;
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
		FString CheckCode;
};