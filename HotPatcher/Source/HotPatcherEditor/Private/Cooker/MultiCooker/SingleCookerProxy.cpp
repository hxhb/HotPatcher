#include "Cooker/MultiCooker/SingleCookerProxy.h"
#include "FlibHotPatcherEditorHelper.h"
#include "FlibMultiCookerHelper.h"
#include "ShaderCompiler.h"
#include "Async/ParallelFor.h"
#include "ShaderPatch/FlibShaderCodeLibraryHelper.h"
#include "ThreadUtils/FThreadUtils.hpp"
#include "Cooker/MultiCooker/FCookShaderCollectionProxy.h"
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
		PlatformSavePackageContexts = UFlibHotPatcherEditorHelper::CreatePlatformsPackageContexts(GetSettingObject()->CookTargetPlatforms,GetSettingObject()->IoStoreSettings.bIoStore);
	}
#endif
	InitShaderLibConllections();
	// cook package tracker
	if(GetSettingObject()->bPackageTracker)
	{
		PackagePathSet.PackagePaths.Append(GetSettingObject()->SkipLoadedAssets);
		for(const auto& Asset:GetSettingObject()->CookAssets)
		{
			PackagePathSet.PackagePaths.Add(Asset.PackagePath);
		}
		PackageTracker = MakeShareable(new FPackageTracker(PackagePathSet.PackagePaths));
	}
	IFileManager::Get().DeleteDirectory(*GetSettingObject()->StorageMetadataDir);
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
		OnCookAssetSuccessed.Broadcast(PackagePath,Platform,Result);
	};
	const FCookActionResultEvent CookFailedCallback = [this](const FSoftObjectPath& PackagePath,ETargetPlatform Platform,ESavePackageResult Result)
	{
		OnCookAssetFailed.Broadcast(PackagePath,Platform,Result);
	};
	const FCookActionEvent CookAssetBegin = [this](const FSoftObjectPath& PackagePath,ETargetPlatform Platform)
	{
		OnCookAssetBegin.Broadcast(PackagePath,Platform);
	};
	
	FCookCluster DefaultCluser;
	for(const auto& AssetDetail:Assets)
	{
		FSoftObjectPath SoftObjectPath{AssetDetail.PackagePath};
		DefaultCluser.Assets.AddUnique(SoftObjectPath);
		GetAssetTypeMapping().Add(SoftObjectPath.GetAssetPathName(),AssetDetail.AssetType);
	}
	
	DefaultCluser.Platforms = GetSettingObject()->CookTargetPlatforms;
	DefaultCluser.bPreGeneratePlatformData = GetSettingObject()->bPreGeneratePlatformData;
	
	DefaultCluser.CookActionCallback.CookFailedCallback = CookFailedCallback;
	DefaultCluser.CookActionCallback.PackageSavedCallback = PackageSavedCallback;
	DefaultCluser.CookActionCallback.BeginCookCallback = CookAssetBegin;
	
	CookCluster(DefaultCluser, GetSettingObject()->bAsyncLoad);

	if(PackageTracker && GetSettingObject()->bCookAdditionalAssets)
	{
		DefaultCluser.Assets.Empty();
		for(FName PackagePath:PackageTracker->GetPendingPackageSet())
		{
			DefaultCluser.Assets.Emplace(PackagePath);
		}
		// cook all additional assets
		CookCluster(DefaultCluser, GetSettingObject()->bAsyncLoad);
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
			UFlibHotPatcherEditorHelper::SavePlatformBulkDataManifest(GetPlatformSavePackageContexts(),Platform);
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
		SavePath
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

	OnCookAssetFailed.AddUObject(this,&USingleCookerProxy::OnCookAssetFailedHandle);
	OnCookAssetSuccessed.AddUObject(this,&USingleCookerProxy::OnCookAssetSuccessedHandle);
	
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

bool USingleCookerProxy::IsFinsihed()
{
	return !GetPaendingCookAssetsSet().Num();
}

void USingleCookerProxy::CookClusterAsync(const FCookCluster& CookCluster)
{
	SCOPED_NAMED_EVENT_TCHAR(TEXT("USingleCookerProxy::CookClusterASync"),FColor::Red);
	TArray<ITargetPlatform*> CookPlatfotms = UFlibHotPatcherEditorHelper::GetTargetPlatformsByNames(CookCluster.Platforms);
	FCookActionCallback CookActionCallback = CookCluster.CookActionCallback;
	for(const auto& Asset:CookCluster.Assets)
	{
		uint32 Priority = GetAssetTypeMapping().Find(Asset.GetAssetPathName())->IsEqual(WorldType) ? UFlibAssetManageHelper::AsyncLoadHighPriority : UFlibAssetManageHelper::DefaultAsyncLoadPriority;
		UFlibAssetManageHelper::LoadObjectAsync(Asset,[this,CookPlatfotms,CookActionCallback](FSoftObjectPath ObjectPath)
		{
			OnAsyncObjectLoaded(ObjectPath,CookPlatfotms,CookActionCallback);
		},Priority);
		GetPaendingCookAssetsSet().Add(*Asset.GetLongPackageName());
	}
}

void USingleCookerProxy::CookClusterSync(const FCookCluster& CookCluster)
{
	SCOPED_NAMED_EVENT_TCHAR(TEXT("USingleCookerProxy::CookClusterSync"),FColor::Red);
	
	TArray<ITargetPlatform*> TargetPlatforms = UFlibHotPatcherEditorHelper::GetTargetPlatformsByNames(CookCluster.Platforms);

	if(GetSettingObject()->bConcurrentSave)
	{
		FString CookBaseDir = GetSettingObject()->StorageCookedDir;
		TMap<FString, FSavePackageContext*> SavePackageContextsNameMapping = GetPlatformSavePackageContextsNameMapping();
		ParallelFor(CookCluster.Assets.Num(), [=](int32 AssetIndex)
		{
			UFlibHotPatcherEditorHelper::CookPackage(
				CookCluster.Assets[AssetIndex],
				TargetPlatforms,
				CookCluster.CookActionCallback,
				SavePackageContextsNameMapping,CookBaseDir,true);
		},!GetSettingObject()->bConcurrentSave);
	}
	else
	{
		UFlibHotPatcherEditorHelper::CookAssets(CookCluster.Assets,CookCluster.Platforms,CookCluster.CookActionCallback
#if WITH_PACKAGE_CONTEXT
									,GetPlatformSavePackageContextsRaw()
#endif
									,GetSettingObject()->StorageCookedDir
);
	}
}


void USingleCookerProxy::PreGeneratePlatformData(const FCookCluster& CookCluster)
{
	TArray<ITargetPlatform*> TargetPlatforms = UFlibHotPatcherEditorHelper::GetTargetPlatformsByNames(CookCluster.Platforms);
	bool bConcurrentSave = GetSettingObject()->bConcurrentSave;
	if(CookCluster.bPreGeneratePlatformData)
	{
		TArray<UPackage*> AllObjects;
		{
			SCOPED_NAMED_EVENT_TCHAR(TEXT("BeginCacheForCookedPlatformData for Assets"),FColor::Red);
			GIsCookerLoadingPackage = true;
			for (const auto& AssetObjectPath : CookCluster.Assets)
			{
				if(!AssetObjectPath.IsValid())
					continue;

				FString PackageName = AssetObjectPath.GetLongPackageName();
				if(GetSettingObject()->IsSkipAsset(PackageName))
				{
					UE_LOG(LogHotPatcher,Display,TEXT("Skip Cook %s"),*PackageName);
					continue;
				}
				UPackage* Package = LoadPackage(nullptr, *PackageName, LOAD_None);
				if(!Package)
				{
					UE_LOG(LogHotPatcher,Warning,TEXT("CookCluster LodPackage %s is null!"),*PackageName);
					continue;
				}
				AllObjects.AddUnique(Package);
	
				UFlibHotPatcherEditorHelper::BeginPackageObjectsCacheForCookedPlatformData(Package,TargetPlatforms,true);
			}
			GIsCookerLoadingPackage = false;
		}
	
		TArray<UObject*> ObjectsToWaitForCookedPlatformData;
		ObjectsToWaitForCookedPlatformData.Reserve(65536);
	
		for (const auto& Obj : AllObjects)
		{
			bool bAllPlatformDataLoaded = true;
			bool bIsTexture = Obj->IsA(UTexture::StaticClass());
			for (const ITargetPlatform* TargetPlatform : TargetPlatforms)
			{
				if (bConcurrentSave)
				{
					GIsCookerLoadingPackage = true;
					{
						Obj->PreSave(TargetPlatform);
					}
					GIsCookerLoadingPackage = false;
				}
				if (!bIsTexture || bConcurrentSave)
				{
					Obj->BeginCacheForCookedPlatformData(TargetPlatform);
					if (!Obj->IsCachedCookedPlatformDataLoaded(TargetPlatform))
					{
						bAllPlatformDataLoaded = false;
					}
				}
			}
			if (!bAllPlatformDataLoaded)
			{
				ObjectsToWaitForCookedPlatformData.Add(Obj);
			}
		}
	
		// Wait for all shaders to finish compiling
		UFlibShaderCodeLibraryHelper::WaitShaderCompilingComplate();
	
		// Wait for all platform data to be loaded
		{
			SCOPED_NAMED_EVENT_TCHAR(TEXT("Wait for all platform data to be loaded"),FColor::Red);
			UE_LOG(LogHotPatcher, Display, TEXT("Waiting for ObjectsToWaitForCookedPlatformData compilation..."));
			while (ObjectsToWaitForCookedPlatformData.Num() > 0)
			{
				for (int32 ObjIdx = ObjectsToWaitForCookedPlatformData.Num() - 1; ObjIdx >= 0; --ObjIdx)
				{
					UObject* Obj = ObjectsToWaitForCookedPlatformData[ObjIdx];
					bool bAllPlatformDataLoaded = true;
					for (const ITargetPlatform* TargetPlatform : TargetPlatforms)
					{
						if (!Obj->IsCachedCookedPlatformDataLoaded(TargetPlatform))
						{
							bAllPlatformDataLoaded = false;
							break;
						}
					}
	
					if (bAllPlatformDataLoaded)
					{
						ObjectsToWaitForCookedPlatformData.RemoveAtSwap(ObjIdx, 1, false);
					}
				}
	
				FPlatformProcess::Sleep(0.001f);
			}
	
			ObjectsToWaitForCookedPlatformData.Empty();
			UE_LOG(LogHotPatcher, Display, TEXT("ObjectsToWaitForCookedPlatformData compilated!"));
		}
	}
}

void USingleCookerProxy::CookCluster(const FCookCluster& CookCluster, bool bAsync)
{
	SCOPED_NAMED_EVENT_TCHAR(TEXT("USingleCookerProxy::CookCluster"),FColor::Red);
	if(CookCluster.bPreGeneratePlatformData)
	{
		PreGeneratePlatformData(CookCluster);
	}
	if(bAsync)
	{
		CookClusterAsync(CookCluster);
	}
	else
	{
		CookClusterSync(CookCluster);
	}
}

void USingleCookerProxy::AddCluster(const FCookCluster& CookCluster)
{
	for(auto Platform:CookCluster.Platforms)
	{
		if(!GetPlatformSavePackageContexts().Contains(Platform))
		{
			GetPlatformSavePackageContexts().Append(UFlibHotPatcherEditorHelper::CreatePlatformsPackageContexts(TArray<ETargetPlatform>{Platform},GetSettingObject()->IoStoreSettings.bIoStore));
		}
	}
	for(const auto& SoftObjectPath:CookCluster.Assets)
	{
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

void USingleCookerProxy::OnCookAssetFailedHandle(const FSoftObjectPath& PackagePath, ETargetPlatform Platform,ESavePackageResult Result)
{
	SCOPED_NAMED_EVENT_TCHAR(TEXT("USingleCookerProxy::OnCookAssetFailed"),FColor::Red);
	FString PlatformName = THotPatcherTemplateHelper::GetEnumNameByValue(Platform);
	UE_LOG(LogHotPatcher,Warning,TEXT("Cook %s for %s Failed!"),*PackagePath.GetAssetPathString(),*PlatformName);
	FAssetsCollection& AssetsCollection = GetCookFailedAssetsCollection().CookFailedAssets.FindOrAdd(Platform);
	AssetsCollection.TargetPlatform = Platform;
	FAssetData AssetData;
	FAssetDetail AssetDetail;
	UFlibAssetManageHelper::GetSingleAssetsData(PackagePath.GetAssetPathString(),AssetData);
	UFlibAssetManageHelper::ConvFAssetDataToFAssetDetail(AssetData,AssetDetail);
	AssetsCollection.Assets.AddUnique(AssetDetail);
	GetPaendingCookAssetsSet().Remove(PackagePath.GetAssetPathName());
}

void USingleCookerProxy::OnCookAssetSuccessedHandle(const FSoftObjectPath& PackagePath, ETargetPlatform Platform,ESavePackageResult Result)
{
	GetPaendingCookAssetsSet().Remove(PackagePath.GetAssetPathName());
	MarkAssetCooked(PackagePath,Platform);
}

void USingleCookerProxy::OnAsyncObjectLoaded(FSoftObjectPath ObjectPath,const TArray<ITargetPlatform*>& Platforms,FCookActionCallback CookActionCallback)
{
	UFlibHotPatcherEditorHelper::CookPackage(ObjectPath,Platforms,CookActionCallback,GetPlatformSavePackageContextsNameMapping(),GetSettingObject()->StorageCookedDir,true);
}

void USingleCookerProxy::WaitCookerFinished()
{
	SCOPED_NAMED_EVENT_TCHAR(TEXT("USingleCookerProxy::WaitCookerFinished"),FColor::Red);
	// Wait for all shaders to finish compiling
	UFlibShaderCodeLibraryHelper::WaitShaderCompilingComplate();
	UFlibHotPatcherEditorHelper::WaitForAsyncFileWrites();
}

TMap<ETargetPlatform, FSavePackageContext*> USingleCookerProxy::GetPlatformSavePackageContextsRaw()
{
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

void USingleCookerProxy::MarkAssetCooked(const FSoftObjectPath& PackagePath, ETargetPlatform Platform)
{
	GetPlatformCookAssetOrders(Platform).Add(PackagePath.GetAssetPathName());
}


#endif
