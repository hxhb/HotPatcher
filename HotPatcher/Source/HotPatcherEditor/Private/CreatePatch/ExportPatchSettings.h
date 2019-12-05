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
	UExportPatchSettings(){}

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

	FORCEINLINE FString GetVersionId()const
	{
		return VersionId;
	}
	FORCEINLINE TArray<FString> GetAssetIncludeFilters()const
	{
		TArray<FString> Result;
		for (const auto& Filter : AssetIncludeFilters)
		{
			Result.AddUnique(Filter.Path);
		}
		return Result;
	}
	FORCEINLINE TArray<FString> GetAssetIgnoreFilters()const
	{
		TArray<FString> Result;
		for (const auto& Filter : AssetIgnoreFilters)
		{
			Result.AddUnique(Filter.Path);
		}
		return Result;
	}
	FORCEINLINE FString GetBaseVersion()const { return BaseVersion.FilePath; }
	FORCEINLINE FString GetSaveAbsPath()const {
		if (!SavePath.Path.IsEmpty())
		{
			return FPaths::ConvertRelativePathToFull(SavePath.Path);
		}
		return TEXT("");
	}

	FORCEINLINE TArray<FString> GetUnrealPakOptions()const { return UnrealPakOptions; }
	FORCEINLINE TArray<ETargetPlatform> GetPakTargetPlatforms()const { return PakTargetPlatforms; }
	FORCEINLINE bool IsSavePakList()const { return bSavePakList; }
	FORCEINLINE bool IsSaveDiffAnalysis()const { return IsByBaseVersion() && bSaveDiffAnalysis; }
	FORCEINLINE bool IsSavePatchConfig()const { return bSavePatchConfig; }
	FORCEINLINE bool IsIncludeAssetRegistry()const { return bIncludeAssetRegistry; }
	FORCEINLINE bool IsIncludeGlobalShaderCache()const { return bIncludeGlobalShaderCache; }
	FORCEINLINE bool IsIncludeProjectIni()const { return bIncludeProjectIni; }
	FORCEINLINE bool IsByBaseVersion()const { return bByBaseVersion; }

	FORCEINLINE	bool GetAllExternAssetCookCommands(const FString& InProjectDir, const FString& InPlatform, TArray<FString>& OutCookCommands)
	{
		OutCookCommands.Reset();
		TArray<FString> SearchAssetList;
		if (IsIncludeAssetRegistry())
		{
			FString AssetRegistryCookCommand;
			if (UFlibPatchParserHelper::GetCookedAssetRegistryFile(InProjectDir, UFlibPatchParserHelper::GetProjectName(), InPlatform, AssetRegistryCookCommand))
			{
				SearchAssetList.AddUnique(AssetRegistryCookCommand);
			}
		}

		if (IsIncludeGlobalShaderCache())
		{
			TArray<FString> GlobalShaderCacheList = UFlibPatchParserHelper::SearchCookedGlobalShaderCacheFiles(InProjectDir, InPlatform);
			if (!!GlobalShaderCacheList.Num())
			{
				SearchAssetList.Append(GlobalShaderCacheList);
			}
		}

		// combine as cook commands
		{
			for (const auto& AssetFile:SearchAssetList)
			{
				FString CurrentCommand;
				if (UFlibPatchParserHelper::ConvNotAssetFileToCookCommand(InProjectDir, InPlatform, AssetFile, CurrentCommand))
				{
					OutCookCommands.AddUnique(CurrentCommand);
				}
			}
		}

		if (IsIncludeProjectIni())
		{
			TArray<FString> IniFiles = UFlibPatchParserHelper::SearchProjectIniFiles(InProjectDir);
			TArray<FString> IniCookCommmands;
			UFlibPatchParserHelper::ConvProjectIniFilesToCookCommands(
				InProjectDir,
				UFlibPatchParserHelper::GetProjectName(),
				IniFiles,
				IniCookCommmands
			);
			if (!!IniCookCommmands.Num())
			{
				OutCookCommands.Append(IniCookCommmands);
			}
		}
		return true;
	}

	FORCEINLINE bool SerializePatchConfigToJsonObject(TSharedPtr<FJsonObject>& OutJsonObject)const
	{
		if (!OutJsonObject.IsValid())
		{
			OutJsonObject = MakeShareable(new FJsonObject);
		}
		OutJsonObject->SetStringField(TEXT("VersionId"), GetVersionId());
		OutJsonObject->SetBoolField(TEXT("bByBaseVersion"), IsByBaseVersion());
		OutJsonObject->SetStringField(TEXT("BaseVersion"), GetBaseVersion());
		
		auto SerializeArrayLambda = [&OutJsonObject](const TArray<FString>& InArray,const FString& InJsonArrayName)
		{
			TArray<TSharedPtr<FJsonValue>> ArrayJsonValueList;
			for (const auto& ArrayItem : InArray)
			{
				ArrayJsonValueList.Add(MakeShareable(new FJsonValueString(ArrayItem)));
			}
			OutJsonObject->SetArrayField(InJsonArrayName, ArrayJsonValueList);
		};
		auto ConvDirPathsToStrings = [](const TArray<FDirectoryPath>& InDirPaths)->TArray<FString>
		{
			TArray<FString> Resault;
			for (const auto& Dir : InDirPaths)
			{
				Resault.Add(Dir.Path);
			}
			return Resault;
		};

		SerializeArrayLambda(ConvDirPathsToStrings(AssetIncludeFilters), TEXT("AssetIncludeFilter"));
		SerializeArrayLambda(ConvDirPathsToStrings(AssetIgnoreFilters), TEXT("AssetIgnoreFilter"));

		OutJsonObject->SetBoolField(TEXT("bIncludeAssetRegistry"), IsIncludeAssetRegistry());
		OutJsonObject->SetBoolField(TEXT("bIncludeGlobalShaderCache"), IsIncludeGlobalShaderCache());
		OutJsonObject->SetBoolField(TEXT("bIncludeProjectIni"), IsIncludeProjectIni());

		// serialize all add extern file to pak
		{
			TSharedPtr<FJsonObject> AddExFilesJsonObject = MakeShareable(new FJsonObject);
			for (const auto& ExFileInfo : GetAddExternFiles())
			{
				TSharedPtr<FJsonObject> CurrentFileJsonObject;
				UFlibHotPatcherEditorHelper::SerializeExAssetFileInfoToJsonObject(ExFileInfo, CurrentFileJsonObject);
				AddExFilesJsonObject->SetObjectField(ExFileInfo.MountPath,CurrentFileJsonObject);
			}
			OutJsonObject->SetObjectField(TEXT("AddExFilesToPak"), AddExFilesJsonObject);
		}
	
		// serialize UnrealPakOptions
		SerializeArrayLambda(GetUnrealPakOptions(), TEXT("UnrealPakOptions"));

		// serialize platform list
		{
			TArray<FString> AllPlatforms;
			for (const auto &Platform : GetPakTargetPlatforms())
			{
				// save UnrealPak.exe command file
				FString PlatformName;
				{	
					FString EnumName;
					StaticEnum<ETargetPlatform>()->GetNameByValue((int64)Platform).ToString().Split(TEXT("::"), &EnumName, &PlatformName, ESearchCase::CaseSensitive, ESearchDir::FromEnd);
				}
				AllPlatforms.AddUnique(PlatformName);
			}
			SerializeArrayLambda(AllPlatforms, TEXT("TargetPlatforms"));
		}

		OutJsonObject->SetBoolField(TEXT("bSavePakList"), IsSavePakList());
		OutJsonObject->SetBoolField(TEXT("bSaveDiffAnalysis"), IsSaveDiffAnalysis());
		OutJsonObject->SetBoolField(TEXT("bSavePatchConfig"), IsSavePatchConfig());
		OutJsonObject->SetStringField(TEXT("SavePath"),GetSaveAbsPath());

		return true;
	}

	FORCEINLINE bool SerializePatchConfigToString(FString& OutSerializedStr)const
	{
		TSharedPtr<FJsonObject> PatchConfigJsonObject = MakeShareable(new FJsonObject);
		SerializePatchConfigToJsonObject(PatchConfigJsonObject);

		auto JsonWriter = TJsonWriterFactory<TCHAR>::Create(&OutSerializedStr);
		return FJsonSerializer::Serialize(PatchConfigJsonObject.ToSharedRef(), JsonWriter);
	}

	FORCEINLINE TArray<FExternAssetFileInfo> GetAddExternFiles()const { return AddExternFileToPak; }

	FORCEINLINE TArray<FString> CombineAddExternFileToCookCommands()
	{
		TArray<FString> resault;
		FString ProjectName = UFlibPatchParserHelper::GetProjectName();
		for (const auto& ExternFile : GetAddExternFiles())
		{
			FString FileAbsPath = FPaths::ConvertRelativePathToFull(ExternFile.FilePath.FilePath);
			if (FPaths::FileExists(FileAbsPath) &&
				ExternFile.MountPath.StartsWith(FPaths::Combine(TEXT("../../.."),ProjectName))
				)
			{
				resault.AddUnique(
					FString::Printf(TEXT("\"%s\" \"%s\""),*FileAbsPath,*ExternFile.MountPath)
				);
			}
		}
		return resault;
	}

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BaseVersion")
		bool bByBaseVersion = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "BaseVersion",meta = (RelativeToGameContentDir, EditCondition="bByBaseVersion"))
		FFilePath BaseVersion;
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "PatchSettings")
		FString VersionId;

	/** You can use copied asset string reference here, e.g. World'/Game/NewMap.NewMap'*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "PatchSettings|IncludeFilter",meta = (RelativeToGameContentDir, LongPackageName))
		TArray<FDirectoryPath> AssetIncludeFilters;
	/** You can use copied asset string reference here, e.g. World'/Game/NewMap.NewMap'*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PatchSettings|IgnoreFilter", meta = (RelativeToGameContentDir, LongPackageName))
		TArray<FDirectoryPath> AssetIgnoreFilters;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pak Options")
		bool bIncludeAssetRegistry;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pak Options")
		bool bIncludeGlobalShaderCache;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pak Options")
		bool bIncludeProjectIni;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pak Options")
		TArray<FExternAssetFileInfo> AddExternFileToPak;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pak Options")
		TArray<FString> UnrealPakOptions{TEXT("-compress")};
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pak Options")
		TArray<ETargetPlatform> PakTargetPlatforms;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SaveTo")
		bool bSavePakList = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SaveTo",meta=(EditCondition="bByBaseVersion"))
		bool bSaveDiffAnalysis = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SaveTo")
		bool bSavePatchConfig = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SaveTo")
		FDirectoryPath SavePath;

};