#include "Cooker/MultiCooker/SingleCookerProxy.h"
#include "FlibHotPatcherCoreHelper.h"
#include "HotPatcherCore.h"
#include "Cooker/MultiCooker/FlibHotCookerHelper.h"
#include "ShaderCompiler.h"
#include "Async/ParallelFor.h"
#include "ShaderPatch/FlibShaderCodeLibraryHelper.h"
#include "ThreadUtils/FThreadUtils.hpp"
#include "Cooker/MultiCooker/FCookShaderCollectionProxy.h"
#include "Engine/Engine.h"
#include "Engine/Texture.h"
#include "Materials/MaterialInstance.h"
#include "Misc/ScopeExit.h"
#include "Engine/AssetManager.h"
#include "LevelSequence/Public/LevelSequence.h"
#include "Particles/ParticleSystem.h"
#if WITH_PACKAGE_CONTEXT
// // engine header
#include "UObject/SavePackage.h"
#endif

#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION > 25
#include "Serialization/BulkDataManifest.h"
#endif


void USingleCookerProxy::Init(FPatcherEntitySettingBase* InSetting)
{
	SCOPED_NAMED_EVENT_TEXT("Init",FColor::Red);
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
		// all cooker assets
		ExixtPackagePathSet.PackagePaths.Append(GetSettingObject()->SkipLoadedAssets);
		// current cooker assets
		ExixtPackagePathSet.PackagePaths.Append(GetCookerAssets());
		PackageTracker = MakeShareable(new FPackageTracker(ExixtPackagePathSet.PackagePaths));
	}
	UFlibHotPatcherCoreHelper::DeleteDirectory(GetSettingObject()->GetStorageMetadataAbsDir());
	
	GetCookFailedAssetsCollection().MissionName = GetSettingObject()->MissionName;
	GetCookFailedAssetsCollection().MissionID = GetSettingObject()->MissionID;
	GetCookFailedAssetsCollection().CookFailedAssets.Empty();
	OnAssetCooked.AddUObject(this,&USingleCookerProxy::OnAssetCookedHandle);
}

void USingleCookerProxy::CreateCookQueue()
{
	SCOPED_NAMED_EVENT_TEXT("CreateCookQueue",FColor::Red);

	FString DumpCookerInfo;
	DumpCookerInfo.Append(TEXT("-----------------------------Dump Cooker-----------------------------"));
	DumpCookerInfo.Append(FString::Printf(TEXT("\tTotal Asset: %d\n"),GlobalCluser.AssetDetails.Num()));
	DumpCookerInfo.Append(FString::Printf(TEXT("\tbForceCookInOneFrame: %s\n"),GetSettingObject()->bForceCookInOneFrame ? TEXT("true"):TEXT("false")));
	FString PlatformsStr;
	for(auto Platform:GlobalCluser.Platforms)
	{
		PlatformsStr += THotPatcherTemplateHelper::GetEnumNameByValue(Platform);
		PlatformsStr.Append(TEXT(","));
	}
	PlatformsStr.RemoveFromEnd(TEXT(","));
	DumpCookerInfo.Append(FString::Printf(TEXT("\tTarget Platforms: %s\n"),*PlatformsStr));
	
	if(GetSettingObject()->bForceCookInOneFrame)
	{
		CookCluserQueue.Enqueue(GlobalCluser);
		return;
	}
	else
	{
		int32 NumberOfAssetsPerFrame = GetSettingObject()->GetNumberOfAssetsPerFrame();
		for(auto Class:GetPreCacheClasses())
		{
			TArray<FAssetDetail> ObjectAssets = UFlibAssetManageHelper::GetAssetDetailsByClass(GlobalCluser.AssetDetails,Class,true);
			if(ObjectAssets.Num())
			{
				DumpCookerInfo.Append(FString::Printf(TEXT("\t%s -- %d\n"),*Class->GetName(),ObjectAssets.Num()));
			}
			while(ObjectAssets.Num())
			{
				int32 ClusterAssetNum = NumberOfAssetsPerFrame < 1 ? ObjectAssets.Num() : NumberOfAssetsPerFrame;
				int32 NewClusterAssetNum = FMath::Min(ClusterAssetNum,ObjectAssets.Num());
			
				TArray<FAssetDetail> CulsterObjectAssets(ObjectAssets.GetData(),NewClusterAssetNum);
				FCookCluster NewCluster;
				NewCluster.AssetDetails = std::move(CulsterObjectAssets);
				ObjectAssets.RemoveAt(0,NewClusterAssetNum);
				NewCluster.Platforms = GetSettingObject()->CookTargetPlatforms;
				NewCluster.bPreGeneratePlatformData = GetSettingObject()->bPreGeneratePlatformData;

				NewCluster.CookActionCallback.OnAssetCooked = GetOnPackageSavedCallback();
				NewCluster.CookActionCallback.OnCookBegin = GetOnCookAssetBeginCallback();
				CookCluserQueue.Enqueue(NewCluster);
			}
		}

		if(GlobalCluser.AssetDetails.Num())
		{
			CookCluserQueue.Enqueue(GlobalCluser);
			DumpCookerInfo.Append(FString::Printf(TEXT("\tOther Assets -- %d\n"),GlobalCluser.AssetDetails.Num()));
		}
	}
	DumpCookerInfo.Append(TEXT("---------------------------------------------------------------------"));
	UE_LOG(LogHotPatcher,Display,TEXT("%s"),*DumpCookerInfo);
}

