// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

// project header
#include "ETargetPlatform.h"
#include "FExternAssetFileInfo.h"
#include "FlibPatchParserHelper.h"
#include "FlibHotPatcherEditorHelper.h"

// engine header
#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "Engine/EngineTypes.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"
#include "ExportPatchSettings.generated.h"

/** Singleton wrapper to allow for using the setting structure in SSettingsView */
UCLASS()
class UExportPatchSettings : public UObject
{
	GENERATED_BODY()
public:

	UExportPatchSettings();

	FORCEINLINE static UExportPatchSettings* Get()
	{
		static bool bInitialized = false;
		// This is a singleton, use default object
		UExportPatchSettings* DefaultSettings = GetMutableDefault<UExportPatchSettings>();

		if (!bInitialized)
		{
			bInitialized = true;
		}

		return DefaultSettings;
	}
	
	TArray<FString> GetAssetIncludeFilters()const;
	TArray<FString> GetAssetIgnoreFilters()const;
	FString GetSaveAbsPath()const;
	TArray<FString> GetAllExternalCookCommands()const;
	bool GetAllExternAssetCookCommands(const FString& InProjectDir, const FString& InPlatform, TArray<FString>& OutCookCommands)const;
	bool SerializePatchConfigToJsonObject(TSharedPtr<FJsonObject>& OutJsonObject)const;
	bool SerializePatchConfigToString(FString& OutSerializedStr)const;
	TArray<FString> CombineAddExternFileToCookCommands()const;

	FORCEINLINE FString GetVersionId()const { return VersionId; }
	FORCEINLINE FString GetBaseVersion()const { return BaseVersion.FilePath; }
	FORCEINLINE TArray<FString> GetUnrealPakOptions()const { return UnrealPakOptions; }
	FORCEINLINE TArray<ETargetPlatform> GetPakTargetPlatforms()const { return PakTargetPlatforms; }
	TArray<FString> GetPakTargetPlatformNames()const;

	FORCEINLINE bool IsSavePakList()const { return bSavePakList; }
	FORCEINLINE bool IsSaveDiffAnalysis()const { return IsByBaseVersion() && bSaveDiffAnalysis; }
//	FORCEINLINE bool IsSavePakVersion()const { return bSavePakVersion; }
	FORCEINLINE bool IsSavePatchConfig()const { return bSavePatchConfig; }

	FORCEINLINE bool IsIncludeAssetRegistry()const { return bIncludeAssetRegistry; }
	FORCEINLINE bool IsIncludeGlobalShaderCache()const { return bIncludeGlobalShaderCache; }
	FORCEINLINE bool IsIncludeProjectIni()const { return bIncludeProjectIni; }
	FORCEINLINE bool IsByBaseVersion()const { return bByBaseVersion; }
	FORCEINLINE bool IsIncludeHasRefAssetsOnly()const { return bIncludeHasRefAssetsOnly; }
	FORCEINLINE bool IsIncludePakVersion()const { return bIncludePakVersionFile; }
	FORCEINLINE FString GetPakVersionFileMountPoint()const { return PakVersionFileMountPoint; }
	FORCEINLINE TArray<FExternAssetFileInfo> GetAddExternFiles()const { return AddExternFileToPak; }

	static FPakVersion GetPakVersion(const FHotPatcherVersion& InHotPatcherVersion,const FString& InUtcTime);
	static FString GetSavePakVersionPath(const FString& InSaveAbsPath,const FHotPatcherVersion& InVersion);
	static FString GetSavePakCommandsPath(const FString& InSaveAbsPath, const FString& InPlatfornName, const FHotPatcherVersion& InVersion);

	TArray<FString> CombineAllCookCommandsInTheSetting(const FString& InPlatformName, const FAssetDependenciesInfo& AllChangedAssetInfo)const;
	FHotPatcherVersion GetNewPatchVersionInfo()const;
	bool GetBaseVersionInfo(FHotPatcherVersion& OutBaseVersion)const;
	FString GetCurrentVersionSavePath()const;
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BaseVersion")
		bool bByBaseVersion = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "BaseVersion",meta = (RelativeToGameContentDir, EditCondition="bByBaseVersion"))
		FFilePath BaseVersion;
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "PatchSettings")
		FString VersionId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "PatchSettings",meta = (RelativeToGameContentDir, LongPackageName))
		TArray<FDirectoryPath> AssetIncludeFilters;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PatchSettings", meta = (RelativeToGameContentDir, LongPackageName))
		TArray<FDirectoryPath> AssetIgnoreFilters;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PatchSettings")
		bool bIncludeHasRefAssetsOnly;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PatchSettings")
		bool bIncludeAssetRegistry;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PatchSettings")
		bool bIncludeGlobalShaderCache;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PatchSettings")
		bool bIncludeProjectIni;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PatchSettings")
		TArray<FExternAssetFileInfo> AddExternFileToPak;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PatchSettings")
		bool bIncludePakVersionFile;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PatchSettings",meta=(EditCondition = "bIncludePakVersionFile"))
		FString PakVersionFileMountPoint;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pak Options")
		TArray<FString> UnrealPakOptions{TEXT("-compress")};
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pak Options")
		TArray<ETargetPlatform> PakTargetPlatforms;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SaveTo")
		bool bSavePakList = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SaveTo",meta=(EditCondition="bByBaseVersion"))
		bool bSaveDiffAnalysis = true;
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SaveTo", meta = (EditCondition = "bIncludePakVersion"))
	//	bool bSavePakVersion;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SaveTo")
		bool bSavePatchConfig = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SaveTo")
		FDirectoryPath SavePath;

};