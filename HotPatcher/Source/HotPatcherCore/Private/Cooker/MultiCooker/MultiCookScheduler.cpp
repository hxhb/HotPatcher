// Fill out your copyright notice in the Description page of Project Settings.


#include "Cooker/MultiCooker/MultiCookScheduler.h"

#include "FlibHotPatcherCoreHelper.h"
#include "Cooker/MultiCooker/FlibMultiCookerHelper.h"

TArray<FSingleCookerSettings> UMultiCookScheduler::MultiCookScheduler_Implementation(FMultiCookerSettings& MultiCookerSettings, TArray<FAssetDetail>& AllDetails)
{
	SCOPED_NAMED_EVENT_TEXT("UMultiCookScheduler::MultiCookScheduler",FColor::Red);
	int32 ProcessNumber = MultiCookerSettings.ProcessNumber;
	
	TArray<FSingleCookerSettings> AllSingleCookerSettings;

	TMap<FName,TArray<FAssetDetail>> TypeAssetDetails;
	TSet<FName> AllPackagePaths;
	
	TSet<UClass*> Classes;
	{
		for(auto& ParentClass:MultiCookerSettings.GetCookClasses().CookClasses)
		{
			Classes.Append(UFlibHotPatcherCoreHelper::GetDerivedClasses(ParentClass,MultiCookerSettings.GetCookClasses().bContainChildClasses,true));
		}
	}

	auto IsMatchClass = [&](FName ClassName)->bool
	{
		bool bIsMatchedClass = false;
		for(const auto ExcludeClass:Classes)
		{
			if(ClassName.IsEqual(*ExcludeClass->GetName()))
			{
				bIsMatchedClass = true;
				break;
			}
		}
		return bIsMatchedClass;
	};
	
	// for(const auto& AssetDetail:AllDetails)
	for(int32 index = 0;index < AllDetails.Num();)
	{
		bool bIsMatchedClass = IsMatchClass(AllDetails[index].AssetType);
		if(MultiCookerSettings.GetCookClasses().CookClassesMode == ECookClassesMode::SKIP_CLASSES && bIsMatchedClass && !!Classes.Num() ||
		 (MultiCookerSettings.GetCookClasses().CookClassesMode == ECookClassesMode::COOK_CLASSES_ONLY) && !bIsMatchedClass && !!Classes.Num()
		)
		{
			AllDetails.RemoveAt(index);
			// UE_LOG(LogHotPatcher,Display,TEXT("Ignore %s (match ignore type %s)"),*AssetDetail.PackagePath.ToString(),*AssetDetail.AssetType.ToString());
			continue;
		}
		
		TArray<FAssetDetail>& Assets = TypeAssetDetails.FindOrAdd(AllDetails[index].AssetType);
		Assets.AddUnique(AllDetails[index]);
		AllPackagePaths.Add(FName(*UFlibAssetManageHelper::PackagePathToLongPackageName(AllDetails[index].PackagePath.ToString())));
		++index;
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
		EmptySetting.ShaderOptions = MultiCookerSettings.GetCookMetadata().ShaderOptions;
		EmptySetting.IoStoreSettings = MultiCookerSettings.GetCookMetadata().IoStoreSettings;
		EmptySetting.bSerializeAssetRegistry = MultiCookerSettings.GetCookMetadata().bSerializeAssetRegistry;
		EmptySetting.SkipCookContents = MultiCookerSettings.GetAllSkipContents();
		EmptySetting.ForceSkipClasses = MultiCookerSettings.ForceSkipClasses;
		EmptySetting.SkipLoadedAssets = AllPackagePaths;
		EmptySetting.bDisplayConfig = MultiCookerSettings.GetCookProfiling().bDisplayMissionConfig;
		EmptySetting.bPreGeneratePlatformData = MultiCookerSettings.GetCookOptimize().bPreGeneratePlatformData;
		EmptySetting.bWaitEachAssetCompleted = MultiCookerSettings.GetCookOptimize().bWaitEachAssetCompleted;
		EmptySetting.bConcurrentSave = MultiCookerSettings.GetCookOptimize().bConcurrentSave;
		EmptySetting.bAsyncLoad = MultiCookerSettings.GetCookOptimize().bAsyncLoad;
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