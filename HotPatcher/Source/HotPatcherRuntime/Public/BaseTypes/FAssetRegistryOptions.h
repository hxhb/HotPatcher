#pragma once
#include "BaseTypes.h"

// engine header
#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "Engine/EngineTypes.h"
#include "FAssetRegistryOptions.generated.h"

UENUM(BlueprintType)
enum class EAssetRegistryRule : uint8
{
	PATCH,
	PER_CHUNK,
	CUSTOM
};


USTRUCT(BlueprintType)
struct HOTPATCHERRUNTIME_API FAssetRegistryOptions
{
	GENERATED_BODY()
	FAssetRegistryOptions()
	{
		AssetRegistryMountPointRegular = FString::Printf(TEXT("%s/AssetRegistry"),AS_PROJECTDIR_MARK);
		AssetRegistryNameRegular = FString::Printf(TEXT("[CHUNK_NAME]_AssetRegistry.bin"));
	}
	FString GetAssetRegistryNameRegular(const FString& ChunkName)const
	{
		return AssetRegistryNameRegular.Replace(TEXT("[CHUNK_NAME]"),*ChunkName);
	}
	FString GetAssetRegistryMountPointRegular()const { return AssetRegistryMountPointRegular; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bSerializeAssetRegistry = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bSerializeAssetRegistryManifest = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString AssetRegistryMountPointRegular;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EAssetRegistryRule AssetRegistryRule = EAssetRegistryRule::PATCH;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bCustomAssetRegistryName = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite,meta=(EditCondition="bCustomAssetRegistryName"))
	FString AssetRegistryNameRegular;
};