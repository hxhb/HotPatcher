#include "Cooker/MultiCooker/FSingleCookerSettings.h"
#include "FlibPatchParserHelper.h"

FSingleCookerSettings::FSingleCookerSettings()
{
	OverrideNumberOfAssetsPerFrame.Add(UWorld::StaticClass(),2);
}

FString FSingleCookerSettings::GetStorageCookedAbsDir() const
{
	return UFlibPatchParserHelper::ReplaceMark(StorageCookedDir);
}

FString FSingleCookerSettings::GetStorageMetadataAbsDir() const
{
	return UFlibPatchParserHelper::ReplaceMark(StorageMetadataDir);
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
