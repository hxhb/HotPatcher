#include "BaseTypes/FAssetScanConfig.h"

bool FAssetScanConfig::IsMatchForceSkip(const FSoftObjectPath& ObjectPath,FString& OutReason)
{
	SCOPED_NAMED_EVENT_TEXT("IsMatchForceSkip",FColor::Red);
	bool bSkip = false;
	if(bForceSkipContent)
	{
		bool bSkipAsset = ForceSkipAssets.Contains(ObjectPath);
		if(bSkipAsset)
		{
			OutReason = FString::Printf(TEXT("IsForceSkipAsset"));
		}
		bool bSkipDir = false;
		for(const auto& ForceSkipDir:ForceSkipContentRules)
		{
			if(ObjectPath.GetLongPackageName().StartsWith(ForceSkipDir.Path))
			{
				bSkipDir = true;
				OutReason = FString::Printf(TEXT("ForceSkipDir %s"),*ForceSkipDir.Path);
				break;
			}
		}
		bool bSkipClasses = false;
		FName AssetClassesName = UFlibAssetManageHelper::GetAssetType(ObjectPath);
		for(const auto& Classes:ForceSkipClasses)
		{
			if(Classes->GetFName().IsEqual(AssetClassesName))
			{
				OutReason = FString::Printf(TEXT("ForceSkipClasses %s"),*Classes->GetName());
				bSkipClasses = true;
				break;
			}
		}
		bSkip = bSkipAsset || bSkipDir || bSkipClasses;
	}
	return bSkip;
}
