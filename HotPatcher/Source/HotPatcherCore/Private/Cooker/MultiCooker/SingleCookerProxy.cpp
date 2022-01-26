#include "Cooker/MultiCooker/SingleCookerProxy.h"
#include "FlibHotPatcherCoreHelper.h"
#include "Cooker/MultiCooker/FlibMultiCookerHelper.h"
#include "ShaderCompiler.h"
#include "Async/ParallelFor.h"
#include "ShaderPatch/FlibShaderCodeLibraryHelper.h"
#include "ThreadUtils/FThreadUtils.hpp"
#include "Cooker/MultiCooker/FCookShaderCollectionProxy.h"
#include "Materials/MaterialInstance.h"
#include "Misc/ScopeExit.h"

#if WITH_PACKAGE_CONTEXT
// // engine header
#include "UObject/SavePackage.h"

#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION > 25
#include "Serialization/BulkDataManifest.h"
#endif


void USingleCookerProxy::Init(FPatcherEntitySettingBase* InSetting)
{
	Super::Init(InSetting);
	// IConsoleVariable* StreamableDelegateDelayFrames = IConsoleManager::Get().FindConsoleVariable(TEXT("s.StreamableDelegateDelayFrames"));
	// StreamableDelegateDelayFrames->Set(0);
	
#if WITH_PACKAGE_CONTEXT
	if(GetSettingObject()->bOverrideSavePackageContext)
	{
		PlatformSavePackageContexts = GetSettingObject()->PlatformSavePackageContexts;
	}
	else
	{
		PlatformSavePackageContexts = UFlibHotPatcherCoreHelper::CreatePlatformsPackageContexts(GetSettingObject()->CookTargetPlatforms,GetSettingObject()->IoStoreSettings.bIoStore);
	}
#endif
	InitShaderLibConllections();
	// cook package tracker
	if(GetSettingObject()->bPackageTracker)
	{
		PackagePathSet.PackagePaths.Append(GetSettingObject()->SkipLoadedAssets);
		for(const auto& Asset:GetSettingObject()->CookAssets)
		{
			PackagePathSet.PackagePaths.Add(*UFlibAssetManageHelper::PackagePathToLongPackageName(Asset.PackagePath.ToString()));
		}
		PackageTracker = MakeShareable(new FPackageTracker(PackagePathSet.PackagePaths));
	}
	UFlibHotPatcherCoreHelper::DeleteDirectory(GetSettingObject()->StorageMetadataDir);
}

void USingleCookerProxy::Shutdown()
{
	SCOPED_NAMED_EVENT_TCHAR(TEXT("USingleCookerProxy::Shutdown"),FColor::Red);
	WaitCookerFinished();
	ShutdowShaderLibCollections();
	
	if(PackageTracker)
	{
		FPackagePathSet AdditionalPackageSet;
		AdditionalPackageSet.PackagePaths.Append(PackageTracker->GetPendingPackageSet());
		if(AdditionalPackageSet.PackagePaths.Num())
		{
			FString OutString;
			THotPatcherTemplateHelper::TSerializeStructAsJsonString(AdditionalPackageSet,OutString);

			FFileHelper::SaveStringToFile(OutString,*FPaths::Combine(GetSettingObject()->StorageMetadataDir,TEXT("AdditionalPackageSet.json")));
		}
	}
	BulkDataManifest();
	IoStoreManifest();
	Super::Shutdown();
}

