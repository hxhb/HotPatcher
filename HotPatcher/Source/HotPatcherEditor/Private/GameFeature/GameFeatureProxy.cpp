#include "GameFeature/GameFeatureProxy.h"

#include <Interfaces/IPluginManager.h>

#include "HotPatcherEditor.h"
#include "CreatePatch/PatcherProxy.h"

bool UGameFeatureProxy::DoExport()
{
	PatchSettings = MakeShareable(new FExportPatchSettings);
	// make patch setting
	{
		PatchSettings->bByBaseVersion = false;
		PatchSettings->VersionId = GetSettingObject()->FeatureName;
		FDirectoryPath FeaturePluginPath;
		FeaturePluginPath.Path = FString::Printf(TEXT("/%s"),*PatchSettings->VersionId);
		
		PatchSettings->AssetIncludeFilters.Add(FeaturePluginPath);

		FPlatformExternAssets PlatformExternAssets;
		{
			PlatformExternAssets.TargetPlatform = ETargetPlatform::AllPlatforms;
			FExternFileInfo FeaturePlugin;
			
			if(UFlibPatchParserHelper::GetPluginPakPathByName(PatchSettings->VersionId,FeaturePlugin.FilePath.FilePath,FeaturePlugin.MountPath))
			{
				FeaturePlugin.Type = EPatchAssetType::NEW;
				PlatformExternAssets.AddExternFileToPak.Add(FeaturePlugin);
			}
		}
		PatchSettings->AddExternAssetsToPlatform.Add(PlatformExternAssets);
		PatchSettings->bCookPatchAssets = GetSettingObject()->bCookPatchAssets;
		
		{
			PatchSettings->SerializeAssetRegistryOptions = GetSettingObject()->SerializeAssetRegistryOptions;
			PatchSettings->SerializeAssetRegistryOptions.AssetRegistryMountPointRegular = FString::Printf(TEXT("%s[%s]"),AS_PLUGINDIR_MARK,*GetSettingObject()->FeatureName);
			PatchSettings->SerializeAssetRegistryOptions.AssetRegistryNameRegular = FString::Printf(TEXT("AssetRegistry.bin"));
		}
		{
			PatchSettings->CookShaderOptions = GetSettingObject()->CookShaderOptions;
			PatchSettings->CookShaderOptions.bSharedShaderLibrary = true;
			PatchSettings->CookShaderOptions.bNativeShader = true;
			PatchSettings->CookShaderOptions.ShderLibMountPointRegular = FString::Printf(TEXT("%s[%s]"),AS_PLUGINDIR_MARK,*GetSettingObject()->FeatureName);
		}
		PatchSettings->PakTargetPlatforms.Append(GetSettingObject()->TargetPlatforms);
		PatchSettings->SavePath.Path = GetSettingObject()->GetSaveAbsPath();
		PatchSettings->bStorageNewRelease = false;
		PatchSettings->bStorageConfig = true;
	}
	UPatcherProxy* PatcherProxy = NewObject<UPatcherProxy>();
	PatcherProxy->AddToRoot();
	PatcherProxy->SetProxySettings(PatchSettings.Get());
	PatcherProxy->DoExport();

	if(GetSettingObject()->IsSaveConfig())
	{
		FString SaveToFile = FPaths::Combine(GetSettingObject()->GetSaveAbsPath(),FString::Printf(TEXT("%s_GameFeatureConfig.json"),*GetSettingObject()->FeatureName));
		FString SerializedJsonStr;
		UFlibPatchParserHelper::TSerializeStructAsJsonString(*GetSettingObject(),SerializedJsonStr);
		FFileHelper::SaveStringToFile(SerializedJsonStr, *SaveToFile);
	}
	return Super::DoExport();
}
