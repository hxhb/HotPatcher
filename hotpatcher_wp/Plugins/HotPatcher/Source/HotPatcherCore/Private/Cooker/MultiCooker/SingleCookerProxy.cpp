#include "Cooker/MultiCooker/SingleCookerProxy.h"

#include "FlibHotPatcherCoreHelper.h"
#include "HotPatcherCore.h"
#include "HotPatcherRuntime.h"
#include "Cooker/MultiCooker/FlibHotCookerHelper.h"
#include "ShaderCompiler.h"
#include "UObject/UObjectHash.h"
#include "UObject/UObjectIterator.h"
#include "Async/ParallelFor.h"
#include "ShaderPatch/FlibShaderCodeLibraryHelper.h"
#include "ThreadUtils/FThreadUtils.hpp"
#include "Cooker/MultiCooker/FCookShaderCollectionProxy.h"
#include "Engine/Engine.h"
#include "Misc/ScopeExit.h"
#include "Engine/AssetManager.h"
#include "Interfaces/ITargetPlatform.h"

#if WITH_PACKAGE_CONTEXT
// // engine header
#include "UObject/SavePackage.h"
#endif

#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION > 25
#include "Serialization/BulkDataManifest.h"
#endif


bool IsAlwayPostLoadClasses(UPackage* Package, UObject* Object)
{
	return true;
}

void FFreezePackageTracker::NotifyUObjectCreated(const UObjectBase* Object, int32 Index)
{
	// UObject* ObjectOuter = Object->GetOuter();
	auto ObjectOuter = const_cast<UObject*>(static_cast<const UObject*>(Object));
	UPackage* Package = ObjectOuter ? ObjectOuter->GetOutermost() : nullptr;
	if(Package)
	{
		    
		FName AssetPathName = FName(*Package->GetPathName());
		if(!CookerAssetsSet.Contains(AssetPathName) && AllAssetsSet.Contains(AssetPathName) && !IsAlwayPostLoadClasses(Package, ObjectOuter))
		{
			PackageObjectsMap.FindOrAdd(AssetPathName).Add(ObjectOuter);
			FreezeObjects.Add(ObjectOuter);
			ObjectOuter->ClearFlags(RF_NeedPostLoad);
			ObjectOuter->ClearFlags(RF_NeedPostLoadSubobjects);
		}
	}
}

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
		OtherCookerPackageTracker = MakeShareable(new FOtherCookerPackageTracker(GetCookerAssets(),GetSettingObject()->SkipLoadedAssets));
		// FreezePackageTracker = MakeShareable(new FFreezePackageTracker(GetCookerAssets(),GetSettingObject()->SkipLoadedAssets));
	}
	
	UFlibHotPatcherCoreHelper::DeleteDirectory(GetSettingObject()->GetStorageMetadataAbsDir());
	
	GetCookFailedAssetsCollection().MissionName = GetSettingObject()->MissionName;
	GetCookFailedAssetsCollection().MissionID = GetSettingObject()->MissionID;
	GetCookFailedAssetsCollection().CookFailedAssets.Empty();
	OnAssetCooked.AddUObject(this,&USingleCookerProxy::OnAssetCookedHandle);
}

