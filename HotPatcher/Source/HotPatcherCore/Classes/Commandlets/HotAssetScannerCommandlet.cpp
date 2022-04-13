#include "HotAssetScannerCommandlet.h"
#include "ShaderPatch/FExportShaderPatchSettings.h"
#include "CommandletHelper.h"
// engine header
#include "CoreMinimal.h"
#include "Async/ParallelFor.h"
#include "Misc/FileHelper.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "ShaderPatch/FExportShaderPatchSettings.h"
#include "ShaderPatch/ShaderPatchProxy.h"
#include "BaseTypes/FPackageTracker.h"
DEFINE_LOG_CATEGORY(LogHotAssetScannerCommandlet);

static const bool bNoDDC = FParse::Param(FCommandLine::Get(), TEXT("NoDDC"));

struct FObjectTrackerTagCleaner:public FPackageTrackerBase
{
	virtual void NotifyUObjectCreated(const UObjectBase* Object, int32 Index) override
	{
		auto ObjectIns = const_cast<UObject*>(static_cast<const UObject*>(Object));
		if(ObjectIns && bNoDDC)
		{
			ObjectIns->ClearFlags(RF_NeedPostLoad);
			ObjectIns->ClearFlags(RF_NeedPostLoadSubobjects);
		}
	}
};

void UHotAssetScannerCommandlet::MaybeMarkPackageAsAlreadyLoaded(UPackage* Package)
{
	Package->SetPackageFlags(PKG_ReloadingForCooker);
	Package->SetPackageFlags(PKG_FilterEditorOnly);
}

int32 UHotAssetScannerCommandlet::Main(const FString& Params)
{
	SCOPED_NAMED_EVENT_TEXT("UHotAssetScannerCommandlet::Main",FColor::Red);
	Super::Main(Params);
	// for Object Create Tracking,Optimize Asset searching, dont execute UObject::PostLoad
	TSharedPtr<FObjectTrackerTagCleaner> ObjectTrackerTagCleaner = MakeShareable(new FObjectTrackerTagCleaner);
	
	UE_LOG(LogHotAssetScannerCommandlet, Display, TEXT("UHotAssetScannerCommandlet::Main"));

	FString config_path;
	bool bStatus = FParse::Value(*Params, *FString(PATCHER_CONFIG_PARAM_NAME).ToLower(), config_path);
	if (!bStatus)
	{
		UE_LOG(LogHotAssetScannerCommandlet, Warning, TEXT("not -config=xxxx.json params."));
		return -1;
	}

	if (bStatus && !FPaths::FileExists(config_path))
	{
		UE_LOG(LogHotAssetScannerCommandlet, Error, TEXT("cofnig file %s not exists."), *config_path);
		return -1;
	}
	
	if(IsRunningCommandlet())
	{
		// load asset registry
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		AssetRegistryModule.Get().SearchAllAssets(true);
	}
	
	if (bNoDDC)
	{
		FCoreUObjectDelegates::PackageCreatedForLoad.AddUObject(this,&UHotAssetScannerCommandlet::MaybeMarkPackageAsAlreadyLoaded);
	}
	
	TSharedPtr<FAssetScanConfig> AssetScanConfig = MakeShareable(new FAssetScanConfig);
	
	FString JsonContent;
	if (FPaths::FileExists(config_path) && FFileHelper::LoadFileToString(JsonContent, *config_path))
	{
		THotPatcherTemplateHelper::TDeserializeJsonStringAsStruct(JsonContent,*AssetScanConfig);
	}

	TMap<FString, FString> KeyValues = THotPatcherTemplateHelper::GetCommandLineParamsMap(Params);
	THotPatcherTemplateHelper::ReplaceProperty(*AssetScanConfig, KeyValues);
	
	FString FinalConfig;
	THotPatcherTemplateHelper::TSerializeStructAsJsonString(*AssetScanConfig,FinalConfig);
	UE_LOG(LogHotAssetScannerCommandlet, Display, TEXT("%s"), *FinalConfig);
	
	FHotPatcherVersion CurrentVersion;
	{
		CurrentVersion.VersionId = TEXT("HotAssetScanner");
		CurrentVersion.Date = FDateTime::UtcNow().ToString();
		CurrentVersion.BaseVersionId = TEXT("");
		UFlibPatchParserHelper::RunAssetScanner(*AssetScanConfig,CurrentVersion);
	}
	
	FString SearchResult;
	FString SaveSearchResultPath = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(),FString::Printf(TEXT("HotPatcher/MultiCooker/%s/SearchCookerAssets.json"),FApp::GetProjectName())));
	if(THotPatcherTemplateHelper::TSerializeStructAsJsonString(CurrentVersion,SearchResult))
	{
		FFileHelper::SaveStringToFile(SearchResult,*SaveSearchResultPath);
	}
	
	UE_LOG(LogHotAssetScannerCommandlet,Display,TEXT("HotAssetScanner Misstion is Finished!"));
	
	if(FParse::Param(FCommandLine::Get(), TEXT("wait")))
	{
		system("pause");
	}
	
	return 0;
}
