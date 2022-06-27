#pragma once

#include "CoreMinimal.h"
#include "FUnrealPakSettings.generated.h"

USTRUCT(BlueprintType)
struct FUnrealPakSettings
{
	GENERATED_USTRUCT_BODY()
public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="")
		TArray<FString> UnrealPakListOptions;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="")
		TArray<FString> UnrealCommandletOptions;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="")
		bool bStoragePakList = true;
};
