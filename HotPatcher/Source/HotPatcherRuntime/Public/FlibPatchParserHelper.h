// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
//project header
#include "FChunkInfo.h"
#include "FPakFileInfo.h"
#include "FReplaceText.h"
#include "AssetManager/FAssetDependenciesInfo.h"
#include "FHotPatcherVersion.h"
#include "FPatchVersionDiff.h"
#include "FlibPakHelper.h"
#include "FExternDirectoryInfo.h"
#include "FExternDirectoryInfo.h"
#include "FHotPatcherAssetDependency.h"
#include "FCookerConfig.h"
#include "FPlatformExternFiles.h"
// cpp standard
#include <typeinfo>
#include <cctype>
#include <algorithm>
#include <string>
// engine header
#include "Resources/Version.h"
#include "JsonObjectConverter.h"
#include "CoreMinimal.h"
#include "FPlatformExternAssets.h"
#include "Containers/UnrealString.h"
#include "Templates/SharedPointer.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FlibPatchParserHelper.generated.h"

/**
 * 
 */
UCLASS()
class HOTPATCHERRUNTIME_API UFlibPatchParserHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:

	UFUNCTION(BlueprintCallable, Category = "HotPatcher|Flib")
		static TArray<FString> GetAvailableMaps(FString GameName, bool IncludeEngineMaps,bool IncludePluginMaps, bool Sorted);
	UFUNCTION(BlueprintCallable, Category = "HotPatcher|Flib")
		static FString GetProjectName();
	UFUNCTION(BlueprintCallable, Category = "HotPatcher|Flib")
		static FString GetUnrealPakBinary();
	UFUNCTION(BlueprintCallable, Category = "HotPatcher|Flib")
		static FString GetUE4CmdBinary();

	static FHotPatcherVersion ExportReleaseVersionInfo(
        const FString& InVersionId,
        const FString& InBaseVersion,
        const FString& InDate,
        const TArray<FString>& InIncludeFilter,
        const TArray<FString>& InIgnoreFilter,
        const TArray<EAssetRegistryDependencyTypeEx>& AssetRegistryDependencyTypes,
        const TArray<FPatcherSpecifyAsset>& InIncludeSpecifyAsset,
        // const TArray<FExternAssetFileInfo>& InAllExternFiles,
        const TArray<FPlatformExternAssets>& AddToPlatformExFiles,
        bool InIncludeHasRefAssetsOnly = false,
        bool bInAnalysisFilterDepend = true
    );
	static FHotPatcherVersion ExportReleaseVersionInfoByChunk(
		const FString& InVersionId,
		const FString& InBaseVersion,
		const FString& InDate,
		const FChunkInfo& InChunkInfo,
		bool InIncludeHasRefAssetsOnly = false,
		bool bInAnalysisFilterDepend = true
	);


	UFUNCTION(BlueprintCallable, Category = "HotPatcher|Flib")
		static bool DiffVersionAssets(const FAssetDependenciesInfo& InNewVersion, 
								const FAssetDependenciesInfo& InBaseVersion,
								FAssetDependenciesInfo& OutAddAsset,
								FAssetDependenciesInfo& OutModifyAsset,
								FAssetDependenciesInfo& OutDeleteAsset
		);

	UFUNCTION()
	static bool DiffVersionAllPlatformExFiles(
        const FHotPatcherVersion& InBaseVersion,
        const FHotPatcherVersion& InNewVersion,
		TMap<ETargetPlatform,FPatchVersionExternDiff>& OutDiff        
    );
	UFUNCTION()
	static FPlatformExternFiles GetAllExFilesByPlatform(const FPlatformExternAssets& InPlatformConf,bool InGeneratedHash=true);
	UFUNCTION(BlueprintCallable, Category = "HotPatcher|Flib")
		static bool GetPakFileInfo(const FString& InFile,FPakFileInfo& OutFileInfo);

	// Cooked/PLATFORM_NAME/Engine/GlobalShaderCache-*.bin
	UFUNCTION(BlueprintCallable, Category = "HotPatcher|Flib")
		static TArray<FString> GetCookedGlobalShaderCacheFiles(const FString& InProjectDir,const FString& InPlatformName);
	// Cooked/PLATFORN_NAME/PROJECT_NAME/AssetRegistry.bin
	UFUNCTION(BlueprintCallable, Category = "HotPatcher|Flib")
		static bool GetCookedAssetRegistryFiles(const FString& InProjectAbsDir, const FString& InProjectName, const FString& InPlatformName,FString& OutFiles);
	// Cooked/PLATFORN_NAME/PROJECT_NAME/Content/ShaderArchive-*.ushaderbytecode
	UFUNCTION(BlueprintCallable, Category = "HotPatcher|Flib")
		static bool GetCookedShaderBytecodeFiles(const FString& InProjectAbsDir, const FString& InProjectName, const FString& InPlatformName,bool InGalobalBytecode,bool InProjectBytecode, TArray<FString>& OutFiles);

	// UFUNCTION(BlueprintCallable, Category = "HotPatcher|Flib")
		static bool ConvIniFilesToPakCommands(
			const FString& InEngineAbsDir, 
			const FString& InProjectAbsDir, 
			const FString& InProjectName, 
			const TArray<FString>& InPakOptions, 
			const TArray<FString>& InIniFiles, 
			TArray<FString>& OutCommands, 
			TFunction<void(const FPakCommand&)> InReceiveCommand = [](const FPakCommand&) {});

	// UFUNCTION(BlueprintCallable, Category = "HotPatcher|Flib")
		static bool ConvNotAssetFileToPakCommand(
			const FString& InProjectDir,
			const FString& InPlatformName, 
			const TArray<FString>& InPakOptions,
			const FString& InCookedFile,
			FString& OutCommand,
			TFunction<void(const FPakCommand&)> InReceiveCommand = [](const FPakCommand&) {});
	static bool ConvNotAssetFileToExFile(const FString& InProjectDir, const FString& InPlatformName, const FString& InCookedFile, FExternFileInfo& OutExFile);
	UFUNCTION(BlueprintCallable, Category = "HotPatcher|Flib")
	static FString HashStringWithSHA1(const FString &InString);

	UFUNCTION(BlueprintCallable, Category = "HotPatcher|Flib")
		static TArray<FString> GetIniConfigs(const FString& InSearchDir, const FString& InPlatformName);
	// return abslute path
	UFUNCTION(BlueprintCallable, Category = "HotPatcher|Flib")
	static TArray<FString> GetProjectIniFiles(const FString& InProjectDir,const FString& InPlatformName);

	UFUNCTION(BlueprintCallable, Category = "HotPatcher|Flib")
	static TArray<FString> GetEngineConfigs(const FString& InPlatformName);

	UFUNCTION(BlueprintCallable, Category = "HotPatcher|Flib")
	static TArray<FString> GetEnabledPluginConfigs(const FString& InPlatformName);


	static TArray<FExternFileInfo> ParserExDirectoryAsExFiles(const TArray<FExternDirectoryInfo>& InExternDirectorys);
	static TArray<FAssetDetail> ParserExFilesInfoAsAssetDetailInfo(const TArray<FExternFileInfo>& InExFiles);

	// get Engine / Project / Plugin ini files
	static TArray<FString> GetIniFilesByPakInternalInfo(const FPakInternalInfo& InPakInternalInfo,const FString& PlatformName);
	// get AssetRegistry.bin / GlobalShaderCache / ShaderBytecode
	static TArray<FString> GetCookedFilesByPakInternalInfo(
		const FPakInternalInfo& InPakInternalInfo, 
		const FString& PlatformName);

	static TArray<FExternFileInfo> GetInternalFilesAsExFiles(const FPakInternalInfo& InPakInternalInfo, const FString& InPlatformName);
	static TArray<FString> GetPakCommandsFromInternalInfo(
		const FPakInternalInfo& InPakInternalInfo, 
		const FString& PlatformName, 
		const TArray<FString>& InPakOptions, 
		TFunction<void(const FPakCommand&)> InReceiveCommand=[](const FPakCommand&) {});
	
	static FChunkInfo CombineChunkInfo(const FChunkInfo& R, const FChunkInfo& L);
	static FChunkInfo CombineChunkInfos(const TArray<FChunkInfo>& Chunks);

	static TArray<FString> GetDirectoryPaths(const TArray<FDirectoryPath>& InDirectoryPath);
	
	// static TArray<FExternFileInfo> GetExternFilesFromChunk(const FChunkInfo& InChunk, TArray<ETargetPlatform> InTargetPlatforms, bool bCalcHash = false);
	TMap<ETargetPlatform,FPlatformExternFiles> GetAllPlatformExternFilesFromChunk(const FChunkInfo& InChunk, bool bCalcHash);
	static FPatchVersionDiff DiffPatchVersionWithPatchSetting(const struct FExportPatchSettings& PatchSetting, const FHotPatcherVersion& Base, const FHotPatcherVersion& New);

	static FChunkAssetDescribe CollectFChunkAssetsDescribeByChunk(const FPatchVersionDiff& DiffInfo, const FChunkInfo& Chunk, TArray<ETargetPlatform> Platforms);

	static TArray<FString> CollectPakCommandsStringsByChunk(const FPatchVersionDiff& DiffInfo, const FChunkInfo& Chunk, const FString& PlatformName, const TArray<FString>& PakOptions);

	static TArray<FPakCommand> CollectPakCommandByChunk(const FPatchVersionDiff& DiffInfo, const FChunkInfo& Chunk, const FString& PlatformName, const TArray<FString>& PakOptions);
	// CurrenrVersionChunk中的过滤器会进行依赖分析，TotalChunk的不会，目的是让用户可以自己控制某个文件夹打包到哪个Pak里，而不会对该文件夹下的资源进行依赖分析
	static FChunkAssetDescribe DiffChunkWithPatchSetting(const struct FExportPatchSettings& PatchSetting, const FChunkInfo& CurrentVersionChunk, const FChunkInfo& TotalChunk);
	static FChunkAssetDescribe DiffChunkByBaseVersionWithPatchSetting(const struct FExportPatchSettings& PatchSetting, const FChunkInfo& CurrentVersionChunk, const FChunkInfo& TotalChunk, const FHotPatcherVersion& BaseVersion);
	static TArray<FString> GetPakCommandStrByCommands(const TArray<FPakCommand>& PakCommands, const TArray<FReplaceText>& InReplaceTexts = TArray<FReplaceText>{});

	static FProcHandle DoUnrealPak(TArray<FString> UnrealPakOptions, bool block);

	static FHotPatcherAssetDependency GetAssetRelatedInfo(const FAssetDetail& InAsset, const TArray<EAssetRegistryDependencyTypeEx>& AssetRegistryDependencyTypes);
	static TArray<FHotPatcherAssetDependency> GetAssetsRelatedInfo(const TArray<FAssetDetail>& InAssets, const TArray<EAssetRegistryDependencyTypeEx>& AssetRegistryDependencyTypes);
	static TArray<FHotPatcherAssetDependency> GetAssetsRelatedInfoByFAssetDependencies(const FAssetDependenciesInfo& InAssetsDependencies, const TArray<EAssetRegistryDependencyTypeEx>& AssetRegistryDependencyTypes);

	static bool GetCookProcCommandParams(const FCookerConfig& InConfig,FString& OutParams);
	//static bool SerializeMonolithicPathMode(const EMonolithicPathMode& InMode, TSharedPtr<FJsonValue>& OutJsonValue);
	//static bool DeSerializeMonolithicPathMode(const TSharedPtr<FJsonValue>& InJsonValue, EMonolithicPathMode& OutMode);

	static void ExcludeContentForVersionDiff(FPatchVersionDiff& VersionDiff,const TArray<FString>& ExcludeRules = {TEXT("")});
	
	template<typename ENUM_TYPE>
	static FString GetEnumNameByValue(ENUM_TYPE InEnumValue, bool bFullName = false)
	{
		FString result;
		{
			FString TypeName;
			FString ValueName;

#if ENGINE_MINOR_VERSION > 21
			UEnum* FoundEnum = StaticEnum<ENUM_TYPE>();
#else
			FString EnumTypeName = ANSI_TO_TCHAR(UFlibPatchParserHelper::GetCPPTypeName<ENUM_TYPE>().c_str());
			UEnum* FoundEnum = FindObject<UEnum>(ANY_PACKAGE, *EnumTypeName, true); 
#endif
			if (FoundEnum)
			{
				result = FoundEnum->GetNameByValue((int64)InEnumValue).ToString();
				result.Split(TEXT("::"), &TypeName, &ValueName, ESearchCase::CaseSensitive, ESearchDir::FromEnd);
				if (!bFullName)
				{
					result = ValueName;
				}
			}
		}
		return result;
	}

	template<typename ENUM_TYPE>
	static bool GetEnumValueByName(const FString& InEnumValueName, ENUM_TYPE& OutEnumValue)
	{
		bool bStatus = false;

#if ENGINE_MINOR_VERSION > 21
		UEnum* FoundEnum = StaticEnum<ENUM_TYPE>();
		FString EnumTypeName = FoundEnum->CppType;
#else
		FString EnumTypeName = ANSI_TO_TCHAR(UFlibPatchParserHelper::GetCPPTypeName<ENUM_TYPE>().c_str());
		UEnum* FoundEnum = FindObject<UEnum>(ANY_PACKAGE, *EnumTypeName, true); 
#endif
		if (FoundEnum)
		{
			FString EnumValueFullName = EnumTypeName + TEXT("::") + InEnumValueName;
			int32 EnumIndex = FoundEnum->GetIndexByName(FName(*EnumValueFullName));
			if (EnumIndex != INDEX_NONE)
			{
				int32 EnumValue = FoundEnum->GetValueByIndex(EnumIndex);
				ENUM_TYPE ResultEnumValue = (ENUM_TYPE)EnumValue;
				OutEnumValue = ResultEnumValue;
				bStatus = true;
			}
		}
		return bStatus;
	}

