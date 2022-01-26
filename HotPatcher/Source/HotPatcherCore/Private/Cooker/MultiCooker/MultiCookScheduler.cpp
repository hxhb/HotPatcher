// Fill out your copyright notice in the Description page of Project Settings.


#include "Cooker/MultiCooker/MultiCookScheduler.h"
#include "Cooker/MultiCooker/FlibMultiCookerHelper.h"

TArray<FSingleCookerSettings> UMultiCookScheduler::MultiCookScheduler_Implementation(FMultiCookerSettings MultiCookerSettings,const TArray<FAssetDetail>& AllDetails)
{
	SCOPED_NAMED_EVENT_TEXT("UMultiCookScheduler::MultiCookScheduler",FColor::Red);
	int32 ProcessNumber = MultiCookerSettings.ProcessNumber;
	
	TArray<FSingleCookerSettings> AllSingleCookerSettings;

	TMap<FName,TArray<FAssetDetail>> TypeAssetDetails;
	TSet<FName> AllPackagePaths;
	for(const auto& AssetDetail:AllDetails)
	{
		TArray<FAssetDetail>& Assets = TypeAssetDetails.FindOrAdd(AssetDetail.AssetType);
		Assets.AddUnique(AssetDetail);
		AllPackagePaths.Add(FName(*UFlibAssetManageHelper::PackagePathToLongPackageName(AssetDetail.PackagePath.ToString())));
	}
	
	for(int32 index = 0;index<ProcessNumber;++index)
	{
		FSingleCookerSettings EmptySetting;
		EmptySetting.MissionID = index;
		EmptySetting.MissionName = FString::Printf(TEXT("%s_Cooker_%d"),FApp::GetProjectName(),EmptySetting.MissionID);
		EmptySetting.ShaderLibName = FString::Printf(TEXT("%s_Shader_%d"),FApp::GetProjectName(),EmptySetting.MissionID);
		EmptySetting.CookTargetPlatforms = MultiCookerSettings.CookTargetPlatforms;
		// EmptySetting.ShaderLibName = FApp::GetProjectName();
		EmptySetting.bPackageTracker = MultiCookerSettings.bPackageTracker;
		EmptySetting.ShaderOptions = MultiCookerSettings.ShaderOptions;
		EmptySetting.IoStoreSettings = MultiCookerSettings.IoStoreSettings;
		EmptySetting.bSerializeAssetRegistry = MultiCookerSettings.bSerializeAssetRegistry;
		EmptySetting.SkipCookContents = MultiCookerSettings.GetAllSkipContents();
		EmptySetting.ForceSkipClasses = MultiCookerSettings.ForceSkipClasses;
		EmptySetting.SkipLoadedAssets = AllPackagePaths;
		EmptySetting.bDisplayConfig = MultiCookerSettings.bDisplayMissionConfig;
		EmptySetting.bPreGeneratePlatformData = MultiCookerSettings.bPreGeneratePlatformData;
		EmptySetting.bWaitEachAssetCompleted = MultiCookerSettings.bWaitEachAssetCompleted;
		EmptySetting.bConcurrentSave = MultiCookerSettings.bConcurrentSave;
		EmptySetting.bAsyncLoad = MultiCookerSettings.bAsyncLoad;
		EmptySetting.bCookAdditionalAssets = false;
		EmptySetting.StorageCookedDir = FPaths::Combine(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()),TEXT("Cooked"));
		EmptySetting.StorageMetadataDir = FPaths::Combine(UFlibMultiCookerHelper::GetMultiCookerBaseDir(),EmptySetting.MissionName);
		AllSingleCookerSettings.Add(EmptySetting);
	}
	
	for(auto& TypeAssets:TypeAssetDetails)
	{
		TArray<TArray<FAssetDetail>> SplitInfo = THotPatcherTemplateHelper::SplitArray(TypeAssets.Value,ProcessNumber);
		for(int32 index = 0;index < ProcessNumber;++index)
		{
			AllSingleCookerSettings[index].CookAssets.Append(SplitInfo[index]);
		}
	}
	
	return AllSingleCookerSettings;
}