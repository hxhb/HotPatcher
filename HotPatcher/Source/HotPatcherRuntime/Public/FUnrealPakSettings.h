#pragma once

#include "CoreMinimal.h"
#include "FUnrealPakSettings.generated.h"

USTRUCT(BlueprintType)
struct FUnrealPakSettings
{
	GENERATED_USTRUCT_BODY()
public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TArray<FString> UnrealPakListOptions;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TArray<FString> UnrealCommandletOptions;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		bool bStoragePakList = true;
};