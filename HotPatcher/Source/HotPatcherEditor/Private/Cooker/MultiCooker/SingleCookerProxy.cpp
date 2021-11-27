#include "Cooker/MultiCooker/SingleCookerProxy.h"
#include "FlibHotPatcherEditorHelper.h"
#include "FlibMultiCookerHelper.h"
#include "ShaderCompiler.h"
#include "ShaderPatch/FlibShaderCodeLibraryHelper.h"
#include "ThreadUtils/FThreadUtils.hpp"

void USingleCookerProxy::Init()
{
	InitPlatformPackageContexts();
	Super::Init();
}

void USingleCookerProxy::Shutdown()
{
	Super::Shutdown();
}

TSharedPtr<FCookShaderCollectionProxy> USingleCookerProxy::CreateCookShaderCollectionProxyByPlatform(ETargetPlatform Platform)
{
	TSharedPtr<FCookShaderCollectionProxy> CookShaderCollection;
	if(GetSettingObject()->MultiCookerSettings.ShaderOptions.bSharedShaderLibrary)
	{
		FString PlatformName = UFlibPatchParserHelper::GetEnumNameByValue(Platform);
		FString SavePath = FPaths::Combine(UFlibMultiCookerHelper::GetMultiCookerBaseDir(),GetSettingObject()->MissionName);
		if(FPaths::DirectoryExists(SavePath))
		{
			IFileManager::Get().DeleteDirectory(*SavePath);
		}
		FString ActualLibraryName = UFlibShaderCodeLibraryHelper::GenerateShaderCodeLibraryName(FApp::GetProjectName(),false);
		FString ShaderLibraryName = FString::Printf(TEXT("%s_Shader_%d"),*GetSettingObject()->MissionName,GetSettingObject()->MissionID);
		CookShaderCollection = MakeShareable(
			new FCookShaderCollectionProxy(
				PlatformName,
				ShaderLibraryName,
				GetSettingObject()->MultiCookerSettings.ShaderOptions.bNativeShader,
				SavePath
				));
		PlatformCookShaderCollectionMap.Add(Platform,CookShaderCollection);
	}
	return CookShaderCollection;
}

