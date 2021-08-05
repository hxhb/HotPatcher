#pragma once
#include "ETargetPlatform.h"
#include "FPlatformBasePak.h"
#include "BinariesPatchFeature.h"
#include "FlibPatchParserHelper.h"
// engine header
#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "FBinariesPatchConfig.generated.h"

struct FPakCommandItem
{
	FString AssetAbsPath;
	FString AssetMountPath;
};

USTRUCT(BlueprintType)
struct HOTPATCHERRUNTIME_API FBinariesPatchConfig
{
	GENERATED_BODY();
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BinariesPatch")
	EBinariesPatchFeature BinariesPatchType;
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BinariesPatch")
	FDirectoryPath OldCookedDir;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BinariesPatch")
	FFilePath ExtractKey;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BinariesPatch")
	TArray<FPlatformBasePak> BaseVersionPaks;
	// etc .ini/.lua
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BinariesPatch")
	TArray<FString> IgnoreFileRules;

	bool IsMatchIgnoreRules(const FPakCommandItem& File)
	{
		bool bIsIgnore = false;
		for(const auto& IgnoreRule:GetBinariesPatchIgnoreFileRules())
		{
			if(File.AssetAbsPath.EndsWith(IgnoreRule,ESearchCase::CaseSensitive))
			{
				bIsIgnore = true;
				break;
			}
		}				
		return bIsIgnore;
	}
	FORCEINLINE TArray<FString> GetBinariesPatchIgnoreFileRules()const {return IgnoreFileRules;}
	FORCEINLINE TArray<FPlatformBasePak> GetBaseVersionPaks()const {return BaseVersionPaks;};
	FORCEINLINE FString GetBinariesPatchFeatureName() const { return UFlibPatchParserHelper::GetEnumNameByValue(BinariesPatchType); }
	FORCEINLINE FString GetOldCookedDir() const { return UFlibPatchParserHelper::ReplaceMarkPath(OldCookedDir.Path); }
	FORCEINLINE FString GetBasePakExtractKey() const { return UFlibPatchParserHelper::ReplaceMarkPath(ExtractKey.FilePath); }
	
	FORCEINLINE TArray<FString> GetBaseVersionPakByPlatform(ETargetPlatform Platform)
	{
		TArray<FString> result;
		for(const auto& BaseVersionPak:GetBaseVersionPaks())
		{
			if(BaseVersionPak.Platform == Platform)
			{
				for(const auto& Path:BaseVersionPak.Paks)
				{
					result.Emplace(UFlibPatchParserHelper::ReplaceMarkPath(Path.FilePath));
				}
			}
		}
		return result;
	}
	
};