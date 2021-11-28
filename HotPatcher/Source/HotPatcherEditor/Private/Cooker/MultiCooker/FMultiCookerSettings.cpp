#include "Cooker/MultiCooker/FMultiCookerSettings.h"
#include "Cooker/MultiCooker/MultiCookScheduler.h"


FMultiCookerSettings::FMultiCookerSettings()
{
	ShaderOptions.bSharedShaderLibrary = true;
	ShaderOptions.bNativeShader = true;
	bSerializeAssetRegistry = true;
	CookScheduler = UMultiCookScheduler::StaticClass();
	bStorageConfig = true;
	SavePath.Path = TEXT("[PROJECTDIR]/Saved/HotPatcher/MultiCooker");
	bStandaloneMode = false;
}

bool FMultiCookerSettings::IsValidConfig() const
{
	return (IncludeSpecifyAssets.Num() || AssetIncludeFilters.Num() )&& CookTargetPlatforms.Num();
}