void USingleCookerProxy::DoCookMission(const TArray<FAssetDetail>& Assets)
{
	SCOPED_NAMED_EVENT_TCHAR(TEXT("USingleCookerProxy::DoCookMission"),FColor::Red);
	const FCookActionResultEvent PackageSavedCallback = [this](const FSoftObjectPath& PackagePath,ETargetPlatform Platform,ESavePackageResult Result)
	{
		OnAssetCooked.Broadcast(PackagePath,Platform,Result);
	};

	const FCookActionEvent CookAssetBegin = [this](const FSoftObjectPath& PackagePath,ETargetPlatform Platform)
	{
		OnCookAssetBegin.Broadcast(PackagePath,Platform);
	};
	
	FCookCluster DefaultCluser;
	DefaultCluser.AssetDetails = Assets;
	
	DefaultCluser.Platforms = GetSettingObject()->CookTargetPlatforms;
	DefaultCluser.bPreGeneratePlatformData = GetSettingObject()->bPreGeneratePlatformData;

	DefaultCluser.CookActionCallback.OnAssetCooked = PackageSavedCallback;
	DefaultCluser.CookActionCallback.OnCookBegin = CookAssetBegin;

	auto GetPackageTrackerClusterLambda = [&]()->FCookCluster
	{
		FCookCluster PackageTrackerCluster;
	
		PackageTrackerCluster.Platforms = GetSettingObject()->CookTargetPlatforms;
		PackageTrackerCluster.bPreGeneratePlatformData = GetSettingObject()->bPreGeneratePlatformData;

		PackageTrackerCluster.CookActionCallback.OnAssetCooked = PackageSavedCallback;
		PackageTrackerCluster.CookActionCallback.OnCookBegin = CookAssetBegin;
		
		if(PackageTracker && GetSettingObject()->bCookAdditionalAssets)
		{
			PackageTrackerCluster.AssetDetails.Empty();
			for(FName PackagePath:PackageTracker->GetPendingPackageSet())
			{
				
				PackageTrackerCluster.AssetDetails.Emplace(UFlibAssetManageHelper::GetAssetDetailByPackageName(PackagePath.ToString()));
			}
		}
		return PackageTrackerCluster;
	};
	
	if(DefaultCluser.bPreGeneratePlatformData)
	{
		PreGeneratePlatformData(DefaultCluser);
		if(PackageTracker && GetSettingObject()->bCookAdditionalAssets)
		{
			PreGeneratePlatformData(GetPackageTrackerClusterLambda());
		}
	}
	
	CookCluster(DefaultCluser);
	if(PackageTracker && GetSettingObject()->bCookAdditionalAssets)
	{
		// cook all additional assets
		CookCluster(GetPackageTrackerClusterLambda());
	}
}

void USingleCookerProxy::BulkDataManifest()
{
	// save bulk data manifest
	for(auto& Platform:GetSettingObject()->CookTargetPlatforms)
	{
		if(GetSettingObject()->IoStoreSettings.bStorageBulkDataInfo)
		{
#if WITH_PACKAGE_CONTEXT
			UFlibHotPatcherCoreHelper::SavePlatformBulkDataManifest(GetPlatformSavePackageContexts(),Platform);
#endif
		}
	}
}

void USingleCookerProxy::IoStoreManifest()
{
	if(GetSettingObject()->IoStoreSettings.bIoStore)
	{
		TSet<ETargetPlatform> Platforms;
		for(const auto& Cluser:CookClusters)
		{
			for(auto Platform:Cluser.Platforms)
			{
				Platforms.Add(Platform);
			}
		}

		for(auto Platform:Platforms)
		{
			FString PlatformName = THotPatcherTemplateHelper::GetEnumNameByValue(Platform);
			TimeRecorder StorageCookOpenOrderTR(FString::Printf(TEXT("Storage CookOpenOrder.txt for %s, time:"),*PlatformName));
			struct CookOpenOrder
			{
				CookOpenOrder()=default;
				CookOpenOrder(const FString& InPath,int32 InOrder):uasset_relative_path(InPath),order(InOrder){}
				FString uasset_relative_path;
				int32 order;
			};
			auto MakeCookOpenOrder = [](const TArray<FName>& Assets)->TArray<CookOpenOrder>
			{
				TArray<CookOpenOrder> result;
				TArray<FAssetData> AssetsData;
				TArray<FString> AssetPackagePaths;
				for (auto AssetPackagePath : Assets)
				{
					FSoftObjectPath ObjectPath{ AssetPackagePath };
					TArray<FAssetData> AssetData;
					UFlibAssetManageHelper::GetSpecifyAssetData(ObjectPath.GetLongPackageName(),AssetData,true);
					AssetsData.Append(AssetData);
				}

				// UFlibAssetManageHelper::GetAssetsData(AssetPackagePaths,AssetsData);
					
				for(int32 index=0;index<AssetsData.Num();++index)
				{
					UPackage* Package = AssetsData[index].GetPackage();
					FString LocalPath;
					const FString* PackageExtension = Package->ContainsMap() ? &FPackageName::GetMapPackageExtension() : &FPackageName::GetAssetPackageExtension();
					FPackageName::TryConvertLongPackageNameToFilename(AssetsData[index].PackageName.ToString(), LocalPath, *PackageExtension);
					result.Emplace(LocalPath,index);
				}
				return result;
			};
			TArray<CookOpenOrder> CookOpenOrders = MakeCookOpenOrder(GetPlatformCookAssetOrders(Platform));

			auto SaveCookOpenOrder = [](const TArray<CookOpenOrder>& CookOpenOrders,const FString& File)
			{
				TArray<FString> result;
				for(const auto& OrderFile:CookOpenOrders)
				{
					result.Emplace(FString::Printf(TEXT("\"%s\" %d"),*OrderFile.uasset_relative_path,OrderFile.order));
				}
				FFileHelper::SaveStringArrayToFile(result,*FPaths::ConvertRelativePathToFull(File));
			};
			SaveCookOpenOrder(CookOpenOrders,FPaths::Combine(GetSettingObject()->StorageMetadataDir,GetSettingObject()->MissionName,PlatformName,TEXT("CookOpenOrder.txt")));
		}
	}
}

