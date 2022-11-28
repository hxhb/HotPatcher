#pragma once
// engine header
#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "Engine/EngineTypes.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"
#include "SpecifyCookFilterSetting.generated.h"

/** Singleton wrapper to allow for using the setting structure in SSettingsView */
UCLASS()
class USpecifyCookFilterSetting : public UObject
{
	GENERATED_BODY()
public:

	USpecifyCookFilterSetting() {}

	FORCEINLINE static USpecifyCookFilterSetting* Get()
	{
		static bool bInitialized = false;
		// This is a singleton, use default object
		USpecifyCookFilterSetting* DefaultSettings = GetMutableDefault<USpecifyCookFilterSetting>();

		if (!bInitialized)
		{
			bInitialized = true;
		}

		return DefaultSettings;
	}

	FORCEINLINE TArray<FDirectoryPath>& GetAlwayCookFilters(){return AlwayCookFilters;}
	FORCEINLINE TArray<FSoftObjectPath>& GetCookAssets(){return CookAssets;}

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "Directory",meta = (RelativeToGameContentDir, LongPackageName))
		TArray<FDirectoryPath> AlwayCookFilters;
	// UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "Assets")
		TArray<FSoftObjectPath> CookAssets;
	//UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "CookDirectoryFilter",meta = (RelativeToGameContentDir, LongPackageName))
	//	TArray<FDirectoryPath> NeverCookFilters;

};