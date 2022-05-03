#pragma once

#include "CoreMinimal.h"
#include "FPakVersion.generated.h"

USTRUCT(BlueprintType)
struct FPakVersion
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category="")
		FString VersionId;
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category="")
		FString BaseVersionId;
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category="")
		FString Date;
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category="")
		FString CheckCode;
};
