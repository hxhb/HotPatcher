// Fill out your copyright notice in the Description page of Project Settings.


#include "Cooker/MultiCooker/MultiCookScheduler_MatCulster.h"

#include "FlibHotPatcherCoreHelper.h"
#include "Cooker/MultiCooker/FlibMultiCookerHelper.h"
#include "Materials/MaterialInstance.h"

TArray<FSingleCookerSettings> UMultiCookScheduler_MatCulster::MultiCookScheduler_Implementation(FMultiCookerSettings MultiCookerSettings,const TArray<FAssetDetail>& AllDetails)
{
	SCOPED_NAMED_EVENT_TEXT("UMultiCookScheduler_MatCulster::MultiCookScheduler",FColor::Red);
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
		EmptySetting.bConcurrentSave = MultiCookerSettings.bConcurrentSave;
		EmptySetting.bAsyncLoad = MultiCookerSettings.bAsyncLoad;
		EmptySetting.bCookAdditionalAssets = false;
		EmptySetting.StorageCookedDir = FPaths::Combine(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()),TEXT("Cooked"));
		EmptySetting.StorageMetadataDir = FPaths::Combine(UFlibMultiCookerHelper::GetMultiCookerBaseDir(),EmptySetting.MissionName);
		AllSingleCookerSettings.Add(EmptySetting);
	}

	// parser Material & MaterialInstance to same cooker
	{
		FString MaterialName = *UMaterial::StaticClass()->GetName();
		
		TArray<FAssetDetail>& MaterialAsets = *TypeAssetDetails.Find(*MaterialName);
		TArray<TArray<FAssetDetail>> MaterialSplitInfo = THotPatcherTemplateHelper::SplitArray(MaterialAsets,ProcessNumber);
		TypeAssetDetails.Remove(*MaterialName);
		TArray<UClass*> MatInsClasses = UFlibHotPatcherCoreHelper::GetDerivedClasses(UMaterialInstance::StaticClass(),true,true);
		TArray<FString> MatInsClassesNames;
		
		for(auto Class:MatInsClasses)
		{
			MatInsClassesNames.AddUnique(Class->GetName());
		}
		TArray<EAssetRegistryDependencyTypeEx> RefTypes = {
			EAssetRegistryDependencyTypeEx::Hard
		};
		
		for(int32 Idx=0;Idx < ProcessNumber;++Idx)
		{
			AllSingleCookerSettings[Idx].CookAssets.Append(MaterialSplitInfo[Idx]);

			for(const auto& MaterialAsset:MaterialSplitInfo[Idx])
			{
				const TArray<FAssetDetail>& RefAssets = UFlibHotPatcherCoreHelper::GetReferenceRecursivelyByClassName(MaterialAsset,MatInsClassesNames,RefTypes);
				
				for(const auto& Asset:RefAssets)
				{
					FName AssetType = Asset.AssetType;
					if(TypeAssetDetails.Contains(AssetType))
					{
						if(TypeAssetDetails.Find(AssetType)->Contains(Asset))
						{
							AllSingleCookerSettings[Idx].CookAssets.AddUnique(Asset);
							TypeAssetDetails.Find(AssetType)->Remove(Asset);
						}
					}
				}
			}
		}
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
