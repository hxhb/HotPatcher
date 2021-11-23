#include "Cooker/MultiCooker/SingleCookerProxy.h"
#include "FlibHotPatcherEditorHelper.h"
#include "ShaderPatch/FlibShaderCodeLibraryHelper.h"

void USingleCookerProxy::Init()
{
	InitPlatformPackageContexts();
	Super::Init();
}

void USingleCookerProxy::Shutdown()
{
	Super::Shutdown();
}

bool USingleCookerProxy::DoExport()
{
	for(const auto& Platform:GetSettingObject()->MultiCookerSettings.CookTargetPlatforms)
	{
		TSharedPtr<FCookShaderCollectionProxy> CookShaderCollection;
		FString PlatformName = UFlibPatchParserHelper::GetEnumNameByValue(Platform);
		if(GetSettingObject()->MultiCookerSettings.ShaderOptions.bSharedShaderLibrary)
		{
			FString SavePath = FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("MultiCookDir"));
			FString ActualLibraryName = UFlibShaderCodeLibraryHelper::GenerateShaderCodeLibraryName(FApp::GetProjectName(),false);
			FString ShaderLibraryName = FString::Printf(TEXT("%s_Process_%d"),FApp::GetProjectName(),1);
			CookShaderCollection = MakeShareable(
				new FCookShaderCollectionProxy(
					PlatformName,
					ShaderLibraryName,
					GetSettingObject()->MultiCookerSettings.ShaderOptions.bNativeShader,
					SavePath
					));
			CookShaderCollection->Init();
		}

		UFlibHotPatcherEditorHelper::CookChunkAssets(
							GetSettingObject()->CookAssets,
							TArray<ETargetPlatform>{Platform}
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
		
		CookShaderCollection->Shutdown();
	}
	
	
	
	return Super::DoExport();
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