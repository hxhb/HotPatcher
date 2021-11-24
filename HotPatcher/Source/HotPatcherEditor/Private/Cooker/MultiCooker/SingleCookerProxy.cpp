#include "Cooker/MultiCooker/SingleCookerProxy.h"
#include "FlibHotPatcherEditorHelper.h"
#include "FlibMultiCookerHelper.h"
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

bool USingleCookerProxy::DoExport()
{
	GetCookFailedAssetsCollection().MissionName = GetSettingObject()->MissionName;
	GetCookFailedAssetsCollection().MissionID = GetSettingObject()->MissionID;
	GetCookFailedAssetsCollection().CookFailedAssets.Empty();
	
	for(const auto& Platform:GetSettingObject()->MultiCookerSettings.CookTargetPlatforms)
	{
		if(GetSettingObject()->MultiCookerSettings.ShaderOptions.bSharedShaderLibrary)
		{
			TSharedPtr<FCookShaderCollectionProxy> PlatfromCookShaderCollection = CreateCookShaderCollectionProxyByPlatform(Platform);
			if(PlatfromCookShaderCollection.IsValid())
			{
				PlatfromCookShaderCollection->Init();
			}
		}
		
		TFunction<void(const FString&,ETargetPlatform)> CookFailedCallback = [this](const FString& PackagePath,ETargetPlatform Platform)
		{
			OnCookAssetFailed(PackagePath,Platform);						
		};
		
		UFlibHotPatcherEditorHelper::CookChunkAssets(
							GetSettingObject()->CookAssets,
							TArray<ETargetPlatform>{Platform},
							CookFailedCallback
	#if WITH_PACKAGE_CONTEXT
							,GetPlatformSavePackageContextsRaw()
	#endif
						);
		if(GetSettingObject()->MultiCookerSettings.IoStoreSettings.bStorageBulkDataInfo)
		{
#if WITH_PACKAGE_CONTEXT
			SavePlatformBulkDataManifest(Platform);
#endif
		}
	}
	
	WaitThreadWorker = MakeShareable(new FThreadWorker(TEXT("SingleCooker_WaitCookComplete"),[this]()
		{
			UPackage::WaitForAsyncFileWrites();
		}));
	WaitThreadWorker->Execute();
	WaitThreadWorker->Join();

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