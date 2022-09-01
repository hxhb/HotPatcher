#include "GameFeature/GameFeatureProxy.h"

#include <Interfaces/IPluginManager.h>


#include "CreatePatch/PatcherProxy.h"

bool UGameFeatureProxy::DoExport()
{
	PatchSettings = MakeShareable(new FExportPatchSettings);
	for(const auto& FeatureName:GetSettingObject()->FeatureNames)
	{
		if(GetSettingObject()->IsSaveConfig())
		{
			FString SaveToFile = FPaths::Combine(GetSettingObject()->GetSaveAbsPath(),FString::Printf(TEXT("%s_GameFeatureConfig.json"),*FeatureName));
			FString SerializedJsonStr;
			THotPatcherTemplateHelper::TSerializeStructAsJsonString(*GetSettingObject(),SerializedJsonStr);
			FFileHelper::SaveStringToFile(SerializedJsonStr, *SaveToFile);
		}
		
		// make patch setting
		{
			PatchSettings->bByBaseVersion = false;
			PatchSettings->VersionId = FeatureName;
			FDirectoryPath FeaturePluginPath;
			FeaturePluginPath.Path = FString::Printf(TEXT("/%s"),*PatchSettings->VersionId);
			
			PatchSettings->GetAssetScanConfigRef().AssetIncludeFilters.Add(FeaturePluginPath);

			FPlatformExternAssets PlatformExternAssets;
			{
				PlatformExternAssets.TargetPlatform = ETargetPlatform::AllPlatforms;
				FExternFileInfo FeaturePlugin;
				
				if(UFlibPatchParserHelper::GetPluginPakPathByName(PatchSettings->VersionId,FeaturePlugin.FilePath.FilePath,FeaturePlugin.MountPath))
				{
					FeaturePlugin.Type = EPatchAssetType::NEW;
					PlatformExternAssets.AddExternFileToPak.Add(FeaturePlugin);

					TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(FeatureName);
					if(Plugin)
					{
						for(const auto& NonContentDir:GetSettingObject()->NonContentDirs)
						{
							FString PluginConfigDir = FPaths::ConvertRelativePathToFull(FPaths::Combine(Plugin->GetBaseDir(),NonContentDir));
							if(FPaths::DirectoryExists(PluginConfigDir))
							{
								FExternDirectoryInfo ConfigDir;
								ConfigDir.DirectoryPath.Path = PluginConfigDir;
								ConfigDir.MountPoint = FPaths::Combine(Plugin->GetBaseDir(),NonContentDir);
								PlatformExternAssets.AddExternDirectoryToPak.Add(ConfigDir);
							}
						}
					}
				}
			}
			PatchSettings->AddExternAssetsToPlatform.Add(PlatformExternAssets);
			PatchSettings->bCookPatchAssets = GetSettingObject()->bCookPatchAssets;
			
			{
				PatchSettings->SerializeAssetRegistryOptions = GetSettingObject()->SerializeAssetRegistryOptions;
				PatchSettings->SerializeAssetRegistryOptions.AssetRegistryMountPointRegular = FString::Printf(TEXT("%s[%s]"),AS_PLUGINDIR_MARK,*FeatureName);
				PatchSettings->SerializeAssetRegistryOptions.AssetRegistryNameRegular = FString::Printf(TEXT("AssetRegistry.bin"));
			}
			{
				PatchSettings->CookShaderOptions = GetSettingObject()->CookShaderOptions;
				PatchSettings->CookShaderOptions.bSharedShaderLibrary = true;
				PatchSettings->CookShaderOptions.bNativeShader = true;
				PatchSettings->CookShaderOptions.ShaderLibMountPointRegular = FString::Printf(TEXT("%s[%s]"),AS_PLUGINDIR_MARK,*FeatureName);
			}
			PatchSettings->IoStoreSettings = GetSettingObject()->IoStoreSettings;
			PatchSettings->EncryptSettings = GetSettingObject()->EncryptSettings;
			PatchSettings->PakTargetPlatforms.Append(GetSettingObject()->TargetPlatforms);
			PatchSettings->SavePath.Path = GetSettingObject()->GetSaveAbsPath();
			PatchSettings->bStorageNewRelease = false;
			PatchSettings->bStorageConfig = true;
		}
		PatcherProxy = NewObject<UPatcherProxy>();
		PatcherProxy->AddToRoot();
		PatcherProxy->Init(PatchSettings.Get());
		PatcherProxy->DoExport();

	}
	return Super::DoExport();
}
