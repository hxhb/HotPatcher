#include "Cooker/MultiCooker/FMultiCookerSettings.h"

#include "FlibHotPatcherCoreHelper.h"
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
	bStandaloneMode = false;
}

FMultiCookerSettings* FMultiCookerSettings::Get()
{
	static FMultiCookerSettings Settings;
	return &Settings;
}

bool FMultiCookerSettings::IsValidConfig() const
{
	return (IncludeSpecifyAssets.Num() || AssetIncludeFilters.Num() || bImportProjectSettings) && CookTargetPlatforms.Num();
}

bool FMultiCookerSettings::IsSkipAssets(FName LongPacakgeName)
{
	return GetAllSkipContents().Contains(LongPacakgeName.ToString());
}

bool FSingleCookerSettings::IsSkipAsset(const FString& PackageName)
{
	bool bRet = false;
	for(const auto& SkipCookContent:SkipCookContents)
	{
		if(PackageName.StartsWith(SkipCookContent))
		{
			bRet = true;
			break;
		}
	}
	return bRet;
}
