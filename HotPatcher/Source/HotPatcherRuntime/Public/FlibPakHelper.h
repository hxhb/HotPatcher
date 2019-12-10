// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include "FPakVersion.h"

// Engine Header
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Templates/SharedPointer.h"
#include "Dom/JsonObject.h"
#include "IPlatformFilePak.h"
#include "FlibPakHelper.generated.h"


UCLASS()
class HOTPATCHERRUNTIME_API UFlibPakHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(Exec)
		static void ExecMountPak(FString InPakPath, int32 InPakOrder=0, FString InMountPoint=TEXT(""));

	UFUNCTION(BlueprintCallable, Category="GWorld|Flib|Pak", meta=(AdvancedDisplay="InMountPoint"))
		static bool MountPak(const FString& PakPath, int32 PakOrder, const FString& InMountPoint = TEXT(""));
	UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|Pak", meta = (AdvancedDisplay = "InMountPoint"))
		static bool UnMountPak(const FString& PakPath);

	UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|Pak", meta=(AdvancedDisplay="InWriteFlag"))
		static bool CreateFileByBytes(const FString& InFile, const TArray<uint8>& InBytes, int32 InWriteFlag = 0);

	UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|Pak")
		static bool ScanPlatformDirectory(const FString& InRelativePath,bool bIncludeFile,bool bIncludeDir,bool bRecursively, TArray<FString>& OutResault);

	UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|Pak")
		static bool SerializePakVersionToString(const FPakVersion& InPakVersion, FString& OutString);
	static bool SerializePakVersionToJsonObject(const FPakVersion& InPakVersion, TSharedPtr<FJsonObject>& OutJsonObject);
	UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|Pak")
		static bool DeserializeStringToPakVersion(const FString& InString, FPakVersion& OutPakVersion);
	static bool DeserializeJsonObjectToPakVersion(const TSharedPtr<FJsonObject>& InJsonObject, FPakVersion& OutPakVersion);

	// secrah specify extension file type file in directory
	UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|Pak")
		static bool ScanExtenFilesInDirectory(const FString& InRelativePath,const FString& InExtenPostfix,bool InRecursively, TArray<FString>& OutFiles);

	// search in FPaths::ProjectSavedDir()/TEXT("Extension/Versions")
	UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|Pak")
		static TArray<FString> ScanAllVersionDescribleFiles();

	// Additional Pak files in ../../../PROJECT_NAME/Saved/ExtenPaks
	UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|Pak")
		static TArray<FString> ScanExtenPakFiles();
		
	static bool LoadPakDoSomething(const FString& InPakFile, TFunction<bool(const FPakFile*)> InDoSomething);
	UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|Pak")
		static bool LoadFilesByPak(const FString& InPakFile, TArray<FString>& OutFiles);

	UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|Pak")
		static bool LoadVersionInfoByPak(const FString& InPakFile, FPakVersion& OutVersion);

	UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|Pak")
		static TArray<FString> GetAllMountedPaks();
};