void USingleCookerProxy::InitShaderLibConllections()
{
	FString SavePath = GetSettingObject()->StorageMetadataDir;
	SCOPED_NAMED_EVENT_TCHAR(TEXT("USingleCookerProxy::InitShaderLibConllections"),FColor::Red);
	PlatformCookShaderCollection = UFlibMultiCookerHelper::CreateCookShaderCollectionProxyByPlatform(
		GetSettingObject()->ShaderLibName,
		GetSettingObject()->CookTargetPlatforms,
		GetSettingObject()->ShaderOptions.bSharedShaderLibrary,
		GetSettingObject()->ShaderOptions.bNativeShader,
		true,
		SavePath,
		true
	);
	
	if(PlatformCookShaderCollection.IsValid())
	{
		PlatformCookShaderCollection->Init();
	}
}

void USingleCookerProxy::ShutdowShaderLibCollections()
{
	SCOPED_NAMED_EVENT_TCHAR(TEXT("USingleCookerProxy::ShutdowShaderLibCollections"),FColor::Red);
	if(GetSettingObject()->ShaderOptions.bSharedShaderLibrary)
	{
		if(PlatformCookShaderCollection.IsValid())
		{
			PlatformCookShaderCollection->Shutdown();
		}
	}
}

bool USingleCookerProxy::DoExport()
{
	SCOPED_NAMED_EVENT_TCHAR(TEXT("USingleCookerProxy::DoExport"),FColor::Red);
	GetCookFailedAssetsCollection().MissionName = GetSettingObject()->MissionName;
	GetCookFailedAssetsCollection().MissionID = GetSettingObject()->MissionID;
	GetCookFailedAssetsCollection().CookFailedAssets.Empty();

	OnAssetCooked.AddUObject(this,&USingleCookerProxy::OnAssetCookedHandle);
	
	DoCookMission(GetSettingObject()->CookAssets);
	
	if(HasError())
	{
		FString FailedJsonString;
		THotPatcherTemplateHelper::TSerializeStructAsJsonString(GetCookFailedAssetsCollection(),FailedJsonString);
		UE_LOG(LogHotPatcher,Warning,TEXT("Single Cooker Proxy %s:\n%s"),*GetSettingObject()->MissionName,*FailedJsonString);
		FString SaveTo = UFlibMultiCookerHelper::GetCookerProcFailedResultPath(GetSettingObject()->StorageMetadataDir,GetSettingObject()->MissionName,GetSettingObject()->MissionID);
		FFileHelper::SaveStringToFile(FailedJsonString,*SaveTo);
	}
	
	return !HasError();
}
FName WorldType = TEXT("World");


