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
struct GAMEFEATUREPACKEREDITOR_API FGameFeaturePackagerSettings:public FHotPatcherSettingBase
{
	GENERATED_USTRUCT_BODY()
public:
	FGameFeaturePackagerSettings()
	{
		SerializeAssetRegistryOptions.bSerializeAssetRegistry = true;
		NonContentDirs.AddUnique(TEXT("Config"));
	}
	virtual ~FGameFeaturePackagerSettings(){};

	FORCEINLINE static FGameFeaturePackagerSettings* Get()
	{
		static FGameFeaturePackagerSettings StaticIns;

		return &StaticIns;
	}
	
	UPROPERTY(EditAnywhere)
	TArray<FString> FeatureNames;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bAutoLoadFeaturePlugin = true;
	// Config/ Script/ etc.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FString> NonContentDirs;
	/*
	 * Cook Asset in current patch
	 * shader code gets saved inline inside material assets
	 * bShareMaterialShaderCode as false
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Base")
	bool bCookPatchAssets = true;
	
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Base", meta=(EditCondition = "bCookPatchAssets"))
	FCookShaderOptions CookShaderOptions;
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Base", meta=(EditCondition = "bCookPatchAssets"))
	FAssetRegistryOptions SerializeAssetRegistryOptions;

	// support UE4.26 later
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Base")
	FIoStoreSettings IoStoreSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base")
	FPakEncryptSettings EncryptSettings;
	
	UPROPERTY(EditAnywhere, Category="Base")
	TArray<ETargetPlatform> TargetPlatforms;
};

