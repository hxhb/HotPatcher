#pragma once
#include "ETargetPlatform.h"
#include "FPlatformBasePak.h"
#include "BinariesPatchFeature.h"
#include "FlibPatchParserHelper.h"
// engine header
#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "HAL/FileManager.h"
#include "FBinariesPatchConfig.generated.h"

struct FPakCommandItem
{
	FString AssetAbsPath;
	FString AssetMountPath;
};
UENUM(BlueprintType)
enum class EMatchRule:uint8
{
	None,
	MATCH,
	IGNORE
};

UENUM(BlueprintType)
enum class EMatchOperator:uint8
{
	None,
	GREAT_THAN,
	LESS_THAN,
	EQUAL
};

USTRUCT(BlueprintType)
struct FMatchRule
{
	GENERATED_BODY()
	// match or ignore
	UPROPERTY(EditAnywhere)
	EMatchRule Rule = EMatchRule::None;

	// grate/less/equal
	UPROPERTY(EditAnywhere)
	EMatchOperator Operator = EMatchOperator::None;

	// uint kb
	UPROPERTY(EditAnywhere,meta=(EditCondition="Operator!=EMatchOperator::None"))
	float Size = 100;
	// match file Formats. etc .ini/.lua, if it is empty match everything
	UPROPERTY(EditAnywhere)
	TArray<FString> Formaters;
	UPROPERTY(EditAnywhere)
    TArray<FString> AssetTypes;
};

USTRUCT(BlueprintType)
struct HOTPATCHERRUNTIME_API FBinariesPatchConfig
{
	GENERATED_BODY();
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BinariesPatch")
	EBinariesPatchFeature BinariesPatchType = EBinariesPatchFeature::None;
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BinariesPatch")
	FDirectoryPath OldCookedDir;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BinariesPatch")
	FFilePath ExtractCryptoJson;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BinariesPatch")
	TArray<FPlatformBasePak> BaseVersionPaks;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BinariesPatch")
	TArray<FMatchRule> MatchRules;
	// etc .ini/.lua
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BinariesPatch")
	// TArray<FString> IgnoreFileRules;

	bool IsMatchIgnoreRules(const FPakCommandItem& File)
	{
		bool bIsIgnore = false;
		if(!FPaths::FileExists(File.AssetAbsPath))
			return false;
		uint64 FileSize = IFileManager::Get().FileSize(*File.AssetAbsPath);

		auto FormatMatcher = [](const FString& File,const TArray<FString>& Formaters)->bool
		{
			bool bFormatMatch = !Formaters.Num();
			for(const auto& FileFormat:Formaters)
			{
				if(!FileFormat.IsEmpty() && File.EndsWith(FileFormat,ESearchCase::CaseSensitive))
				{
					bFormatMatch = true;
					break;
				}
			}
			return bFormatMatch;
		};
		
		auto SizeMatcher = [](uint64 FileSize,const FMatchRule& MatchRule)->bool
		{
			bool bMatch = false;
			uint64 RuleSize = MatchRule.Size * 1024; // kb to byte
			switch (MatchRule.Operator)
			{
				case EMatchOperator::GREAT_THAN:
					{
						bMatch = FileSize > RuleSize;
						break;
					}
				case EMatchOperator::LESS_THAN:
					{
						bMatch = FileSize < RuleSize;
						break;
					}
				case EMatchOperator::EQUAL:
					{
						bMatch = FileSize == RuleSize;
						break;
					}
				case EMatchOperator::None:
					{
						bMatch = true;
						break;
					}
			}
			return bMatch;
		};
		
		for(const auto& Rule:GetMatchRules())
		{
			bool bSizeMatch = SizeMatcher(FileSize,Rule);
			bool bFormatMatch = FormatMatcher(File.AssetAbsPath,Rule.Formaters);
			bool bMatched = bSizeMatch && bFormatMatch;
			bIsIgnore = Rule.Rule == EMatchRule::MATCH ? !bMatched : bMatched;
		}
		return bIsIgnore;
	}
	// FORCEINLINE TArray<FString> GetBinariesPatchIgnoreFileRules()const {return IgnoreFileRules;}
	FORCEINLINE TArray<FMatchRule> GetMatchRules()const{ return MatchRules; }
	FORCEINLINE TArray<FPlatformBasePak> GetBaseVersionPaks()const {return BaseVersionPaks;};
	FORCEINLINE FString GetBinariesPatchFeatureName() const { return UFlibPatchParserHelper::GetEnumNameByValue(BinariesPatchType); }
	FORCEINLINE FString GetOldCookedDir() const { return UFlibPatchParserHelper::ReplaceMarkPath(OldCookedDir.Path); }
	FORCEINLINE FString GetBasePakExtractCryptoJson() const { return UFlibPatchParserHelper::ReplaceMarkPath(ExtractCryptoJson.FilePath); }
	
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