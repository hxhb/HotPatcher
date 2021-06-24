#include "CookManager.h"
#include "FlibPatchParserHelper.h"
#include "FlibHotPatcherEditorHelper.h"

void FCookManager::Init()
{
	UPackage::PackageSavedEvent.AddRaw(this,&FCookManager::OnPackageSavedEvent);
}

void FCookManager::Shutdown(){}
FString FCookManager::GetCookedAssetPath(const FString& PackageName,ETargetPlatform Platform)
{
	FString CookedDir = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Cooked")));
	return UFlibHotPatcherEditorHelper::GetCookAssetsSaveDir(CookedDir,PackageName, UFlibPatchParserHelper::GetEnumNameByValue(Platform));
}

void FCookManager::OnPackageSavedEvent(const FString& InFilePath,UObject* Object)
{
	TArray<int32> RemoveMissionIndexs;
	for(int32 index = 0; index<CookMissions.Num(); index++)
	{
		for (const auto& CookInfo:CookMissions[index].MissionPackages)
		{
			for(const auto& CookedItem:CookInfo.GetCookedSavePaths())
			{
				if(CookedItem.Value.Equals(InFilePath))
				{
					CookMissions[index].CookedCount++;
					if(CookInfo.Callback)
						CookInfo.Callback(CookedItem.Key,CookInfo.PackageName,InFilePath);
					if(CookMissions[index].CookedCount == CookMissions[index].MissionPackages.Num())
					{
						if(CookMissions[index].Callback)
							CookMissions[index].Callback(true);
						RemoveMissionIndexs.AddUnique(index);
					}
				}
			}
		}
	}
	for(const auto& removeIndex:RemoveMissionIndexs)
	{
		CookMissions.RemoveAt(removeIndex);
	}
}
	
int32 FCookManager::AddCookMission(const FCookMission& InCookMission,TFunction<void(TArray<FCookManager::FCookPackageInfo>)> FaildPackagesCallback)
{
	int32 index=-1;
	index = CookMissions.Add(InCookMission);
	TArray<FCookManager::FCookPackageInfo> FaildPackages;
	for(const auto& Package:InCookMission.MissionPackages)
	{
		if(!UFlibHotPatcherEditorHelper::CookPackage(Package.AssetData,Package.AssetData.GetPackage(),Package.GetCookPlatformsString()))
		{
			FaildPackages.Add(Package);
		}
	}
	if(!!FaildPackages.Num())
	{
		FaildPackagesCallback(FaildPackages);
		if(InCookMission.Callback)
		{
			InCookMission.Callback(false);
		}
		CookMissions.RemoveAt(index);
		index = -1;
	}
	return index;
}


TArray<ETargetPlatform> FCookManager::FCookPackageInfo::GetCookPlatforms() const
{
	return CookPlatforms;
}

TArray<FString> FCookManager::FCookPackageInfo::GetCookPlatformsString()const
{
	TArray<FString> Result;
	for(const auto& Platform:GetCookPlatforms())
	{
		Result.Add(UFlibPatchParserHelper::GetEnumNameByValue(Platform));
	}
	return Result;
}

TMap<ETargetPlatform,FString> FCookManager::FCookPackageInfo::GetCookedSavePaths()const
{
	TMap<ETargetPlatform,FString> result;
	for(const auto&Platform:CookPlatforms)
	{
		FString CookedDir = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Cooked")));
		result.Add(Platform,UFlibHotPatcherEditorHelper::GetCookAssetsSaveDir(CookedDir,PackageName, UFlibPatchParserHelper::GetEnumNameByValue(Platform)));
	}
	return result;
}