void USingleCookerProxy::CookClusterSync(const FCookCluster& CookCluster)
{
	SCOPED_NAMED_EVENT_TCHAR(TEXT("USingleCookerProxy::CookClusterSync"),FColor::Red);
	
	TArray<ITargetPlatform*> TargetPlatforms = UFlibHotPatcherCoreHelper::GetTargetPlatformsByNames(CookCluster.Platforms);

	if(GetSettingObject()->bConcurrentSave)
	{
		FString CookBaseDir = GetSettingObject()->StorageCookedDir;
		TMap<FString, FSavePackageContext*> SavePackageContextsNameMapping = GetPlatformSavePackageContextsNameMapping();
		
		TArray<UPackage*> AllPackages = UFlibAssetManageHelper::LoadPackagesForCooking(CookCluster.AsSoftObjectPaths(),GetSettingObject()->bConcurrentSave);
		
		GIsSavingPackage = true;
		TMap<FName,TMap<FName,FString>> PackageCookedSavePaths;
		for(const auto& Package:AllPackages)
		{
			FName PackagePathName = *Package->GetPathName();
			PackageCookedSavePaths.Add(PackagePathName,TMap<FName,FString>{});
			for(auto Platform:TargetPlatforms)
			{
				FString CookedSavePath = UFlibHotPatcherCoreHelper::GetAssetCookedSavePath(CookBaseDir,PackagePathName.ToString(), Platform->PlatformName());
				PackageCookedSavePaths.Find(PackagePathName)->Add(*Platform->PlatformName(),CookedSavePath);
			}
		}
		
		ParallelFor(AllPackages.Num(), [=](int32 AssetIndex)
		{
			UFlibHotPatcherCoreHelper::CookPackage(
				AllPackages[AssetIndex],
				TargetPlatforms,
				CookCluster.CookActionCallback,
				SavePackageContextsNameMapping,
				*PackageCookedSavePaths.Find(*AllPackages[AssetIndex]->GetPathName()),
				true);
		},!GetSettingObject()->bConcurrentSave);
		GIsSavingPackage = false;
	}
	else
	{
		UFlibHotPatcherCoreHelper::CookAssets(CookCluster.AsSoftObjectPaths(),CookCluster.Platforms,CookCluster.CookActionCallback
#if WITH_PACKAGE_CONTEXT
									,GetPlatformSavePackageContextsRaw()
#endif
									,GetSettingObject()->StorageCookedDir
);
	}
	
}

void USingleCookerProxy::PreGeneratePlatformData(const FCookCluster& CookCluster)
{
	TArray<ITargetPlatform*> TargetPlatforms = UFlibHotPatcherCoreHelper::GetTargetPlatformsByNames(CookCluster.Platforms);
	bool bConcurrentSave = GetSettingObject()->bConcurrentSave;
	TSet<UObject*> ProcessedObjects;
	TSet<UObject*> PendingCachePlatformDataObjects;
	if(CookCluster.bPreGeneratePlatformData)
	{
		FCookCluster Cluster = CookCluster;

		auto PreCachePlatformDataForPackages = [&](TArray<UPackage*>& PreCachePackages,const FString& Display)
		{
			uint32 TotalPackagesNum = PreCachePackages.Num();
			for(int32 Index = PreCachePackages.Num() - 1 ;Index >= 0;--Index)
			{
				UPackage* CurrentPackage = PreCachePackages[Index];
				UE_LOG(LogHotPatcher,Display,TEXT("PreCache %s, pending %d total %d"),*CurrentPackage->GetPathName(),PreCachePackages.Num(),TotalPackagesNum);
				
				UFlibHotPatcherCoreHelper::CacheForCookedPlatformData(
					TArray<UPackage*>{CurrentPackage},
					TargetPlatforms,
					ProcessedObjects,
					PendingCachePlatformDataObjects,
					bConcurrentSave,
					GetSettingObject()->bWaitEachAssetCompleted);
				PreCachePackages.RemoveAtSwap(Index,1,false);
			}
			UFlibHotPatcherCoreHelper::WaitObjectsCachePlatformDataComplete(ProcessedObjects,PendingCachePlatformDataObjects,TargetPlatforms);
			GEngine->ForceGarbageCollection();
		};
		
		for(auto Class:GetPreCacheClasses())
		{
			FString DisplayStr = FString::Printf(TEXT("PreCache DDC For %s"),*Class->GetName());
			FScopedNamedEvent CacheClassEvent(FColor::Red,*DisplayStr);
			UE_LOG(LogHotPatcher,Log,TEXT("%s"),*DisplayStr);
			TArray<FAssetDetail> ClassAssetDetails = THotPatcherTemplateHelper::GetArrayBySrcWithCondition<FAssetDetail>(Cluster.AssetDetails,[&](FAssetDetail AssetDetail)->bool
				{
					return AssetDetail.AssetType.IsEqual(*Class->GetName());
				},true);
			TArray<FSoftObjectPath> ObjectPaths;
			for(const auto& AssetDetail:ClassAssetDetails)
			{
				ObjectPaths.Emplace(AssetDetail.PackagePath);
			}
			
			TArray<UPackage*> PreCachePackages = UFlibAssetManageHelper::LoadPackagesForCooking(ObjectPaths,GetSettingObject()->bConcurrentSave);
			PreCachePlatformDataForPackages(PreCachePackages,Class->GetName());
		}
		TArray<UPackage*> OthesPackages = UFlibAssetManageHelper::LoadPackagesForCooking(Cluster.AsSoftObjectPaths(),GetSettingObject()->bConcurrentSave);
		PreCachePlatformDataForPackages(OthesPackages,TEXT("Global"));
	}
}

