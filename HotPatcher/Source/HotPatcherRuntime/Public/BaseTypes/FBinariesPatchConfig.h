#pragma once
#include "ETargetPlatform.h"
#include "FPlatformBasePak.h"
#include "BinariesPatchFeature.h"
#include "FPakEncryptionKeys.h"

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
public:
	FORCEINLINE FPakEncryptSettings GetEncryptSettings()const{ return EncryptSettings; }
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BinariesPatch")
	EBinariesPatchFeature BinariesPatchType = EBinariesPatchFeature::None;
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BinariesPatch")
	FDirectoryPath OldCookedDir;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BinariesPatch")
	FPakEncryptSettings EncryptSettings;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BinariesPatch")
	TArray<FPlatformBasePak> BaseVersionPaks;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BinariesPatch")
	TArray<FMatchRule> MatchRules;
	// etc .ini/.lua
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BinariesPatch")
	// TArray<FString> IgnoreFileRules;
	bool IsMatchIgnoreRules(const FPakCommandItem& File);

	// FORCEINLINE TArray<FString> GetBinariesPatchIgnoreFileRules()const {return IgnoreFileRules;}
	FORCEINLINE TArray<FMatchRule> GetMatchRules()const{ return MatchRules; }
	FORCEINLINE TArray<FPlatformBasePak> GetBaseVersionPaks()const {return BaseVersionPaks;};
	FString GetBinariesPatchFeatureName() const;
	FString GetOldCookedDir() const;
	FString GetBasePakExtractCryptoJson() const;
	TArray<FString> GetBaseVersionPakByPlatform(ETargetPlatform Platform);
	
};
