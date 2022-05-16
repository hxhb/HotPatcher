#pragma once

// Engine Header
#include "Resources/Version.h"
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Templates/SharedPointer.h"
#include "Dom/JsonObject.h"
#include "IPlatformFilePak.h"
#include "AssetRegistryState.h"
#include "FlibRemakePakHelper.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogRemakePak,All,All);

USTRUCT(BlueprintType)
struct FDumpPakEntry
{
	GENERATED_USTRUCT_BODY()
	
	UPROPERTY()
	int64 Offset;
	UPROPERTY()
	int64 PakEntrySize;
	UPROPERTY()
	int64 ContentSize;
};

USTRUCT(BlueprintType)
struct FDumpPakAsset
{
	GENERATED_USTRUCT_BODY()
	
	UPROPERTY()
	FString PackageName;
	UPROPERTY()
	FName GUID;
	
	UPROPERTY()
	TMap<FString,FDumpPakEntry> AssetEntrys;
};


USTRUCT(BlueprintType)
struct FPakDumper
{
	GENERATED_USTRUCT_BODY()
	UPROPERTY()
	FString PakName;
	UPROPERTY()
	FString MountPoint;

	UPROPERTY()
	TMap<FString,FDumpPakAsset> PakEntrys;
};


UCLASS()
class REMAKEPAK_API UFlibRemakePakHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	static FSHA1 GetPakEntryHASH(FPakFile* InPakFile,const FPakEntry& PakEntry);
	UFUNCTION(BlueprintCallable)
	static void DumpPakEntrys(const FString& InPak, const FString& AESKey,const FString& SaveTo);
	UFUNCTION(BlueprintCallable)
	static void TestRemakePak(const FString& OldPak,const FString& NewPak);

};