void USingleCookerProxy::CookCluster(const FCookCluster& CookCluster)
{
	SCOPED_NAMED_EVENT_TCHAR(TEXT("USingleCookerProxy::CookCluster"),FColor::Red);
	TArray<ITargetPlatform*> CookPlatfotms = UFlibHotPatcherCoreHelper::GetTargetPlatformsByNames(CookCluster.Platforms);
	
	{
		SCOPED_NAMED_EVENT_TCHAR(TEXT("Delete Old Cooked Files"),FColor::Red);
		FString CookBaseDir = GetSettingObject()->StorageCookedDir;
		TArray<FString> PaddingDeleteFiles;
		for(const auto& Asset:CookCluster.AsSoftObjectPaths())
		{
			FString PackageName = Asset.GetLongPackageName();
			for(const auto& TargetPlatform:CookPlatfotms)
			{
				FString CookedSavePath = UFlibHotPatcherCoreHelper::GetAssetCookedSavePath(CookBaseDir,PackageName, TargetPlatform->PlatformName());
				PaddingDeleteFiles.AddUnique(CookedSavePath);
			}
		}
	
		ParallelFor(PaddingDeleteFiles.Num(),[PaddingDeleteFiles](int32 Index)
		{
			FString FileName = PaddingDeleteFiles[Index];
			if(!FileName.IsEmpty() && FPaths::FileExists(FileName))
			{
				IFileManager::Get().Delete(*FileName,true,true,true);
			}
		},false);
	}
	
	CookClusterSync(CookCluster);
}

void USingleCookerProxy::AddCluster(const FCookCluster& CookCluster)
{
	for(auto Platform:CookCluster.Platforms)
	{
		if(!GetPlatformSavePackageContexts().Contains(Platform))
		{
			GetPlatformSavePackageContexts().Append(UFlibHotPatcherCoreHelper::CreatePlatformsPackageContexts(TArray<ETargetPlatform>{Platform},GetSettingObject()->IoStoreSettings.bIoStore));
		}
	}
	for(const auto& AssetDetail:CookCluster.AssetDetails)
	{
		FSoftObjectPath SoftObjectPath(AssetDetail.PackagePath);
		FName AssetType = UFlibAssetManageHelper::GetAssetType(SoftObjectPath);
		if(!AssetType.IsNone())
		{
			GetAssetTypeMapping().Add(SoftObjectPath.GetAssetPathName(),AssetType);
		}
	}
	CookClusters.Add(CookCluster);
}


bool USingleCookerProxy::HasError()
{
	SCOPED_NAMED_EVENT_TCHAR(TEXT("USingleCookerProxy::HasError"),FColor::Red);
	TArray<ETargetPlatform> TargetPlatforms;
	GetCookFailedAssetsCollection().CookFailedAssets.GetKeys(TargetPlatforms);
	return !!TargetPlatforms.Num();
}