void USingleCookerProxy::CleanClusterCachedPlatformData(const FCookCluster& CookCluster)
{
	SCOPED_NAMED_EVENT_TEXT("CleanClusterCachedPlatformData",FColor::Red);
	TArray<ITargetPlatform*> TargetPlatforms = UFlibHotPatcherCoreHelper::GetTargetPlatformsByNames(CookCluster.Platforms);
	TArray<UPackage*> PreCachePackages = UFlibAssetManageHelper::LoadPackagesForCooking(CookCluster.AsSoftObjectPaths(),GetSettingObject()->bConcurrentSave);
	for(auto Package:PreCachePackages)
	{
		TArray<UObject*> ExportMap;
		GetObjectsWithOuter(Package,ExportMap,true);
		for(const auto& ExportObj:ExportMap)
		{
#if ENGINE_MINOR_VERSION < 26
			FScopedNamedEvent CacheExportEvent(FColor::Red,*FString::Printf(TEXT("%s"),*ExportObj->GetName()));
#endif
			if (ExportObj->HasAnyFlags(RF_Transient))
			{
				// UE_LOG(LogHotPatcherCoreHelper, Display, TEXT("%s is PreCached."),*ExportObj->GetFullName());
				continue;
			}
			ExportObj->ClearAllCachedCookedPlatformData();
		}
	}
}

void USingleCookerProxy::PreGeneratePlatformData(const FCookCluster& CookCluster)
{
	SCOPED_NAMED_EVENT_TEXT("PreGeneratePlatformData",FColor::Red);
	TArray<ITargetPlatform*> TargetPlatforms = UFlibHotPatcherCoreHelper::GetTargetPlatformsByNames(CookCluster.Platforms);
	bool bConcurrentSave = GetSettingObject()->bConcurrentSave;
	TSet<UObject*> ProcessedObjects;
	TSet<UObject*> PendingCachePlatformDataObjects;

	auto PreCachePlatformDataForPackages = [&](TArray<UPackage*>& PreCachePackages)
	{
		uint32 TotalPackagesNum = PreCachePackages.Num();
		for(int32 Index = PreCachePackages.Num() - 1 ;Index >= 0;--Index)
		{
			UPackage* CurrentPackage = PreCachePackages[Index];
			if(GCookLog)
			{
				UE_LOG(LogHotPatcher,Log,TEXT("PreCache %s, pending %d total %d"),*CurrentPackage->GetPathName(),PreCachePackages.Num()-1,TotalPackagesNum);
			}
			
			UFlibHotPatcherCoreHelper::CacheForCookedPlatformData(
				TArray<UPackage*>{CurrentPackage},
				TargetPlatforms,
				ProcessedObjects,
				PendingCachePlatformDataObjects,
				bConcurrentSave,
				GetSettingObject()->bWaitEachAssetCompleted);
			PreCachePackages.RemoveAtSwap(Index,1,false);
		}
	};
	
	TArray<UPackage*> PreCachePackages = UFlibAssetManageHelper::LoadPackagesForCooking(CookCluster.AsSoftObjectPaths(),GetSettingObject()->bConcurrentSave);
	
	if(CookCluster.bPreGeneratePlatformData)
	{
		PreCachePlatformDataForPackages(PreCachePackages);
	}
	UFlibHotPatcherCoreHelper::WaitObjectsCachePlatformDataComplete(ProcessedObjects,PendingCachePlatformDataObjects,TargetPlatforms);
}

