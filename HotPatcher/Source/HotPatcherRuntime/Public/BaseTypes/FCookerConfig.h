#pragma once 
// engine header
#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "FCookerConfig.generated.h"

USTRUCT()
struct FCookerConfig
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY(EditAnywhere, Category="")
	FString EngineBin;
	UPROPERTY(EditAnywhere, Category="")
	FString ProjectPath;
	UPROPERTY(EditAnywhere, Category="")
	FString EngineParams;
	UPROPERTY(EditAnywhere, Category="")
	TArray<FString> CookPlatforms;
	UPROPERTY(EditAnywhere, Category="")
	bool bCookAllMap = false;
	UPROPERTY(EditAnywhere, Category="")
	TArray<FString> CookMaps;
	UPROPERTY(EditAnywhere, Category="")
	TArray<FString> CookFilter;
	UPROPERTY(EditAnywhere, Category="")
	TArray<FString> CookSettings;
	UPROPERTY(EditAnywhere, Category="")
	FString Options;
};
