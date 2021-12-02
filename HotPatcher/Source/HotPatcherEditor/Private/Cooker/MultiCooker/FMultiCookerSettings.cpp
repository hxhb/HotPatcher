#include "Cooker/MultiCooker/FMultiCookerSettings.h"

#include "FlibHotPatcherEditorHelper.h"
#include "Cooker/MultiCooker/MultiCookScheduler.h"

FMultiCookerSettings::FMultiCookerSettings()
{
	bImportProjectSettings = false;
	ShaderOptions.bSharedShaderLibrary = true;
	ShaderOptions.bNativeShader = true;
	ShaderOptions.bMergeShaderLibrary = true;
	bSerializeAssetRegistry = true;
	Scheduler = UMultiCookScheduler::StaticClass();
	bStorageConfig = true;
	SavePath.Path = TEXT("[PROJECTDIR]/Saved/HotPatcher/MultiCooker");
	ForceSkipContentRules.Append(UFlibPatchParserHelper::GetDefaultForceSkipContentDir());
	bStandaloneMode = false;
}

bool FMultiCookerSettings::IsValidConfig() const
{
	return (IncludeSpecifyAssets.Num() || AssetIncludeFilters.Num() || bImportProjectSettings) && CookTargetPlatforms.Num();
}

void FMultiCookerSettings::ImportProjectSettings()
{
	FProjectPackageAssetCollection AssetCollection = UFlibHotPatcherEditorHelper::ImportProjectSettingsPackages();
	AssetIncludeFilters.Append(AssetCollection.DirectoryPaths);

	for(const auto& Asset:AssetCollection.NeedCookPackages)
	{
		FPatcherSpecifyAsset CurrentAsset;
		CurrentAsset.Asset = Asset;
		CurrentAsset.bAnalysisAssetDependencies = true;
		CurrentAsset.AssetRegistryDependencyTypes = {EAssetRegistryDependencyTypeEx::Packages};
		IncludeSpecifyAssets.Add(CurrentAsset);
	}
}
