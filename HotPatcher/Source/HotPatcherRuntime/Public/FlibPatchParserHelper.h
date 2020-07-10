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
#include "FAssetRelatedInfo.h"
#include "FCookerConfig.h"

// cpp standard
#include <typeinfo>

// engine header
#include "CoreMinimal.h"
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

	UFUNCTION(BlueprintCallable, Category = "HotPatcher|Flib")
		static bool SerializeHotPatcherVersionToString(const FHotPatcherVersion& InVersion, FString& OutResault);
	static bool SerializeHotPatcherVersionToJsonObject(const FHotPatcherVersion& InVersion, TSharedPtr<FJsonObject>& OutJsonObject);
	UFUNCTION(BlueprintCallable, Category = "HotPatcher|Flib")
		static bool DeserializeHotPatcherVersionFromString(const FString& InStringContent, FHotPatcherVersion& OutVersion);
	static bool DeSerializeHotPatcherVersionFromJsonObject(const TSharedPtr<FJsonObject>& InJsonObject, FHotPatcherVersion& OutVersion);

	static FHotPatcherVersion ExportReleaseVersionInfo(
		const FString& InVersionId,
		const FString& InBaseVersion,
		const FString& InDate,
		const TArray<FString>& InIncludeFilter,
		const TArray<FString>& InIgnoreFilter,
		const TArray<EAssetRegistryDependencyTypeEx>& AssetRegistryDependencyTypes,
		const TArray<FPatcherSpecifyAsset>& InIncludeSpecifyAsset,
		const TArray<FExternAssetFileInfo>& InAllExternFiles,
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
	UFUNCTION(BlueprintCallable, Category = "HotPatcher|Flib")
		static bool DiffVersionExFiles(const FHotPatcherVersion& InNewVersion,
			const FHotPatcherVersion& InBaseVersion,
			TArray<FExternAssetFileInfo>& OutAddFiles,
			TArray<FExternAssetFileInfo>& OutModifyFiles,
			TArray<FExternAssetFileInfo>& OutDeleteFiles
		);

	UFUNCTION(BlueprintCallable, Category = "HotPatcher|Flib")
		static bool SerializePakFileInfoToJsonString(const FPakFileInfo& InFileInfo, FString& OutJson);

		static bool SerializePakFileInfoFromJsonObjectToString(const TSharedPtr<FJsonObject>& InFileInfoJsonObject, FString& OutJson);
		static bool SerializePakFileInfoToJsonObject(const FPakFileInfo& InFileInfo, TSharedPtr<FJsonObject>& OutJsonObject);

		static bool SerializePakFileInfoListToJsonObject(const TArray<FPakFileInfo>& InFileInfoList, TSharedPtr<FJsonObject>& OutJsonObject);
		
		static bool SerializePlatformPakInfoToString(const TMap<FString, TArray<FPakFileInfo>>& InPakFilesMap, FString& OutString);
		static bool SerializePlatformPakInfoToJsonObject(const TMap<FString, TArray<FPakFileInfo>>& InPakFilesMap, TSharedPtr<FJsonObject>& OutJsonObject);

		static bool SerializeDiffAssetsInfomationToJsonObject(const FAssetDependenciesInfo& InAddAsset,
				const FAssetDependenciesInfo& InModifyAsset,
				const FAssetDependenciesInfo& InDeleteAsset,
				TSharedPtr<FJsonObject>& OutJsonObject);

	UFUNCTION(BlueprintCallable, Category = "HotPatcher|Flib")
		static FString SerializeDiffAssetsInfomationToString(const FAssetDependenciesInfo& InAddAsset,
		 									 const FAssetDependenciesInfo& InModifyAsset,
											 const FAssetDependenciesInfo& InDeleteAsset);

		static bool SerializeDiffExternalFilesInfomationToJsonObject(
			const TArray<FExternAssetFileInfo>& InAddFiles,
			const TArray<FExternAssetFileInfo>& InModifyFiles,
			const TArray<FExternAssetFileInfo>& InDeleteFiles,
			TSharedPtr<FJsonObject>& OutJsonObject);

	UFUNCTION(BlueprintCallable, Category = "HotPatcher|Flib")
		static FString SerializeDiffExternalFilesInfomationToString(
			const TArray<FExternAssetFileInfo>& InAddFiles,
			const TArray<FExternAssetFileInfo>& InModifyFiles,
			const TArray<FExternAssetFileInfo>& InDeleteFiles);

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
	static bool ConvNotAssetFileToExFile(const FString& InProjectDir, const FString& InPlatformName, const FString& InCookedFile, FExternAssetFileInfo& OutExFile);
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


	static bool SerializeExAssetFileInfoToJsonObject(const FExternAssetFileInfo& InExFileInfo, TSharedPtr<FJsonObject>& OutJsonObject);
	static bool SerializeExDirectoryInfoToJsonObject(const FExternDirectoryInfo& InExDirectoryInfo, TSharedPtr<FJsonObject>& OutJsonObject);
	static bool SerializeSpecifyAssetInfoToJsonObject(const FPatcherSpecifyAsset& InSpecifyAsset, TSharedPtr<FJsonObject>& OutJsonObject);
	static bool DeSerializeSpecifyAssetInfoToJsonObject(const TSharedPtr<FJsonObject>& InJsonObject, FPatcherSpecifyAsset& OutSpecifyAsset);
	// serialize chunk
	static bool SerializeFPakInternalInfoToJsonObject(const FPakInternalInfo& InFPakInternalInfo, TSharedPtr<FJsonObject>& OutJsonObject);
	static bool DeSerializeFPakInternalInfoFromJsonObject(const TSharedPtr<FJsonObject>& JsonObject, FPakInternalInfo& OutFPakInternalInfo);
	static bool SerializeFChunkInfoToJsonObject(const FChunkInfo& InChunkInfo, TSharedPtr<FJsonObject>& OutJsonObject);
	static bool DeSerializeFChunkInfoFromJsonObject(const TSharedPtr<FJsonObject>& InJsonObject, FChunkInfo& OutChunkInfo);

	static TArray<FExternAssetFileInfo> ParserExDirectoryAsExFiles(const TArray<FExternDirectoryInfo>& InExternDirectorys);
	static TArray<FAssetDetail> ParserExFilesInfoAsAssetDetailInfo(const TArray<FExternAssetFileInfo>& InExFiles);

	// get Engine / Project / Plugin ini files
	static TArray<FString> GetIniFilesByPakInternalInfo(const FPakInternalInfo& InPakInternalInfo,const FString& PlatformName);
	// get AssetRegistry.bin / GlobalShaderCache / ShaderBytecode
	static TArray<FString> GetCookedFilesByPakInternalInfo(
		const FPakInternalInfo& InPakInternalInfo, 
		const FString& PlatformName);

	static TArray<FExternAssetFileInfo> GetInternalFilesAsExFiles(const FPakInternalInfo& InPakInternalInfo, const FString& InPlatformName);
	static TArray<FString> GetPakCommandsFromInternalInfo(
		const FPakInternalInfo& InPakInternalInfo, 
		const FString& PlatformName, 
		const TArray<FString>& InPakOptions, 
		TFunction<void(const FPakCommand&)> InReceiveCommand=[](const FPakCommand&) {});
	
	static FChunkInfo CombineChunkInfo(const FChunkInfo& R, const FChunkInfo& L);
	static FChunkInfo CombineChunkInfos(const TArray<FChunkInfo>& Chunks);

	static TArray<FString> GetDirectoryPaths(const TArray<FDirectoryPath>& InDirectoryPath);
	
	static TArray<FExternAssetFileInfo> GetExternFilesFromChunk(const FChunkInfo& InChunk, bool bCalcHash = false);
	static FPatchVersionDiff DiffPatchVersion(const FHotPatcherVersion& Base, const FHotPatcherVersion& New);

	static FChunkAssetDescribe CollectFChunkAssetsDescribeByChunk(const FPatchVersionDiff& DiffInfo, const FChunkInfo& Chunk);

	static TArray<FString> CollectPakCommandsStringsByChunk(const FPatchVersionDiff& DiffInfo, const FChunkInfo& Chunk, const FString& PlatformName, const TArray<FString>& PakOptions);

	static TArray<FPakCommand> CollectPakCommandByChunk(const FPatchVersionDiff& DiffInfo, const FChunkInfo& Chunk, const FString& PlatformName, const TArray<FString>& PakOptions);
	// CurrenrVersionChunk中的过滤器会进行依赖分析，TotalChunk的不会，目的是让用户可以自己控制某个文件夹打包到哪个Pak里，而不会对该文件夹下的资源进行依赖分析
	static FChunkAssetDescribe DiffChunk(const FChunkInfo& CurrentVersionChunk,const FChunkInfo& TotalChunk, bool InIncludeHasRefAssetsOnly);
	static FChunkAssetDescribe DiffChunkByBaseVersion(const FChunkInfo& CurrentVersionChunk, const FChunkInfo& TotalChunk, const FHotPatcherVersion& BaseVersion, bool InIncludeHasRefAssetsOnly);
	static TArray<FString> GetPakCommandStrByCommands(const TArray<FPakCommand>& PakCommands, const TArray<FReplaceText>& InReplaceTexts = TArray<FReplaceText>{});

	static FProcHandle DoUnrealPak(TArray<FString> UnrealPakOptions, bool block);

	static FAssetRelatedInfo GetAssetRelatedInfo(const FAssetDetail& InAsset, const TArray<EAssetRegistryDependencyTypeEx>& AssetRegistryDependencyTypes);
	static TArray<FAssetRelatedInfo> GetAssetsRelatedInfo(const TArray<FAssetDetail>& InAssets, const TArray<EAssetRegistryDependencyTypeEx>& AssetRegistryDependencyTypes);
	static TArray<FAssetRelatedInfo> GetAssetsRelatedInfoByFAssetDependencies(const FAssetDependenciesInfo& InAssetsDependencies, const TArray<EAssetRegistryDependencyTypeEx>& AssetRegistryDependencyTypes);

	static bool SerializeAssetRelatedInfoAsJsonObject(const FAssetRelatedInfo& InAssetDependency, TSharedPtr<FJsonObject>& OutJsonObject);
	static bool SerializeAssetsRelatedInfoAsJsonObject(const TArray<FAssetRelatedInfo>& InAssetsDependency, TSharedPtr<FJsonObject>& OutJsonObject);

	static bool SerializeAssetsRelatedInfoAsString(const TArray<FAssetRelatedInfo>& InAssetsDependency,FString& OutString);

	static TArray<TSharedPtr<FJsonValue>> SerializeFReplaceTextsAsJsonValues(const TArray<FReplaceText>& InReplaceTexts);
	static TSharedPtr<FJsonObject> SerializeFReplaceTextAsJsonObject(const FReplaceText& InReplaceText);
	static FReplaceText DeSerializeFReplaceText(const TSharedPtr<FJsonObject>& InReplaceTextJsonObject);

	static TSharedPtr<FJsonObject> SerializeCookerConfigAsJsonObject(const FCookerConfig& InConfig);
	static FString SerializeCookerConfigAsString(const TSharedPtr<FJsonObject>& InConfigJson);
	static FCookerConfig DeSerializeCookerConfig(const FString& InJsonContent);


	static bool GetCookProcCommandParams(const FCookerConfig& InConfig,FString& OutParams);
	//static bool SerializeMonolithicPathMode(const EMonolithicPathMode& InMode, TSharedPtr<FJsonValue>& OutJsonValue);
	//static bool DeSerializeMonolithicPathMode(const TSharedPtr<FJsonValue>& InJsonValue, EMonolithicPathMode& OutMode);


	template<typename ENUM_TYPE>
	static FString GetEnumNameByValue(ENUM_TYPE InEnumValue, bool bFullName = false)
	{
		FString result;
		{
			FString TypeName;
			FString ValueName;

			UEnum* FoundEnum = StaticEnum<ENUM_TYPE>();
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

		UEnum* FoundEnum = StaticEnum<ENUM_TYPE>();

		FString EnumTypeName = FoundEnum->CppType;
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

	static FString MountPathToRelativePath(const FString& InMountPath);

	// reload Global&Project shaderbytecode
	UFUNCTION(BlueprintCallable)
		static void ReloadShaderbytecode();
};