void USingleCookerProxy::MakeCookQueue(FCookCluster& InCluser)
{
	SCOPED_NAMED_EVENT_TEXT("MakeCookQueue",FColor::Red);

	FString DumpCookerInfo;
	DumpCookerInfo.Append(TEXT("\n-----------------------------Dump Cooker-----------------------------\n"));
	DumpCookerInfo.Append(FString::Printf(TEXT("\tTotal Asset: %d\n"),InCluser.AssetDetails.Num()));
	DumpCookerInfo.Append(FString::Printf(TEXT("\tbForceCookInOneFrame: %s\n"),GetSettingObject()->bForceCookInOneFrame ? TEXT("true"):TEXT("false")));
	FString PlatformsStr;
	for(auto Platform:InCluser.Platforms)
	{
		PlatformsStr += THotPatcherTemplateHelper::GetEnumNameByValue(Platform);
		PlatformsStr.Append(TEXT(","));
	}
	PlatformsStr.RemoveFromEnd(TEXT(","));
	DumpCookerInfo.Append(FString::Printf(TEXT("\tTarget Platforms: %s\n"),*PlatformsStr));
	
	const int32 NumberOfAssetsPerFrame = GetSettingObject()->GetNumberOfAssetsPerFrame();
	
	if(GetSettingObject()->bForceCookInOneFrame)
	{
		CookCluserQueue.Enqueue(InCluser);
		return;
	}
	else
	{
		for(auto Class:GetPreCacheClasses())
		{
			TArray<FAssetDetail> ObjectAssets = UFlibAssetManageHelper::GetAssetDetailsByClass(InCluser.AssetDetails,Class,true);
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

		if(InCluser.AssetDetails.Num())
		{
			int32 OtherAssetNumPerFrame = NumberOfAssetsPerFrame < 1 ? InCluser.AssetDetails.Num() : NumberOfAssetsPerFrame;
			int32 SplitNum = (InCluser.AssetDetails.Num() / OtherAssetNumPerFrame) + 1;
			
			const TArray<TArray<FAssetDetail>> SplitedAssets= THotPatcherTemplateHelper::SplitArray(InCluser.AssetDetails,SplitNum);
			for(const auto& AssetDetails:SplitedAssets)
			{
				FCookCluster NewCluster;
				NewCluster.AssetDetails = std::move(AssetDetails);
				NewCluster.Platforms = GetSettingObject()->CookTargetPlatforms;
				NewCluster.bPreGeneratePlatformData = GetSettingObject()->bPreGeneratePlatformData;
				NewCluster.CookActionCallback.OnAssetCooked = GetOnPackageSavedCallback();
				NewCluster.CookActionCallback.OnCookBegin = GetOnCookAssetBeginCallback();
				CookCluserQueue.Enqueue(NewCluster);
			}
			DumpCookerInfo.Append(FString::Printf(TEXT("\tOther Assets -- %d, make %d cluster.\n"),InCluser.AssetDetails.Num(),SplitedAssets.Num()));
		}
	}
	DumpCookerInfo.Append(TEXT("---------------------------------------------------------------------\n"));
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
#if ENGINE_MAJOR_VERSION > 4
		Package->MarkAsGarbage();
#else
		Package->MarkPendingKill();
#endif
	}
}

void USingleCookerProxy::PreGeneratePlatformData(const FCookCluster& CookCluster)
{
	SCOPED_NAMED_EVENT_TEXT("PreGeneratePlatformData",FColor::Red);
	FExecTimeRecoder PreGeneratePlatformDataTimer(TEXT("PreGeneratePlatformData"));
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
				GetSettingObject()->bWaitEachAssetCompleted,
				[&](UPackage* Package,UObject* ExportObj)
				{
					if(FreezePackageTracker.IsValid() && FreezePackageTracker->IsFreezed(ExportObj))
					{
						ExportObj->SetFlags(RF_NeedPostLoad);
						ExportObj->SetFlags(RF_NeedPostLoadSubobjects);
						ExportObj->ConditionalPostLoad();
					}
				});
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
	Dumper.Append("\n--------------------Dump Cook Cluster--------------------\n");
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
		UE_LOG(LogHotPatcher,Display,TEXT("%s"),*Dumper);
	}
}

