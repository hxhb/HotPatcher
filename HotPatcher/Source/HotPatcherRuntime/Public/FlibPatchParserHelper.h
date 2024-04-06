// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

//project header
#include "BaseTypes/FAssetScanConfig.h"
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
#include "Templates/HotPatcherTemplateHelper.hpp"
#include "AssetRegistry.h"

// engine header
#include "CoreMinimal.h"
#include "Resources/Version.h"
#include "JsonObjectConverter.h"
#include "Misc/CommandLine.h"
#include "FPlatformExternAssets.h"
#include "Containers/UnrealString.h"
#include "CreatePatch/FExportPatchSettings.h"
#include "Templates/SharedPointer.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Misc/FileHelper.h"
#include "FlibPatchParserHelper.generated.h"

struct FExportPatchSettings;
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
	UFUNCTION(BlueprintCallable)
	static FString GetProjectFilePath();
	
	static FHotPatcherVersion ExportReleaseVersionInfoByChunk(
		const FString& InVersionId,
		const FString& InBaseVersion,
		const FString& InDate,
		const FChunkInfo& InChunkInfo,
		bool InIncludeHasRefAssetsOnly = false,
		bool bInAnalysisFilterDependencies = true, EHashCalculator HashCalculator = EHashCalculator::NoHash
	);
	static void RunAssetScanner(FAssetScanConfig ScanConfig,FHotPatcherVersion& ExportVersion);
	static void ExportExternAssetsToPlatform(const TArray<FPlatformExternAssets>& AddExternAssetsToPlatform, FHotPatcherVersion& ExportVersion, bool bGenerateHASH, EHashCalculator
	                                         HashCalculator);

	UFUNCTION(BlueprintCallable, Category = "HotPatcher|Flib")
		static bool DiffVersionAssets(const FAssetDependenciesInfo& InNewVersion, 
								const FAssetDependenciesInfo& InBaseVersion,
								FAssetDependenciesInfo& OutAddAsset,
								FAssetDependenciesInfo& OutModifyAsset,
								FAssetDependenciesInfo& OutDeleteAsset
		);

	// UFUNCTION()
	static bool DiffVersionAllPlatformExFiles(
		const FExportPatchSettings& PatchSetting,
        const FHotPatcherVersion& InBaseVersion,
        const FHotPatcherVersion& InNewVersion,
		TMap<ETargetPlatform,FPatchVersionExternDiff>& OutDiff        
    );
	UFUNCTION()
	static FPlatformExternFiles GetAllExFilesByPlatform(const FPlatformExternAssets& InPlatformConf, bool InGeneratedHash=true, EHashCalculator HashCalculator = EHashCalculator::NoHash);
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
			// const TArray<FString>& InPakOptions, 
			const TArray<FString>& InIniFiles, 
			TArray<FString>& OutCommands, 
			TFunction<void(const FPakCommand&)> InReceiveCommand = [](const FPakCommand&) {});

	// UFUNCTION(BlueprintCallable, Category = "HotPatcher|Flib")
		static bool ConvNotAssetFileToPakCommand(
			const FString& InProjectDir,
			const FString& InPlatformName, 
			// const TArray<FString>& InPakOptions,
			const FString& InCookedFile,
			FString& OutCommand,
			TFunction<void(const FPakCommand&)> InReceiveCommand = [](const FPakCommand&) {});
	// static bool ConvNotAssetFileToExFile(const FString& InProjectDir, const FString& InPlatformName, const FString& InCookedFile, FExternFileInfo& OutExFile);
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


	static TArray<FExternFileInfo> ParserExDirectoryAsExFiles(const TArray<FExternDirectoryInfo>& InExternDirectorys,EHashCalculator HashCalculator,bool InGeneratedHash = true);
	static TArray<FAssetDetail> ParserExFilesInfoAsAssetDetailInfo(const TArray<FExternFileInfo>& InExFiles);

	// get Engine / Project / Plugin ini files
	static TArray<FString> GetIniFilesByPakInternalInfo(const FPakInternalInfo& InPakInternalInfo,const FString& PlatformName);
	// get AssetRegistry.bin / GlobalShaderCache / ShaderBytecode
	static TArray<FString> GetCookedFilesByPakInternalInfo(
		const FPakInternalInfo& InPakInternalInfo, 
		const FString& PlatformName);

	// static TArray<FExternFileInfo> GetInternalFilesAsExFiles(const FPakInternalInfo& InPakInternalInfo, const FString& InPlatformName);
	static TArray<FString> GetPakCommandsFromInternalInfo(
		const FPakInternalInfo& InPakInternalInfo, 
		const FString& PlatformName, 
		// const TArray<FString>& InPakOptions, 
		TFunction<void(const FPakCommand&)> InReceiveCommand=[](const FPakCommand&) {});
	
	static FChunkInfo CombineChunkInfo(const FChunkInfo& R, const FChunkInfo& L);
	static FChunkInfo CombineChunkInfos(const TArray<FChunkInfo>& Chunks);

	static TArray<FString> GetDirectoryPaths(const TArray<FDirectoryPath>& InDirectoryPath);
	
	// static TArray<FExternFileInfo> GetExternFilesFromChunk(const FChunkInfo& InChunk, TArray<ETargetPlatform> InTargetPlatforms, bool bCalcHash = false);
	TMap<ETargetPlatform,FPlatformExternFiles> GetAllPlatformExternFilesFromChunk(const FChunkInfo& InChunk, bool bCalcHash);

	static FChunkAssetDescribe CollectFChunkAssetsDescribeByChunk(
		const FHotPatcherSettingBase* PatcheSettings,
		const FPatchVersionDiff& DiffInfo, const FChunkInfo& Chunk, TArray<ETargetPlatform> Platforms
	);

	static TArray<FString> CollectPakCommandsStringsByChunk(
		const FPatchVersionDiff& DiffInfo,
		const FChunkInfo& Chunk,
		const FString& PlatformName,
		// const TArray<FString>& PakOptions,
		const FExportPatchSettings* PatcheSettings = nullptr
	);

	static TArray<FPakCommand> CollectPakCommandByChunk(
		const FPatchVersionDiff& DiffInfo,
		const FChunkInfo& Chunk,
		const FString& PlatformName,
		// const TArray<FString>& PakOptions,
		const FExportPatchSettings* PatcheSettings=nullptr
	);

	static TArray<FString> GetPakCommandStrByCommands(const TArray<FPakCommand>& PakCommands, const TArray<FReplaceText>& InReplaceTexts = TArray<FReplaceText>{},bool bIoStore=false);
	static bool GetCookProcCommandParams(const FCookerConfig& InConfig,FString& OutParams);
	static void ExcludeContentForVersionDiff(FPatchVersionDiff& VersionDiff,const TArray<FString>& ExcludeRules = {TEXT("")},EHotPatcherMatchModEx matchMod=EHotPatcherMatchModEx::StartWith);
	static FString MountPathToRelativePath(const FString& InMountPath);


	static TMap<FString,FString> GetReplacePathMarkMap();
	static FString ReplaceMark(const FString& Src);
	static FString ReplaceMarkPath(const FString& Src);
	static FString MakeMark(const FString& Src);
	// [PORJECTDIR] to real path
	static void ReplacePatherSettingProjectDir(TArray<FPlatformExternAssets>& PlatformAssets);


	static TArray<FString> GetUnCookUassetExtensions();
	static TArray<FString> GetCookedUassetExtensions();
	static bool IsCookedUassetExtensions(const FString& InAsset);
	static bool IsUnCookUassetExtension(const FString& InAsset);

	// ../../../Content/xxxx.uasset to D:/xxxx/xxx/xxx.uasset
	static FString AssetMountPathToAbs(const FString& InAssetMountPath);
	// ../../../Content/xxxx.uasset to /Game/xxxx
	static FString UAssetMountPathToPackagePath(const FString& InAssetMountPath);

	static bool MatchStrInArray(const FString& InStr,const TArray<FString>& InArray);
	static FString LoadAESKeyStringFromCryptoFile(const FString& InCryptoJson);
	static FAES::FAESKey LoadAESKeyFromCryptoFile(const FString& InCryptoJson);