void USingleCookerProxy::DumpCluster(const FCookCluster& CookCluster, bool bWriteToLog)
{
	SCOPED_NAMED_EVENT_TEXT("DumpCluster",FColor::Red);
	FString Dumper;
	Dumper.Append("--------------------Dump Cook Cluster--------------------\n");
	Dumper.Append(FString::Printf(TEXT("\tAsset Number: %d\n"),CookCluster.AssetDetails.Num()));
	
	FString PlatformsStr;
	{
		for(const auto& Platform:CookCluster.Platforms)
		{
			PlatformsStr = FString::Printf(TEXT("%s+%s"),*PlatformsStr,*THotPatcherTemplateHelper::GetEnumNameByValue(Platform));
		}
		PlatformsStr.RemoveFromStart(TEXT("+"));
	}
	Dumper.Append(FString::Printf(TEXT("\tCook Platforms: %s\n"),*PlatformsStr));
	Dumper.Append(FString::Printf(TEXT("\tPreGeneratePlatformData: %s\n"),CookCluster.bPreGeneratePlatformData?TEXT("true"):TEXT("false")));
	Dumper.Append(TEXT("\tCluster Assets:\n"));
	for(const auto& Asset:CookCluster.AssetDetails)
	{
		Dumper.Append(FString::Printf(TEXT("\t%s %s\n"),*Asset.AssetType.ToString(),*Asset.PackagePath.ToString()));
	}
	Dumper.Append("---------------------------------------------------------\n");
	if(bWriteToLog)
	{
		UE_LOG(LogHotPatcher,Log,TEXT("%s"),*Dumper);
	}
}

void USingleCookerProxy::ExecCookCluster(const FCookCluster& CookCluster)
{
	SCOPED_NAMED_EVENT_TEXT("ExecCookCluster",FColor::Red);

	if(!CookCluster.AssetDetails.Num())
	{
		return;
	}

	DumpCluster(CookCluster,GCookLog);
	
	TArray<ITargetPlatform*> TargetPlatforms = UFlibHotPatcherCoreHelper::GetTargetPlatformsByNames(CookCluster.Platforms);
	FString CookBaseDir = GetSettingObject()->GetStorageCookedAbsDir();
	CleanOldCooked(CookBaseDir,CookCluster.AsSoftObjectPaths(),CookCluster.Platforms);
	
#if WITH_PACKAGE_CONTEXT
	TMap<FString, FSavePackageContext*> SavePackageContextsNameMapping = GetPlatformSavePackageContextsNameMapping();
#endif
	TArray<UPackage*> PreCachePackages = UFlibAssetManageHelper::LoadPackagesForCooking(CookCluster.AsSoftObjectPaths(),GetSettingObject()->bConcurrentSave);

	bool bCanConcurrentSave = GetSettingObject()->bConcurrentSave && CookCluster.bPreGeneratePlatformData;
	// pre cahce platform data
	if(bCanConcurrentSave)
	{
		PreGeneratePlatformData(CookCluster);
	}

	// for cooking
	{
		GIsSavingPackage = bCanConcurrentSave;
		TMap<FName,TMap<FName,FString>> PackageCookedSavePaths;
		for(const auto& Package:PreCachePackages)
		{
			FName PackagePathName = *Package->GetPathName();
			PackageCookedSavePaths.Add(PackagePathName,TMap<FName,FString>{});
			for(auto Platform:TargetPlatforms)
			{
				FString CookedSavePath = UFlibHotPatcherCoreHelper::GetAssetCookedSavePath(CookBaseDir,PackagePathName.ToString(), Platform->PlatformName());
				PackageCookedSavePaths.Find(PackagePathName)->Add(*Platform->PlatformName(),CookedSavePath);
			}
		}
		
		ParallelFor(PreCachePackages.Num(), [=](int32 AssetIndex)
		{
			UFlibHotPatcherCoreHelper::CookPackage(
				PreCachePackages[AssetIndex],
				TargetPlatforms,
				CookCluster.CookActionCallback,
#if WITH_PACKAGE_CONTEXT
				SavePackageContextsNameMapping,
#endif
				*PackageCookedSavePaths.Find(*PreCachePackages[AssetIndex]->GetPathName()),
				GetSettingObject()->bConcurrentSave
			);
		},!GetSettingObject()->bConcurrentSave);
		GIsSavingPackage = false;
	}
	// clean cached ddc
	CleanClusterCachedPlatformData(CookCluster);
	GEngine->ForceGarbageCollection(true);
}

