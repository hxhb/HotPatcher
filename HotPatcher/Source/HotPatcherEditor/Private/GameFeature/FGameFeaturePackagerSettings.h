#pragma once 

#include "HotPatcherLog.h"
#include "CreatePatch/HotPatcherSettingBase.h"
#include "FlibPatchParserHelper.h"
#include "HotPatcherLog.h"
#include "CreatePatch/FExportPatchSettings.h"

// engine header
#include "Misc/FileHelper.h"
#include "CoreMinimal.h"
#include "ETargetPlatform.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "Engine/EngineTypes.h"
#include "Kismet/KismetStringLibrary.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

#include "FGameFeaturePackagerSettings.generated.h"

/** Singleton wrapper to allow for using the setting structure in SSettingsView */
USTRUCT(BlueprintType)
struct HOTPATCHEREDITOR_API FGameFeaturePackagerSettings:public FHotPatcherSettingBase
{
	GENERATED_USTRUCT_BODY()
public:
	FGameFeaturePackagerSettings()
	{
		SerializeAssetRegistryOptions.bSerializeAssetRegistry = true;
	}
	virtual ~FGameFeaturePackagerSettings(){};

	FORCEINLINE static FGameFeaturePackagerSettings* Get()
	{
		static FGameFeaturePackagerSettings StaticIns;

		return &StaticIns;
	}
	
	UPROPERTY(EditAnywhere)
	FString FeatureName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bAutoLoadFeaturePlugin = true;
	/*
	 * Cook Asset in current patch
	 * shader code gets saved inline inside material assets
	 * bShareMaterialShaderCode as false
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bCookPatchAssets = true;
	
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(EditCondition = "bCookPatchAssets"))
	FCookShaderOptions CookShaderOptions;
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(EditCondition = "bCookPatchAssets"))
	FAssetRegistryOptions SerializeAssetRegistryOptions;

	UPROPERTY(EditAnywhere)
	TArray<ETargetPlatform> TargetPlatforms;
};