void USingleCookerProxy::ExecCookCluster(const FCookCluster& CookCluster)
{
	SCOPED_NAMED_EVENT_TEXT("ExecCookCluster",FColor::Red);

	FExecTimeRecoder ExecCookClusterTimer(TEXT("ExecCookCluster"));
	
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
	if(CookCluster.bPreGeneratePlatformData)
	{
		PreGeneratePlatformData(CookCluster);
	}

	// for cooking
	{
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

		TMap<ETargetPlatform,ITargetPlatform*> PlatformMaps;
		for(auto Platform:TargetPlatforms)
		{
			ETargetPlatform EnumPlatform;
			THotPatcherTemplateHelper::GetEnumValueByName(*Platform->PlatformName(),EnumPlatform);
			PlatformMaps.Add(EnumPlatform,Platform);
		}
		
		auto CookPackageLambda = [&](int32 AssetIndex)
		{
			FExecTimeRecoder CookTimer(PreCachePackages[AssetIndex]->GetFullName());
			UFlibHotPatcherCoreHelper::CookPackage(
				PreCachePackages[AssetIndex],
				PlatformMaps,
				CookCluster.CookActionCallback,
#if WITH_PACKAGE_CONTEXT
				SavePackageContextsNameMapping,
#endif
				*PackageCookedSavePaths.Find(*PreCachePackages[AssetIndex]->GetPathName()),
				GetSettingObject()->bConcurrentSave
			);
		};
		
		if(bCanConcurrentSave)
		{
			for(auto& Platform:TargetPlatforms)
			{
				ETargetPlatform TargetPlatform;
				THotPatcherTemplateHelper::GetEnumValueByName(Platform->PlatformName(),TargetPlatform);
			}
			GIsSavingPackage = true;
			ParallelFor(PreCachePackages.Num(), CookPackageLambda,GForceSingleThread ? true : !bCanConcurrentSave);
			GIsSavingPackage = false;
		}
		else
		{
			for(int32 index = 0;index<PreCachePackages.Num();++index)
			{
				CookPackageLambda(index);
			}
		}
	}

	// clean cached ddd / release memory
	// CleanClusterCachedPlatformData(CookCluster);
	// GEngine->ForceGarbageCollection(true);
}

void USingleCookerProxy::Tick(float DeltaTime)
{
	SCOPED_NAMED_EVENT_TEXT("USingleCookerProxy::Tick",FColor::Red);
	if(!GetSettingObject() || GetSettingObject()->bForceCookInOneFrame|| !IsAlready())
	{
		return;
	}
	
	if(!CookCluserQueue.IsEmpty())
	{
		FCookCluster CookCluster;
		CookCluserQueue.Dequeue(CookCluster);
		ExecCookCluster(CookCluster);
	}
	static bool bCookedPackageTracker = false;
	
	if(!bCookedPackageTracker && CookCluserQueue.IsEmpty() && GetSettingObject()->bPackageTracker && GetSettingObject()->bCookPackageTrackerAssets)
	{
		FCookCluster PackageTrackerCluster = GetPackageTrackerAsCluster();
		if(PackageTrackerCluster.AssetDetails.Num())
		{
			MakeCookQueue(PackageTrackerCluster);
			bCookedPackageTracker = true;
		}
	}
}

TStatId USingleCookerProxy::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(USingleCookerProxy, STATGROUP_Tickables);
}

