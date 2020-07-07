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
	FString EngineBin;
	FString ProjectPath;
	FString EngineParams;
	TArray<FString> CookPlatforms;
	bool bCookAllMap;
	TArray<FString> CookMaps;
	TArray<FString> CookFilter;
	TArray<FString> CookSettings;
	FString Options;
};