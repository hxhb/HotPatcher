
#include "CoreMinimal.h"
#include "AssetData.h"
#include "ETargetPlatform.h"

struct FCookManager
{
	FCookManager()=default;
	virtual ~FCookManager()=default;
	virtual void Init();
	virtual void Shutdown();

	static FCookManager& Get()
	{
		static FCookManager Manager;
		return Manager;
	}
	
	struct FCookPackageInfo
	{
		FAssetData AssetData;
		FString PackageName;
		TArray<ETargetPlatform> CookPlatforms;
		TFunction<void(ETargetPlatform,const FString&,const FString&)> Callback;
		TArray<ETargetPlatform> GetCookPlatforms()const;
		TArray<FString> GetCookPlatformsString()const;
		TMap<ETargetPlatform,FString> GetCookedSavePaths()const;
	};
	struct FCookMission
	{
		friend FCookManager;
		TArray<FCookManager::FCookPackageInfo> MissionPackages;
		TFunction<void(bool)> Callback;
	private:
		int32 CookedCount = 0;
	};
	
	virtual FString GetCookedAssetPath(const FString& PackageName,ETargetPlatform Platform);
	void OnPackageSavedEvent(const FString& InFilePath,UObject* Object);
	int32 AddCookMission(const FCookMission& InCookMission,TFunction<void(TArray<FCookManager::FCookPackageInfo>)> FaildPackagesCallback = [](TArray<FCookManager::FCookPackageInfo>){});
private:
	TArray<FCookMission> CookMissions;
};