void USingleCookerProxy::Tick(float DeltaTime)
{
	SCOPED_NAMED_EVENT_TEXT("USingleCookerProxy::Tick",FColor::Red);
	if(!GetSettingObject() || !IsAlready())
	{
		return;
	}
	
	if(!CookCluserQueue.IsEmpty())
	{
		FCookCluster CookCluster;
		CookCluserQueue.Dequeue(CookCluster);
		ExecCookCluster(CookCluster);
	}
	
	if(CookCluserQueue.IsEmpty())
	{
		FCookCluster PackageTrackerCluster = GetPackageTrackerAsCluster();
		if(PackageTrackerCluster.AssetDetails.Num())
		{
			CookCluserQueue.Enqueue(PackageTrackerCluster);
		}
	}
}

TStatId USingleCookerProxy::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(USingleCookerProxy, STATGROUP_Tickables);
}

bool USingleCookerProxy::IsTickable() const
{
	return IsAlready();
}

void USingleCookerProxy::Shutdown()
{
	SCOPED_NAMED_EVENT_TEXT("USingleCookerProxy::Shutdown",FColor::Red);
	{
		SCOPED_NAMED_EVENT_TEXT("USingleCookerProxy::WaitCookerFinished",FColor::Red);
		// Wait for all shaders to finish compiling
		UFlibShaderCodeLibraryHelper::WaitShaderCompilingComplete();
		UFlibHotPatcherCoreHelper::WaitForAsyncFileWrites();
	}
	
	ShutdowShaderLibCollections();
	FString StorageMetadataAbsDir = GetSettingObject()->GetStorageMetadataAbsDir();
	// serialize cook failed assets to json
	if(GetCookFailedAssetsCollection().CookFailedAssets.Num())
	{
		FString FailedJsonString;
		THotPatcherTemplateHelper::TSerializeStructAsJsonString(GetCookFailedAssetsCollection(),FailedJsonString);
		UE_LOG(LogHotPatcher,Warning,TEXT("Single Cooker Proxy %s:\n%s"),*GetSettingObject()->MissionName,*FailedJsonString);
		FString SaveTo = UFlibHotCookerHelper::GetCookerProcFailedResultPath(StorageMetadataAbsDir,GetSettingObject()->MissionName,GetSettingObject()->MissionID);
		UFlibPatchParserHelper::ReplaceMark(SaveTo);
		FFileHelper::SaveStringToFile(FailedJsonString,*SaveTo);
	}
	
	if(PackageTracker)
	{
		auto SerializePackageSetToString = [](const TSet<FName>& Packages)->FString
		{
			FString OutString;
			FPackagePathSet AdditionalPackageSet;
			AdditionalPackageSet.PackagePaths.Append(Packages);
			if(AdditionalPackageSet.PackagePaths.Num())
			{
				THotPatcherTemplateHelper::TSerializeStructAsJsonString(AdditionalPackageSet,OutString);
			}
			return OutString;
		};
		const TSet<FName>& AllLoadedPackagePaths = PackageTracker->GetLoadedPackages();
		const TSet<FName>& AdditionalPackageSet = PackageTracker->GetPendingPackageSet();
		const TSet<FName>& CurrentCookerAssetPaths = GetCookerAssets();
		
		TSet<FName> NotInCookerAssets;
		{
			for(const auto& LoadedPackage:AllLoadedPackagePaths)
			{
				if(!CurrentCookerAssetPaths.Contains(LoadedPackage) && !AdditionalPackageSet.Contains(LoadedPackage))
				{
					NotInCookerAssets.Add(LoadedPackage);
				}
			}
		}
		
		FFileHelper::SaveStringToFile(SerializePackageSetToString(AdditionalPackageSet),*FPaths::Combine(StorageMetadataAbsDir,TEXT("AdditionalPackageSet.json")));
		FFileHelper::SaveStringToFile(SerializePackageSetToString(AllLoadedPackagePaths),*FPaths::Combine(StorageMetadataAbsDir,TEXT("AllLoadedPackageSet.json")));
		FFileHelper::SaveStringToFile(SerializePackageSetToString(NotInCookerAssets),*FPaths::Combine(StorageMetadataAbsDir,TEXT("OtherCookerAssetPackageSet.json")));
	}
	BulkDataManifest();
	IoStoreManifest();
	Super::Shutdown();
}