public:
	static bool GetPluginPakPathByName(const FString& PluginName,FString& uPluginAbsPath,FString& uPluginMountPath);
	// ../../../Example/Plugin/XXXX/
	static FString GetPluginMountPoint(const FString& PluginName);
	// [PRIJECTDIR]/AssetRegistry to ../../../Example/AssetRegistry
	static FString ParserMountPointRegular(const FString& Src);

public:
	UFUNCTION(BlueprintCallable)
	static void ReloadShaderbytecode();
	UFUNCTION(BlueprintCallable,Exec)
		static bool LoadShaderbytecode(const FString& LibraryName, const FString& LibraryDir);	
	UFUNCTION(BlueprintCallable,Exec)
		static void CloseShaderbytecode(const FString& LibraryName);

public:
	// Encrypt
	static FPakEncryptionKeys GetCryptoByProjectSettings();
	static FEncryptSetting GetCryptoSettingsByJson(const FString& CryptoJson);

	static FEncryptSetting GetCryptoSettingByPakEncryptSettings(const FPakEncryptSettings& Config);
	
	static bool SerializePakEncryptionKeyToFile(const FPakEncryptionKeys& PakEncryptionKeys,const FString& ToFile);

	static TArray<FDirectoryPath> GetDefaultForceSkipContentDir();

	static FSHAHash FileSHA1Hash(const FString& Filename);
	
	static FString FileHash(const FString& Filename,EHashCalculator Calculator);

	template<typename T>
	static bool SerializeStruct(T SerializeStruct,const FString& SaveTo)
	{
		SCOPED_NAMED_EVENT_TEXT("SerializeStruct",FColor::Red);
		FString SaveToPath = UFlibPatchParserHelper::ReplaceMark(SaveTo);
		FString SerializedJsonContent;
		THotPatcherTemplateHelper::TSerializeStructAsJsonString(SerializeStruct,SerializedJsonContent);
		return FFileHelper::SaveStringToFile(SerializedJsonContent,*FPaths::ConvertRelativePathToFull(SaveToPath));
	}

	static bool IsValidPatchSettings(const FExportPatchSettings* PatchSettings,bool bExternalFilesCheck);
	static void SetPropertyTransient(UStruct* Struct,const FString& PropertyName,bool bTransient);
	static FString GetTargetPlatformsCmdLine(const TArray<ETargetPlatform>& Platforms);
	static FString GetTargetPlatformsStr(const TArray<ETargetPlatform>& Platforms);
	static FString MergeOptionsAsCmdline(const TArray<FString>& InOptions);
	static FString GetPlatformsStr(TArray<ETargetPlatform> Platforms);

	static bool GetCmdletBoolValue(const FString& Token,bool& OutValue);

	static FString ReplacePakRegular(const FReplacePakRegular& RegularConf, const FString& InRegular);
};

