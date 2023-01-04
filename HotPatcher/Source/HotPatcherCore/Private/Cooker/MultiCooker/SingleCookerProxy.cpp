#include "Cooker/MultiCooker/SingleCookerProxy.h"

#include "FlibHotPatcherCoreHelper.h"
#include "HotPatcherCore.h"
#include "HotPatcherRuntime.h"
#include "Cooker/MultiCooker/FlibHotCookerHelper.h"
#include "ShaderCompiler.h"
#include "UObject/UObjectHash.h"
#include "UObject/UObjectIterator.h"
#include "Async/ParallelFor.h"
#include "ShaderLibUtils/FlibShaderCodeLibraryHelper.h"
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
#define NO_POSTLOAD_CACHE_DDC_OPTION TEXT("-NoPostLoadCacheDDC")
void USingleCookerProxy::Init(FPatcherEntitySettingBase* InSetting)
{
	SCOPED_NAMED_EVENT_TEXT("Init",FColor::Red);
	Super::Init(InSetting);
	// IConsoleVariable* StreamableDelegateDelayFrames = IConsoleManager::Get().FindConsoleVariable(TEXT("s.StreamableDelegateDelayFrames"));
	// StreamableDelegateDelayFrames->Set(0);
	UFlibHotPatcherCoreHelper::DumpActiveTargetPlatforms();

	FString Cmdline = FCommandLine::Get();
	if(!Cmdline.Contains(NO_POSTLOAD_CACHE_DDC_OPTION,ESearchCase::IgnoreCase))
	{
		FCommandLine::Append(*FString::Printf(TEXT(" %s"),NO_POSTLOAD_CACHE_DDC_OPTION));
		UE_LOG(LogHotPatcher,Display,TEXT("Append %s to Cmdline."),NO_POSTLOAD_CACHE_DDC_OPTION);
	}
#if WITH_PACKAGE_CONTEXT
	if(GetSettingObject()->bOverrideSavePackageContext)
	{
		PlatformSavePackageContexts = GetSettingObject()->PlatformSavePackageContexts;
	}
	else
	{
		PlatformSavePackageContexts = UFlibHotPatcherCoreHelper::CreatePlatformsPackageContexts(
			GetSettingObject()->CookTargetPlatforms,
			GetSettingObject()->IoStoreSettings.bIoStore,
			GetSettingObject()->GetStorageCookedAbsDir()
			);
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

int32 USingleCookerProxy::MakeCookQueue(FCookCluster& InCluser)
{
	SCOPED_NAMED_EVENT_TEXT("MakeCookQueue",FColor::Red);
	int32 MakeClusterCount = 0;
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
	
	for(auto Class:GetPreCacheClasses())
	{
		TArray<FAssetDetail> ObjectAssets = UFlibAssetManageHelper::GetAssetDetailsByClass(InCluser.AssetDetails,Class,true);
		if(ObjectAssets.Num())
		{
			DumpCookerInfo.Append(FString::Printf(TEXT("\t%s -- %d\n"),*Class->GetName(),ObjectAssets.Num()));
		}
		int32 ClassesNumberOfAssetsPerFrame = GetClassAssetNumOfPerCluster(Class);
			
		while(ObjectAssets.Num())
		{
			int32 ClusterAssetNum = ClassesNumberOfAssetsPerFrame < 1 ? ObjectAssets.Num() : ClassesNumberOfAssetsPerFrame;
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
			++MakeClusterCount;
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
			++MakeClusterCount;
		}
		DumpCookerInfo.Append(FString::Printf(TEXT("\tOther Assets -- %d, make %d cluster.\n"),InCluser.AssetDetails.Num(),SplitedAssets.Num()));
	}
	
	DumpCookerInfo.Append(TEXT("---------------------------------------------------------------------\n"));
	UE_LOG(LogHotPatcher,Display,TEXT("%s"),*DumpCookerInfo);
	return MakeClusterCount;
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

	CookedClusterCount++;
	UE_LOG(LogHotPatcher,Display,TEXT("ExecuteCookCluster %d with %d assets, total cluster %d"),CookedClusterCount,CookCluster.AssetDetails.Num(),ClusterCount);
	
	FExecTimeRecoder ExecCookClusterTimer(TEXT("ExecCookCluster"));
	
	if(!CookCluster.AssetDetails.Num())
	{
		return;
	}

	bool bContainShaderCluster = UFlibHotPatcherCoreHelper::AssetDetailsHasClasses(CookCluster.AssetDetails,UFlibHotPatcherCoreHelper::GetAllMaterialClassesNames());
	DumpCluster(CookCluster,GCookLog);
	TArray<ITargetPlatform*> TargetPlatforms = UFlibHotPatcherCoreHelper::GetTargetPlatformsByNames(CookCluster.Platforms);
	FString CookBaseDir = GetSettingObject()->GetStorageCookedAbsDir();
	CleanOldCooked(CookBaseDir,CookCluster.AsSoftObjectPaths(),CookCluster.Platforms);
	
#if WITH_PACKAGE_CONTEXT
	TMap<FString, FSavePackageContext*> SavePackageContextsNameMapping = GetPlatformSavePackageContextsNameMapping();
#endif
	
	TSharedPtr<FClassesPackageTracker> ClassesPackageTracker = MakeShareable(new FClassesPackageTracker);
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
			// FExecTimeRecoder CookTimer(PreCachePackages[AssetIndex]->GetFullName());
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
	UFlibShaderCodeLibraryHelper::WaitShaderCompilingComplete();
	UFlibHotPatcherCoreHelper::WaitForAsyncFileWrites();
	// for GC
	{
		GEngine->ForceGarbageCollection(true);
		CollectGarbage(RF_NoFlags, true);
	}
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
			ClusterCount += MakeCookQueue(PackageTrackerCluster);
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
		for(FName LongPackageName:PackageTracker->GetPendingPackageSet())
		{
			// make asset data to asset registry
			FSoftObjectPath ObjectPath(
				UFlibAssetManageHelper::LongPackageNameToPackagePath(LongPackageName.ToString())
			);
			UFlibAssetManageHelper::UpdateAssetRegistryData(ObjectPath.GetLongPackageName());
			
			FAssetData AssetData;
			if(UAssetManager::Get().GetAssetDataForPath(ObjectPath,AssetData))
			{
				FAssetDetail AssetDetail;
				if(UFlibAssetManageHelper::ConvFAssetDataToFAssetDetail(AssetData,AssetDetail))
				{
					PackageTrackerCluster.AssetDetails.Emplace(AssetDetail);
				}
			}
			else
			{
				UE_LOG(LogHotPatcher,Warning,TEXT("[GetPackageTrackerAsCluster] Get %s AssetData Failed!"),*LongPackageName.ToString());
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
	
	ClusterCount = MakeCookQueue(GlobalCluser);

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
	SCOPED_NAMED_EVENT_TEXT("CleanOldCooked",FColor::Red);
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
	
	FString PlatformName = FString::Printf(TEXT("%lld"),(int64)Platform);
	if(GetPlatformNameMapping().Contains(Platform))
	{
		PlatformName = GetPlatformNameMapping().Find(Platform)->ToString();;
	}
		
	FString SavePackageResultStr = UFlibHotPatcherCoreHelper::GetSavePackageResultStr(Result);
	FName AssetPathName = PackagePath.GetAssetPathName();
	FString AssetPath = PackagePath.GetAssetPathString();
	
	GetPaendingCookAssetsSet().Remove(AssetPathName);

	switch(Result)
	{
		case ESavePackageResult::Success:
			{
				GetPlatformCookAssetOrders(Platform).Add(AssetPathName);
				break;
			}
		case ESavePackageResult::Error:
		case ESavePackageResult::MissingFile:
			{
				UE_LOG(LogHotPatcher,Error,TEXT("CookError %s for %s (%s)!"),*AssetPath,*PlatformName,*SavePackageResultStr);
				GetCookFailedAssetsCollection().CookFailedAssets.FindOrAdd(Platform).PackagePaths.Add(AssetPathName);
				break;
			}
		default:
			{
				UE_LOG(LogHotPatcher,Warning,TEXT("CookWarning %s for %s Failed (%s)!"),*AssetPath,*PlatformName, *SavePackageResultStr);
			}
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
	return UFlibHotPatcherCoreHelper::GetPreCacheClasses();
}


int32 USingleCookerProxy::GetClassAssetNumOfPerCluster(UClass* Class)
{
	int32 ClassesNumberOfAssetsPerFrame = GetSettingObject()->GetNumberOfAssetsPerFrame();
			

	for(const auto& OverrideClasses:GetSettingObject()->GetOverrideNumberOfAssetsPerFrame())
	{
		if(OverrideClasses.Key == Class)
		{
			ClassesNumberOfAssetsPerFrame = OverrideClasses.Value;
			break;
		}
	}
	return ClassesNumberOfAssetsPerFrame;
}