void USingleCookerProxy::OnAssetCookedHandle(const FSoftObjectPath& PackagePath, ETargetPlatform Platform,
	ESavePackageResult Result)
{
	FScopeLock Lock(&SynchronizationObject);
	if(Result == ESavePackageResult::Success)
	{
		GetPaendingCookAssetsSet().Remove(PackagePath.GetAssetPathName());
		MarkAssetCooked(PackagePath,Platform);
	}
	else
	{
		SCOPED_NAMED_EVENT_TCHAR(TEXT("USingleCookerProxy::OnCookAssetFailed"),FColor::Red);
		FString PlatformName = THotPatcherTemplateHelper::GetEnumNameByValue(Platform);
		UE_LOG(LogHotPatcher,Warning,TEXT("Cook %s for %s Failed!"),*PackagePath.GetAssetPathString(),*PlatformName);
		GetPaendingCookAssetsSet().Remove(PackagePath.GetAssetPathName());
	}
}

bool USingleCookerProxy::IsFinsihed()
{
	return !GetPaendingCookAssetsSet().Num();
}

void USingleCookerProxy::WaitCookerFinished()
{
	SCOPED_NAMED_EVENT_TCHAR(TEXT("USingleCookerProxy::WaitCookerFinished"),FColor::Red);
	// Wait for all shaders to finish compiling
	UFlibShaderCodeLibraryHelper::WaitShaderCompilingComplete();
	UFlibHotPatcherCoreHelper::WaitForAsyncFileWrites();
}

TMap<ETargetPlatform, FSavePackageContext*> USingleCookerProxy::GetPlatformSavePackageContextsRaw()
{
	FScopeLock Lock(&SynchronizationObject);
	SCOPED_NAMED_EVENT_TCHAR(TEXT("USingleCookerProxy::GetPlatformSavePackageContextsRaw"),FColor::Red);
	TMap<ETargetPlatform,FSavePackageContext*> result;
	TArray<ETargetPlatform> Keys;
	GetPlatformSavePackageContexts().GetKeys(Keys);
	for(const auto& Key:Keys)
	{
		result.Add(Key,GetPlatformSavePackageContexts().Find(Key)->Get());
	}
	return result;
}


TMap<FString, FSavePackageContext*> USingleCookerProxy::GetPlatformSavePackageContextsNameMapping()
{
	FScopeLock Lock(&SynchronizationObject);
	SCOPED_NAMED_EVENT_TCHAR(TEXT("USingleCookerProxy::GetPlatformSavePackageContextsNameMapping"),FColor::Red);
	TMap<FString,FSavePackageContext*> result;
	TArray<ETargetPlatform> Keys;
	GetPlatformSavePackageContexts().GetKeys(Keys);
	for(const auto& Key:Keys)
	{
		result.Add(THotPatcherTemplateHelper::GetEnumNameByValue(Key),GetPlatformSavePackageContexts().Find(Key)->Get());
	}
	return result;
}

TArray<FName>& USingleCookerProxy::GetPlatformCookAssetOrders(ETargetPlatform Platform)
{
	FScopeLock Lock(&SynchronizationObject);
	return CookAssetOrders.FindOrAdd(Platform);
}

TSet<FName> USingleCookerProxy::GetAdditionalAssets()
{
	if(GetSettingObject()->bPackageTracker && PackageTracker.IsValid())
	{
		return PackageTracker->GetPendingPackageSet();
	}
	return TSet<FName>{};
}

TArray<UClass*> USingleCookerProxy::GetPreCacheClasses() const
{
	TArray<UClass*> Classes;

	TSet<UClass*> ParentClasses = {
		UTexture::StaticClass(),
		UMaterialInterface::StaticClass(),
		UMaterialFunctionInterface::StaticClass(),
		UMaterialExpression::StaticClass()
	};

	for(auto& ParentClass:ParentClasses)
	{
		Classes.Append(UFlibHotPatcherCoreHelper::GetDerivedClasses(ParentClass,true,true));
	}
	
	return Classes;
}

void USingleCookerProxy::MarkAssetCooked(const FSoftObjectPath& PackagePath, ETargetPlatform Platform)
{
	FScopeLock Lock(&SynchronizationObject);
	GetPlatformCookAssetOrders(Platform).Add(PackagePath.GetAssetPathName());
}


#endif