void USingleCookerProxy::BulkDataManifest()
{
	SCOPED_NAMED_EVENT_TEXT("BulkDataManifest",FColor::Red);
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
	SCOPED_NAMED_EVENT_TEXT("IoStoreManifest",FColor::Red);
	if(GetSettingObject()->IoStoreSettings.bIoStore)
	{
		TSet<ETargetPlatform> Platforms;
		for(auto Platform:GlobalCluser.Platforms)
		{
			Platforms.Add(Platform);
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
					FSoftObjectPath ObjectPath{ AssetPackagePath.ToString() };
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
			SaveCookOpenOrder(CookOpenOrders,FPaths::Combine(GetSettingObject()->GetStorageMetadataAbsDir(),GetSettingObject()->MissionName,PlatformName,TEXT("CookOpenOrder.txt")));
		}
	}
}

void USingleCookerProxy::InitShaderLibConllections()
{
	SCOPED_NAMED_EVENT_TEXT("USingleCookerProxy::InitShaderLibConllections",FColor::Red);
	FString SavePath = GetSettingObject()->GetStorageMetadataAbsDir();
	PlatformCookShaderCollection = UFlibHotCookerHelper::CreateCookShaderCollectionProxyByPlatform(
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
	SCOPED_NAMED_EVENT_TEXT("USingleCookerProxy::ShutdowShaderLibCollections",FColor::Red);
	if(GetSettingObject()->ShaderOptions.bSharedShaderLibrary)
	{
		if(PlatformCookShaderCollection.IsValid())
		{
			PlatformCookShaderCollection->Shutdown();
		}
	}
}

FCookCluster USingleCookerProxy::GetPackageTrackerAsCluster()
{
	SCOPED_NAMED_EVENT_TEXT("GetPackageTrackerAsCluster",FColor::Red);
	
	FCookCluster PackageTrackerCluster;
		
	PackageTrackerCluster.Platforms = GetSettingObject()->CookTargetPlatforms;
	PackageTrackerCluster.bPreGeneratePlatformData = GetSettingObject()->bPreGeneratePlatformData;

	PackageTrackerCluster.CookActionCallback.OnAssetCooked = GetOnPackageSavedCallback();
	PackageTrackerCluster.CookActionCallback.OnCookBegin = GetOnCookAssetBeginCallback();
			
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

FCookActionResultEvent USingleCookerProxy::GetOnPackageSavedCallback()
{
	SCOPED_NAMED_EVENT_TEXT("GetOnPackageSavedCallback",FColor::Red);
	const FCookActionResultEvent PackageSavedCallback = [this](const FSoftObjectPath& PackagePath,ETargetPlatform Platform,ESavePackageResult Result)
	{
		OnAssetCooked.Broadcast(PackagePath,Platform,Result);
	};
	return PackageSavedCallback;
}

FCookActionEvent USingleCookerProxy::GetOnCookAssetBeginCallback()
{
	const FCookActionEvent CookAssetBegin = [this](const FSoftObjectPath& PackagePath,ETargetPlatform Platform)
	{
		OnCookAssetBegin.Broadcast(PackagePath,Platform);
	};
	return CookAssetBegin;
}

TSet<FName> USingleCookerProxy::GetCookerAssets()
{
	static bool bInited = false;
	static TSet<FName> PackagePaths;
	if(!bInited)
	{
		for(const auto& Asset:GetSettingObject()->CookAssets)
		{
			PackagePaths.Add(*UFlibAssetManageHelper::PackagePathToLongPackageName(Asset.PackagePath.ToString()));
		}
		bInited = true;
	}
	return PackagePaths;
}

bool USingleCookerProxy::DoExport()
{
	SCOPED_NAMED_EVENT_TEXT("DoExport",FColor::Red);
	GlobalCluser.AssetDetails = GetSettingObject()->CookAssets;;
	GlobalCluser.Platforms = GetSettingObject()->CookTargetPlatforms;;
	GlobalCluser.CookActionCallback.OnAssetCooked = GetOnPackageSavedCallback();;
	GlobalCluser.CookActionCallback.OnCookBegin = GetOnCookAssetBeginCallback();
	GlobalCluser.bPreGeneratePlatformData = GetSettingObject()->bPreGeneratePlatformData;

	UFlibAssetManageHelper::ReplaceReditector(GlobalCluser.AssetDetails);
	UFlibAssetManageHelper::RemoveInvalidAssets(GlobalCluser.AssetDetails);
	
	CreateCookQueue();

	// force cook all in onece frame
	if(GetSettingObject()->bForceCookInOneFrame)
	{
		while(!CookCluserQueue.IsEmpty())
		{
			FCookCluster CookCluster;
			CookCluserQueue.Dequeue(CookCluster);
			ExecCookCluster(CookCluster);
		}
	
		if(CookCluserQueue.IsEmpty())
		{
			ExecCookCluster(GetPackageTrackerAsCluster());
		}
	}
	
	bAlready = true;
	return !HasError();
}

void USingleCookerProxy::CleanOldCooked(const FString& CookBaseDir,const TArray<FSoftObjectPath>& ObjectPaths,const TArray<ETargetPlatform>& CookPlatforms)
{
	TArray<ITargetPlatform*> CookPlatfotms = UFlibHotPatcherCoreHelper::GetTargetPlatformsByNames(CookPlatforms);
	{
		SCOPED_NAMED_EVENT_TEXT("Delete Old Cooked Files",FColor::Red);
		TArray<FString> PaddingDeleteFiles;

		// FCriticalSection	LocalSynchronizationObject;
		// for(const auto& Asset:ObjectPaths)
		ParallelFor(ObjectPaths.Num(),[&](int32 Index)
		{
			FString PackageName = ObjectPaths[Index].GetLongPackageName();
			for(const auto& TargetPlatform:CookPlatfotms)
			{
				FString CookedSavePath = UFlibHotPatcherCoreHelper::GetAssetCookedSavePath(CookBaseDir,PackageName, TargetPlatform->PlatformName());
				// FScopeLock Lock(&LocalSynchronizationObject);
				PaddingDeleteFiles.AddUnique(CookedSavePath);
			}
		},true);
	
		ParallelFor(PaddingDeleteFiles.Num(),[PaddingDeleteFiles](int32 Index)
		{
			FString FileName = PaddingDeleteFiles[Index];
			if(!FileName.IsEmpty() && FPaths::FileExists(FileName))
			{
				IFileManager::Get().Delete(*FileName,true,true,true);
			}
		},GForceSingleThread);
	}
}


bool USingleCookerProxy::HasError()
{
	SCOPED_NAMED_EVENT_TEXT("USingleCookerProxy::HasError",FColor::Red);
	TArray<ETargetPlatform> TargetPlatforms;
	GetCookFailedAssetsCollection().CookFailedAssets.GetKeys(TargetPlatforms);
	return !!TargetPlatforms.Num();
}

void USingleCookerProxy::OnAssetCookedHandle(const FSoftObjectPath& PackagePath, ETargetPlatform Platform,
	ESavePackageResult Result)
{
	FScopeLock Lock(&SynchronizationObject);
	SCOPED_NAMED_EVENT_TEXT("OnAssetCookedHandle",FColor::Red);
	if(Result == ESavePackageResult::Success)
	{
		GetPaendingCookAssetsSet().Remove(PackagePath.GetAssetPathName());
		GetPlatformCookAssetOrders(Platform).Add(PackagePath.GetAssetPathName());
	}
	else
	{
		SCOPED_NAMED_EVENT_TEXT("USingleCookerProxy::OnCookAssetFailed",FColor::Red);
		FString PlatformName = THotPatcherTemplateHelper::GetEnumNameByValue(Platform);
		UE_LOG(LogHotPatcher,Warning,TEXT("Cook %s for %s Failed (%s)!"),*PackagePath.GetAssetPathString(),*PlatformName, *UFlibHotPatcherCoreHelper::GetSavePackageResultStr(Result));
		GetPaendingCookAssetsSet().Remove(PackagePath.GetAssetPathName());
	}
}

bool USingleCookerProxy::IsFinsihed()
{
	return !GetPaendingCookAssetsSet().Num() && CookCluserQueue.IsEmpty();
}

#if WITH_PACKAGE_CONTEXT
TMap<ETargetPlatform, FSavePackageContext*> USingleCookerProxy::GetPlatformSavePackageContextsRaw()
{
	FScopeLock Lock(&SynchronizationObject);
	SCOPED_NAMED_EVENT_TEXT("USingleCookerProxy::GetPlatformSavePackageContextsRaw",FColor::Red);
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
	SCOPED_NAMED_EVENT_TEXT("USingleCookerProxy::GetPlatformSavePackageContextsNameMapping",FColor::Red);
	TMap<FString,FSavePackageContext*> result;
	TArray<ETargetPlatform> Keys;
	GetPlatformSavePackageContexts().GetKeys(Keys);
	for(const auto& Key:Keys)
	{
		result.Add(THotPatcherTemplateHelper::GetEnumNameByValue(Key),GetPlatformSavePackageContexts().Find(Key)->Get());
	}
	return result;
}
#endif

TArray<FName>& USingleCookerProxy::GetPlatformCookAssetOrders(ETargetPlatform Platform)
{
	FScopeLock Lock(&SynchronizationObject);
	return CookAssetOrders.FindOrAdd(Platform);
}

TSet<FName> USingleCookerProxy::GetAdditionalAssets()
{
	SCOPED_NAMED_EVENT_TEXT("GetAdditionalAssets",FColor::Red);
	if(GetSettingObject()->bPackageTracker && PackageTracker.IsValid())
	{
		return PackageTracker->GetPendingPackageSet();
	}
	return TSet<FName>{};
}

// pre cache asset type order
TArray<UClass*> USingleCookerProxy::GetPreCacheClasses() const
{
	SCOPED_NAMED_EVENT_TEXT("GetPreCacheClasses",FColor::Red);
	TArray<UClass*> Classes;

	TSet<UClass*> ParentClasses = {
		UTexture::StaticClass(),
		UMaterialExpression::StaticClass(),
		UMaterialFunctionInterface::StaticClass(),
		UMaterialInterface::StaticClass(),
	};

	for(auto& ParentClass:ParentClasses)
	{
		Classes.Append(UFlibHotPatcherCoreHelper::GetDerivedClasses(ParentClass,true,true));
	}

	for(auto& ParentClass:TArray<UClass*>{
		UFXSystemAsset::StaticClass(),
		UAnimSequence::StaticClass(),
		UStaticMesh::StaticClass(),
		USkeletalMesh::StaticClass(),
		UBlueprint::StaticClass(),
	})
	{
		Classes.Append(UFlibHotPatcherCoreHelper::GetDerivedClasses(ParentClass,true,true));
	}
	
	Classes.Append(TArray<UClass*>{
		ULevelSequence::StaticClass(),
		UWorld::StaticClass()});
	
	return Classes;
}

// void USingleCookerProxy::AsyncLoadAssets(const TArray<FSoftObjectPath>& ObjectPaths)
// {
// 	UAssetManager& AssetManager = UAssetManager::Get();
// 	FStreamableManager& StreamableManager = AssetManager.GetStreamableManager();
// 	StreamableManager.RequestAsyncLoad(ObjectPaths,FStreamableDelegate::CreateUObject(this,&USingleCookerProxy::OnAsyncAssetsLoaded));
// }
//
// void USingleCookerProxy::OnAsyncAssetsLoaded()
// {
// 	
// }