bool USingleCookerProxy::IsTickable() const
{
	return IsAlready() && !const_cast<USingleCookerProxy*>(this)->GetSettingObject()->bForceCookInOneFrame;
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
		FString SaveTo = UFlibHotCookerHelper::GetCookerProcFailedResultPath(StorageMetadataAbsDir,GetSettingObject()->MissionName,GetSettingObject()->MissionID);
		SaveTo = UFlibPatchParserHelper::ReplaceMark(SaveTo);
		UE_LOG(LogHotPatcher,Warning,TEXT("Single Cooker Proxy Cook Failed Assets %s:\n%s\nSerialize to %s"),*GetSettingObject()->MissionName,*FailedJsonString,*SaveTo);
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
		const TSet<FName> LoadedOtherCookerAssets = OtherCookerPackageTracker->GetLoadOtherCookerAssets();

		if(AllLoadedPackagePaths.Num())
		{
			FFileHelper::SaveStringToFile(SerializePackageSetToString(AllLoadedPackagePaths),*FPaths::Combine(StorageMetadataAbsDir,TEXT("AllLoadedPackageSet.json")));
		}
		if(AdditionalPackageSet.Num())
		{
			FFileHelper::SaveStringToFile(SerializePackageSetToString(AdditionalPackageSet),*FPaths::Combine(StorageMetadataAbsDir,TEXT("AdditionalPackageSet.json")));
		}
		if(LoadedOtherCookerAssets.Num())
		{
			FFileHelper::SaveStringToFile(SerializePackageSetToString(LoadedOtherCookerAssets),*FPaths::Combine(StorageMetadataAbsDir,TEXT("OtherCookerAssetPackageSet.json")));
		}
	}
	BulkDataManifest();
	IoStoreManifest();
	bAlready = false;
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
			
	if(PackageTracker && GetSettingObject()->bCookPackageTrackerAssets)
	{
		PackageTrackerCluster.AssetDetails.Empty();
		for(FName PackagePath:PackageTracker->GetPendingPackageSet())
		{
			FAssetDetail AssetDetail = UFlibAssetManageHelper::GetAssetDetailByPackageName(PackagePath.ToString());
			if(AssetDetail.IsValid())
			{
				PackageTrackerCluster.AssetDetails.Emplace(AssetDetail);
			}
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
	
	MakeCookQueue(GlobalCluser);

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

	FName AssetPathName = PackagePath.GetAssetPathName();
	GetPaendingCookAssetsSet().Remove(AssetPathName);
	
	if(Result == ESavePackageResult::Success)
	{
		GetPlatformCookAssetOrders(Platform).Add(AssetPathName);
	}
	else
	{
		SCOPED_NAMED_EVENT_TEXT("USingleCookerProxy::OnCookAssetFailed",FColor::Red);
		UE_LOG(LogHotPatcher,Warning,TEXT("Cook %s for %d Failed (%s)!"),*PackagePath.GetAssetPathString(),(int32)Platform, *UFlibHotPatcherCoreHelper::GetSavePackageResultStr(Result));
		GetCookFailedAssetsCollection().CookFailedAssets.FindOrAdd(Platform).PackagePaths.Add(AssetPathName);
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

TArray<UClass*> GetClassesByNames(const TArray<FName>& ClassesNames)
{
	TArray<UClass*> result;
	for(const auto& ClassesName:ClassesNames)
	{
		for (TObjectIterator<UClass> Itt; Itt; ++Itt)
		{
			if((*Itt)->GetName().Equals(ClassesName.ToString()))
			{
				result.Add(*Itt);
				break;
			}
		}
	}
	return result;
}
// pre cache asset type order
TArray<UClass*> USingleCookerProxy::GetPreCacheClasses() const
{
	SCOPED_NAMED_EVENT_TEXT("GetPreCacheClasses",FColor::Red);
	TArray<UClass*> Classes;
	
	TArray<FName> ParentClassesName = {
		// textures
		TEXT("Texture"),
		TEXT("PaperSprite"),
		// material
		TEXT("MaterialExpression"),
		TEXT("MaterialParameterCollection"),
		TEXT("MaterialFunctionInterface"),
		TEXT("MaterialInterface"),
		// other
		TEXT("PhysicsAsset"),
		TEXT("PhysicalMaterial"),
		TEXT("StaticMesh"),
		// curve
		TEXT("CurveFloat"),
		TEXT("CurveVector"),
		TEXT("CurveLinearColor"),
		// skeletal and animation
		TEXT("Skeleton"),
		TEXT("SkeletalMesh"),
		TEXT("AnimSequence"),
		TEXT("BlendSpace1D"),
		TEXT("BlendSpace"),
		TEXT("AnimMontage"),
		TEXT("AnimComposite"),
		// blueprint
		TEXT("UserDefinedStruct"),
		TEXT("Blueprint"),
		// sound
		TEXT("SoundWave"),
		// particles
		TEXT("FXSystemAsset"),
		// large ref asset
		TEXT("ActorSequence"),
		TEXT("LevelSequence"),
		TEXT("World")
	};

	for(auto& ParentClass:GetClassesByNames(ParentClassesName))
	{
		Classes.Append(UFlibHotPatcherCoreHelper::GetDerivedClasses(ParentClass,true,true));
	}
	
	return Classes;
}
