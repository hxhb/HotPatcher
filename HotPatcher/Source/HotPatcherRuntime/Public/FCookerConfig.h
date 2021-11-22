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
	UPROPERTY(EditAnywhere)
	FString EngineBin;
	UPROPERTY(EditAnywhere)
	FString ProjectPath;
	UPROPERTY(EditAnywhere)
	FString EngineParams;
	UPROPERTY(EditAnywhere)
	TArray<FString> CookPlatforms;
	UPROPERTY(EditAnywhere)
	bool bCookAllMap = false;
	UPROPERTY(EditAnywhere)
	TArray<FString> CookMaps;
	UPROPERTY(EditAnywhere)
	TArray<FString> CookFilter;
	UPROPERTY(EditAnywhere)
	TArray<FString> CookSettings;
	UPROPERTY(EditAnywhere)
	FString Options;
};