#if ENGINE_MINOR_VERSION <= 21
	template<typename T>
	static std::string GetCPPTypeName()
	{
		std::string result;
		std::string type_name = typeid(T).name();

		std::for_each(type_name.begin(),type_name.end(),[&result](const char& character){if(!std::isdigit(character)) result.push_back(character);});

		return result;
	}
#endif
	static FString MountPathToRelativePath(const FString& InMountPath);

	// reload Global&Project shaderbytecode
	UFUNCTION(BlueprintCallable)
		static void ReloadShaderbytecode();


	static FString SerializeAssetsDependencyAsJsonString(const TArray<FHotPatcherAssetDependency>& InAssetsDependency);
	static bool SerializePlatformPakInfoToString(const TMap<FString, TArray<FPakFileInfo>>& InPakFilesMap, FString& OutString);
	static bool SerializePlatformPakInfoToJsonObject(const TMap<FString, TArray<FPakFileInfo>>& InPakFilesMap, TSharedPtr<FJsonObject>& OutJsonObject);
	template<typename TStructType>
	static bool TSerializeStructAsJsonObject(const TStructType& InStruct,TSharedPtr<FJsonObject>& OutJsonObject)
	{
		if(!OutJsonObject.IsValid())
		{
			OutJsonObject = MakeShareable(new FJsonObject);
		}
		bool bStatus = FJsonObjectConverter::UStructToJsonObject(TStructType::StaticStruct(),&InStruct,OutJsonObject.ToSharedRef(),0,0);
		return bStatus;
	}

	template<typename TStructType>
    static bool TDeserializeJsonObjectAsStruct(const TSharedPtr<FJsonObject>& OutJsonObject,TStructType& InStruct)
	{
		bool bStatus = false;
		if(OutJsonObject.IsValid())
		{
			bStatus = FJsonObjectConverter::JsonObjectToUStruct(OutJsonObject.ToSharedRef(),TStructType::StaticStruct(),&InStruct,0,0);
		}
		return bStatus;
	}

	template<typename TStructType>
    static bool TSerializeStructAsJsonString(const TStructType& InStruct,FString& OutJsonString)
	{
		bool bRunStatus = false;

		{
			TSharedPtr<FJsonObject> JsonObject;
			if (UFlibPatchParserHelper::TSerializeStructAsJsonObject<TStructType>(InStruct,JsonObject) && JsonObject.IsValid())
			{
				auto JsonWriter = TJsonWriterFactory<>::Create(&OutJsonString);
				FJsonSerializer::Serialize(JsonObject.ToSharedRef(), JsonWriter);
				bRunStatus = true;
			}
		}
		return bRunStatus;
	}

	template<typename TStructType>
    static bool TDeserializeJsonStringAsStruct(const FString& InJsonString,TStructType& OutStruct)
	{
		bool bRunStatus = false;
		TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(InJsonString);
		TSharedPtr<FJsonObject> DeserializeJsonObject;
		if (FJsonSerializer::Deserialize(JsonReader, DeserializeJsonObject))
		{
			bRunStatus = UFlibPatchParserHelper::TDeserializeJsonObjectAsStruct<TStructType>(DeserializeJsonObject,OutStruct);
		}
		return bRunStatus;
	}


	UFUNCTION(BlueprintCallable)
	static TArray<FAssetDetail> GetAllAssetDependencyDetails(const FAssetDetail& Asset,const TArray<EAssetRegistryDependencyTypeEx>& Types,const FString& AssetType = TEXT(""));
	/*
	 * 0x1 Add
	 * 0x2 Modyfy
	 */
	static void AnalysisWidgetTree(FPatchVersionDiff& PakDiff,int32 flags = 0x1|0x2);
};