void USingleCookerProxy::DoCookMission(const TArray<FAssetDetail>& Assets)
{
	TArray<FSoftObjectPath> SoftObjectPaths;
	TArray<UPackage*> AllObjects;

	USingleCookerProxy::FShaderCollection ShaderCollection(this);


	TArray<ITargetPlatform*> TargetPlatforms;
	TArray<FString> TargetPlatformNames;
	for(const auto& Platform:GetSettingObject()->MultiCookerSettings.CookTargetPlatforms)
	{
		FString PlatformName = UFlibPatchParserHelper::GetEnumNameByValue(Platform);
		TargetPlatformNames.AddUnique(PlatformName);
		ITargetPlatform* TargetPlatform = UFlibHotPatcherEditorHelper::GetTargetPlatformByName(PlatformName);
		if(TargetPlatform)
		{
			TargetPlatforms.AddUnique(TargetPlatform);
		}
	}
	
	GIsCookerLoadingPackage = true;
	for (const auto& Asset : Assets)
	{
		FSoftObjectPath AssetSoftPath;
		AssetSoftPath.SetPath(Asset.mPackagePath);
		UPackage* Package = LoadPackage(nullptr, *Asset.mPackagePath, LOAD_None);;
		SoftObjectPaths.AddUnique(AssetSoftPath);
		AllObjects.AddUnique(Package);

		TArray<UObject*> ExportMap;
		GetObjectsWithOuter(Package,ExportMap);
		for(const auto& ExportObj:ExportMap)
		{
			for(const auto& Platform:TargetPlatforms)
			{
				ExportObj->BeginCacheForCookedPlatformData(Platform);
			}
		}
		if (GShaderCompilingManager)
		{
			GShaderCompilingManager->ProcessAsyncResults(true, false);
		}
	}
	GIsCookerLoadingPackage = false;
	
	TArray<UObject*> ObjectsToWaitForCookedPlatformData;
	ObjectsToWaitForCookedPlatformData.Reserve(65536);
	
	for (const auto& Obj : AllObjects)
	{
		bool bAllPlatformDataLoaded = true;
		bool bIsTexture = Obj->IsA(UTexture::StaticClass());
 		for (const ITargetPlatform* TargetPlatform : TargetPlatforms)
		{
			if (!bIsTexture)
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
	if (GShaderCompilingManager)
	{
		UE_LOG(LogHotPatcher, Display, TEXT("Waiting for shader compilation..."));
		while(GShaderCompilingManager->IsCompiling())
		{
			GShaderCompilingManager->ProcessAsyncResults(false, false);
			FPlatformProcess::Sleep(0.5f);
		}

		// One last process to get the shaders that were compiled at the very end
		GShaderCompilingManager->ProcessAsyncResults(false, false);
		UE_LOG(LogHotPatcher, Display, TEXT("Shader Compilated!"));
	}
		
	// Wait for all platform data to be loaded
	{
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
	TFunction<void(const FString&)> PackageSavedCallback = [](const FString&){};
	TFunction<void(const FString&,ETargetPlatform)> CookFailedCallback = [this](const FString& PackagePath,ETargetPlatform Platform)
	{
		OnCookAssetFailed(PackagePath,Platform);						
	};

	UFlibHotPatcherEditorHelper::CookAssets(SoftObjectPaths,GetSettingObject()->MultiCookerSettings.CookTargetPlatforms,PackageSavedCallback,CookFailedCallback
#if WITH_PACKAGE_CONTEXT
	,GetPlatformSavePackageContextsRaw()
#endif
	);

	UE_LOG(LogHotPatcher, Display, TEXT("Waiting Cook Assets Compilation..."));
	WaitThreadWorker = MakeShareable(new FThreadWorker(TEXT("SingleCooker_WaitCookComplete"),[this]()
		{
			UPackage::WaitForAsyncFileWrites();
		}));
	WaitThreadWorker->Execute();
	WaitThreadWorker->Join();
	
	// save bulk data manifest
	for(auto& Platform:GetSettingObject()->MultiCookerSettings.CookTargetPlatforms)
	{
		if(GetSettingObject()->MultiCookerSettings.IoStoreSettings.bStorageBulkDataInfo)
		{
#if WITH_PACKAGE_CONTEXT
			SavePlatformBulkDataManifest(Platform);
#endif
		}
	}
	
}

void USingleCookerProxy::InitShaderLibConllections()
{
	for(const auto& Platform:GetSettingObject()->MultiCookerSettings.CookTargetPlatforms)
	{
		// init shader lib collection
		if(GetSettingObject()->MultiCookerSettings.ShaderOptions.bSharedShaderLibrary)
		{
			TSharedPtr<FCookShaderCollectionProxy> PlatfromCookShaderCollection = CreateCookShaderCollectionProxyByPlatform(Platform);
			if(PlatfromCookShaderCollection.IsValid())
			{
				PlatfromCookShaderCollection->Init();
			}
		}
	}
}

void USingleCookerProxy::ShutdowShaderLibCollections()
{
	if(GetSettingObject()->MultiCookerSettings.ShaderOptions.bSharedShaderLibrary)
	{
		for(auto& CookShaderCollection:PlatformCookShaderCollectionMap)
		{
			if(CookShaderCollection.Value.IsValid())
			{
				CookShaderCollection.Value->Shutdown();
			}
		}
	}
}

bool USingleCookerProxy::DoExport()
{
	GetCookFailedAssetsCollection().MissionName = GetSettingObject()->MissionName;
	GetCookFailedAssetsCollection().MissionID = GetSettingObject()->MissionID;
	GetCookFailedAssetsCollection().CookFailedAssets.Empty();

// 	USingleCookerProxy::FShaderCollection ShaderCollection(this);
//
// 	TFunction<void(const FString&,ETargetPlatform)> CookFailedCallback = [this](const FString& PackagePath,ETargetPlatform Platform)
// 	{
// 		OnCookAssetFailed(PackagePath,Platform);						
// 	};
// 	
// 	for(const auto& Platform:GetSettingObject()->MultiCookerSettings.CookTargetPlatforms)
// 	{
// 		UFlibHotPatcherEditorHelper::CookChunkAssets(
// 							GetSettingObject()->CookAssets,
// 							TArray<ETargetPlatform>{Platform},
// 							CookFailedCallback
// 	#if WITH_PACKAGE_CONTEXT
// 							,GetPlatformSavePackageContextsRaw()
// 	#endif
// 						);
// 		
// 		if(GetSettingObject()->MultiCookerSettings.IoStoreSettings.bStorageBulkDataInfo)
// 		{
// #if WITH_PACKAGE_CONTEXT
// 			SavePlatformBulkDataManifest(Platform);
// #endif
// 		}
// 	}
// 	 
// 	WaitThreadWorker = MakeShareable(new FThreadWorker(TEXT("SingleCooker_WaitCookComplete"),[this]()
// 		{
// 			UPackage::WaitForAsyncFileWrites();
// 		}));
// 	WaitThreadWorker->Execute();
// 	WaitThreadWorker->Join();

	DoCookMission(GetSettingObject()->CookAssets);
	
	if(HasError())
	{
		FString FailedJsonString;
		UFlibPatchParserHelper::TSerializeStructAsJsonString(GetCookFailedAssetsCollection(),FailedJsonString);
		UE_LOG(LogHotPatcher,Warning,TEXT("Single Cooker Proxy %s:\n%s"),*GetSettingObject()->MissionName,*FailedJsonString);
		FString SaveTo = UFlibMultiCookerHelper::GetCookerProcFailedResultPath(GetSettingObject()->MissionName,GetSettingObject()->MissionID);
		FFileHelper::SaveStringToFile(FailedJsonString,*SaveTo);
	}
	return !HasError();
}

bool USingleCookerProxy::HasError()
{
	TArray<ETargetPlatform> TargetPlatforms;
	GetCookFailedAssetsCollection().CookFailedAssets.GetKeys(TargetPlatforms);
	return !!TargetPlatforms.Num();
}

void USingleCookerProxy::OnCookAssetFailed(const FString& PackagePath, ETargetPlatform Platform)
{
	FString PlatformName = UFlibPatchParserHelper::GetEnumNameByValue(Platform);
	UE_LOG(LogHotPatcher,Warning,TEXT("Cook %s for %s Failed!"),*PackagePath,*PlatformName);
	FAssetsCollection& AssetsCollection = GetCookFailedAssetsCollection().CookFailedAssets.FindOrAdd(Platform);
	AssetsCollection.TargetPlatform = Platform;
	AssetsCollection.Assets.AddUnique(PackagePath);
}

#if WITH_PACKAGE_CONTEXT
// // engine header
#include "UObject/SavePackage.h"

#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION > 25
#include "Serialization/BulkDataManifest.h"
#endif


void USingleCookerProxy::InitPlatformPackageContexts()
{
	ITargetPlatformManagerModule& TPM = GetTargetPlatformManagerRef();
	const TArray<ITargetPlatform*>& TargetPlatforms = TPM.GetTargetPlatforms();
	TArray<ITargetPlatform*> CookPlatforms;
	const FString ProjectPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
	for (ITargetPlatform *TargetPlatform : TargetPlatforms)
	{
		ETargetPlatform Platform;
		UFlibPatchParserHelper::GetEnumValueByName(TargetPlatform->PlatformName(),Platform);
		if (GetSettingObject()->MultiCookerSettings.CookTargetPlatforms.Contains(Platform))
		{
			CookPlatforms.AddUnique(TargetPlatform);
			const FString PlatformString = TargetPlatform->PlatformName();
			PlatformSavePackageContexts.Add(Platform,MakeShareable(UFlibHotPatcherEditorHelper::CreateSaveContext(TargetPlatform,GetSettingObject()->MultiCookerSettings.IoStoreSettings.bIoStore)));
		}
	}
}

TMap<ETargetPlatform, FSavePackageContext*> USingleCookerProxy::GetPlatformSavePackageContextsRaw() const

{
	TMap<ETargetPlatform,FSavePackageContext*> result;
	TArray<ETargetPlatform> Keys;
	GetPlatformSavePackageContexts().GetKeys(Keys);
	for(const auto& Key:Keys)
	{
		result.Add(Key,GetPlatformSavePackageContexts().Find(Key)->Get());
	}
	return result;
}

bool USingleCookerProxy::SavePlatformBulkDataManifest(ETargetPlatform Platform)
{
	bool bRet = false;
	if(!GetPlatformSavePackageContexts().Contains(Platform))
		return bRet;
	TSharedPtr<FSavePackageContext> PackageContext = *GetPlatformSavePackageContexts().Find(Platform);
#if ENGINE_MAJOR_VERSION < 5 && ENGINE_MINOR_VERSION > 25
	if (PackageContext != nullptr && PackageContext->BulkDataManifest != nullptr)
	{
		PackageContext->BulkDataManifest->Save();
		bRet = true;
	}
#endif
	return bRet;
}

#endif