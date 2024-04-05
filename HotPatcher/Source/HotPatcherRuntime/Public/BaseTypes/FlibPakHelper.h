// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include "FPakVersion.h"
#include "AssetRegistry.h"
// Engine Header
#include "Resources/Version.h"
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Templates/SharedPointer.h"
#include "Dom/JsonObject.h"
#include "IPlatformFilePak.h"
#include "FlibPakHelper.generated.h"

#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >=26
	#define FindFilesAtPath FindPrunedFilesAtPath
	#define GetFilenames GetPrunedFilenames
#endif

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

	// secrah specify extension file type file in directory
	UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|Pak")
		static bool ScanExtenFilesInDirectory(const FString& InRelativePath,const FString& InExtenPostfix,bool InRecursively, TArray<FString>& OutFiles);

	// search in FPaths::ProjectSavedDir()/TEXT("Extension/Versions")
	UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|Pak")
		static TArray<FString> ScanAllVersionDescribleFiles();

	// Additional Pak files in ../../../PROJECT_NAME/Saved/ExtenPaks
	UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|Pak")
		static TArray<FString> ScanExtenPakFiles();
	
	UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|Pak")
		static TArray<FString> GetAllMountedPaks();

	UFUNCTION(BlueprintCallable)
		static int32 GetPakOrderByPakPath(const FString& PakFile);


	// Default Load FApp::GetProjectName() on Enging launching
	UFUNCTION(BlueprintCallable)
		static bool OpenPSO(const FString& Name);

	static TArray<FString> GetPakFileList(const FString& InPak, const FString& AESKey);
	static TMap<FString,FPakEntry> GetPakEntrys(FPakFile* InPakFile);
	static FSHA1 GetPakEntryHASH(FPakFile* InPakFile,const FPakEntry& PakEntry);
	
	static FString GetPakFileMountPoint(const FString& InPak, const FString& AESKey);
	static FPakFile* GetPakFileIns(const FString& InPak, const FString& AESKey);

public:
	// reload Global&Project shaderbytecode
	UFUNCTION(BlueprintCallable,Exec)
		static void ReloadShaderbytecode();
	UFUNCTION(BlueprintCallable,Exec)
		static bool LoadShaderbytecode(const FString& LibraryName, const FString& LibraryDir,bool bNative = false);
	UFUNCTION(BlueprintCallable,Exec)
		static bool LoadShaderbytecodeInDefaultDir(const FString& LibraryName);	
	UFUNCTION(BlueprintCallable,Exec)
		static void CloseShaderbytecode(const FString& LibraryName);
	UFUNCTION(BlueprintCallable,Exec)
		static void LoadShaderLibrary(const FString& ScanShaderLibs);
	UFUNCTION(BlueprintCallable,Exec)
		static void LoadHotPatcherAllShaderLibrarys();
		
	static bool LoadAssetRegistryToState(const TCHAR* Path,FAssetRegistryState& Out);
	UFUNCTION(BlueprintCallable,Exec)
		static bool LoadAssetRegistry(const FString& LibraryName, const FString& LibraryDir);

private:
	static TSet<FName> LoadShaderLibraryNames;
};
