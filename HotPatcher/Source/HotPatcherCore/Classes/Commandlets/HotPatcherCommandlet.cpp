#include "HotPatcherCommandlet.h"
// #include "CreatePatch/FExportPatchSettingsEx.h"
#include "CreatePatch/PatcherProxy.h"
#include "CommandletHelper.h"

// engine header
#include "CoreMinimal.h"
#include "FlibHotPatcherCoreHelper.h"
#include "Misc/FileHelper.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY(LogHotPatcherCommandlet);

#define ADD_ASSETS_BY_FILE TEXT("-AddAssetsByFile=")

int32 UHotPatcherCommandlet::Main(const FString& Params)
{
#if WITH_UE5
	PRIVATE_GIsRunningCookCommandlet = true;
#endif
	
	Super::Main(Params);

	FCommandLine::Append(TEXT(" -buildmachine"));
	GIsBuildMachine = true;
	
	UE_LOG(LogHotPatcherCommandlet, Display, TEXT("UHotPatcherCommandlet::Main"));

	FString config_path;
	bool bStatus = FParse::Value(*Params, *FString(PATCHER_CONFIG_PARAM_NAME).ToLower(), config_path);
	if (!bStatus)
	{
		UE_LOG(LogHotPatcherCommandlet, Error, TEXT("not -config=xxxx.json params."));
		return -1;
	}

	config_path = UFlibPatchParserHelper::ReplaceMark(config_path);
	if (!FPaths::FileExists(config_path))
	{
		UE_LOG(LogHotPatcherCommandlet, Error, TEXT("cofnig file %s not exists."), *config_path);
		return -1;
	}

	FString JsonContent;
	bool bExportStatus = false;
	if (FFileHelper::LoadFileToString(JsonContent, *config_path))
	{
		TSharedPtr<FExportPatchSettings> ExportPatchSetting = MakeShareable(new FExportPatchSettings);
		THotPatcherTemplateHelper::TDeserializeJsonStringAsStruct(JsonContent,*ExportPatchSetting);
		// adaptor old version config
		UFlibHotPatcherCoreHelper::AdaptorOldVersionConfig(ExportPatchSetting->GetAssetScanConfigRef(),JsonContent);
		
		TMap<FString, FString> KeyValues = THotPatcherTemplateHelper::GetCommandLineParamsMap(Params);
		THotPatcherTemplateHelper::ReplaceProperty(*ExportPatchSetting, KeyValues);
		TArray<ETargetPlatform> AddPlatforms = CommandletHelper::ParserPlatforms(Params,ADD_PATCH_PLATFORMS);
	
		if(AddPlatforms.Num())
		{
			for(auto& Platform:AddPlatforms)
			{
				ExportPatchSetting->PakTargetPlatforms.AddUnique(Platform);
			}
		}
		CommandletHelper::ModifyTargetPlatforms(Params,TARGET_PLATFORMS_OVERRIDE,ExportPatchSetting->PakTargetPlatforms,true);
		ExportPatchSetting->GetAssetScanConfigRef().AssetIncludeFilters.Append(CommandletHelper::ParserPatchFilters(Params,TEXT("AssetIncludeFilters")));
		ExportPatchSetting->GetAssetScanConfigRef().AssetIgnoreFilters.Append(CommandletHelper::ParserPatchFilters(Params,TEXT("AssetIgnoreFilters")));
		
		FString FinalConfig;
		THotPatcherTemplateHelper::TSerializeStructAsJsonString(*ExportPatchSetting,FinalConfig);
		// UE_LOG(LogHotPatcherCommandlet, Display, TEXT("%s"), *FinalConfig);

		// -AddAssetsByFile=
		{
			FString AddAssetByFile;
			bool bHasAssetByFileStatus = FParse::Value(*Params, *FString(ADD_ASSETS_BY_FILE).ToLower(), AddAssetByFile);
			if(bHasAssetByFileStatus && FPaths::FileExists(AddAssetByFile))
			{
				TArray<FString> PackageNames;
				FFileHelper::LoadFileToStringArray(PackageNames,*AddAssetByFile);
				for(const auto& PackageName:PackageNames)
				{
					if(!PackageName.IsEmpty() && FPackageName::DoesPackageExist(PackageName))
					{
						FString PackagePath = UFlibAssetManageHelper::LongPackageNameToPackagePath(PackageName);
						FPatcherSpecifyAsset SpecifyAsset;
						SpecifyAsset.Asset = FSoftObjectPath{PackagePath};
						SpecifyAsset.bAnalysisAssetDependencies = false;
						ExportPatchSetting->GetAssetScanConfigRef().IncludeSpecifyAssets.Add(SpecifyAsset);
					}
				}
			}
		}
		UPatcherProxy* PatcherProxy = NewObject<UPatcherProxy>();
		PatcherProxy->AddToRoot();
		PatcherProxy->Init(ExportPatchSetting.Get());
		PatcherProxy->OnPaking.AddStatic(&::CommandletHelper::ReceiveMsg);
		PatcherProxy->OnShowMsg.AddStatic(&::CommandletHelper::ReceiveShowMsg);
		bExportStatus = PatcherProxy->DoExport();
		
		UE_LOG(LogHotPatcherCommandlet,Display,TEXT("Export Patch Misstion is %s, return %d!"),bExportStatus?TEXT("Successed"):TEXT("Failure"),(int32)!bExportStatus);
	}

	if(FParse::Param(FCommandLine::Get(), TEXT("wait")))
	{
		system("pause");
	}
	
	return (int32)!bExportStatus;
}
