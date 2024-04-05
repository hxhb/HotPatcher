#pragma once
#include "FlibAssetManageHelper.h"
#include "CoreMinimal.h"
#include "UObject/UObjectArray.h"
#include "HotPatcherLog.h"

struct FPackageTrackerBase : public FUObjectArray::FUObjectCreateListener, public FUObjectArray::FUObjectDeleteListener
{
	FPackageTrackerBase()
	{
		// for (TObjectIterator<UPackage> It; It; ++It)
		// {
		// 	UPackage* Package = *It;
		//
		// 	if (Package->GetOuter() == nullptr)
		// 	{
		// 		LoadedPackages.Add(Package);
		// 	}
		// }

		GUObjectArray.AddUObjectDeleteListener(this);
		GUObjectArray.AddUObjectCreateListener(this);
	}

	virtual ~FPackageTrackerBase()
	{
		GUObjectArray.RemoveUObjectDeleteListener(this);
		GUObjectArray.RemoveUObjectCreateListener(this);
	}

	virtual void NotifyUObjectCreated(const class UObjectBase *Object, int32 Index) override
	{
		if (Object->GetClass() == UPackage::StaticClass())
		{
			auto Package = const_cast<UPackage*>(static_cast<const UPackage*>(Object));

			if (Package->GetOuter() == nullptr && !Package->GetFName().IsNone())
			{
				FName AssetPathName = FName(*Package->GetPathName());
				LoadedPackages.Add(AssetPathName);
				
				OnPackageCreated(Package);
			}
		}
	}

	virtual void NotifyUObjectDeleted(const class UObjectBase *Object, int32 Index) override
	{
		if (Object->GetClass() == UPackage::StaticClass())
		{
			auto Package = const_cast<UPackage*>(static_cast<const UPackage*>(Object));

			if(!Package->GetFName().IsNone())
			{
				
				OnPackageDeleted(Package);
			}
		}
	}

	virtual void OnUObjectArrayShutdown()
	{
		GUObjectArray.RemoveUObjectDeleteListener(this);
		GUObjectArray.RemoveUObjectCreateListener(this);
	}
	virtual void OnPackageCreated(UPackage* Package){};
	virtual void OnPackageDeleted(UPackage* Package){};
	virtual const TSet<FName>& GetLoadedPackages()const{ return LoadedPackages; }
	
protected:
	TSet<FName>		LoadedPackages;
};

struct FPackageTracker : public FPackageTrackerBase
{
	FPackageTracker(TSet<FName>& InExisitAssets):ExisitAssets(InExisitAssets){}

	virtual ~FPackageTracker(){}

	virtual void OnPackageCreated(UPackage* Package) override
	{
		if(!IsTracking())
		{
			return;
		}
		FName AssetPathName = FName(*Package->GetPathName());
		LoadedPackages.Add(AssetPathName);
		if(!ExisitAssets.Contains(AssetPathName))
		{
			FString AssetPathNameStr = AssetPathName.ToString();
			if(FPackageName::DoesPackageExist(AssetPathNameStr)
#if WITH_UE5
				 && !AssetPathNameStr.StartsWith(TEXT("/Game/__ExternalActors__")) &&
				!AssetPathNameStr.StartsWith(TEXT("/Game/__ExternalObjects__"))
#endif
				)
			{
				UE_LOG(LogHotPatcher,Display,TEXT("[PackageTracker] Add %s"),*AssetPathNameStr);
				AddTrackPackage(AssetPathName);
			}
			else
			{
				UE_LOG(LogHotPatcher,Verbose,TEXT("[PackageTracker] %s is not valid package!"),*AssetPathNameStr);
			}
		}
	}
	virtual void OnPackageDeleted(UPackage* Package) override
	{
		FName AssetPathName = FName(*Package->GetPathName());
		RemoveTrackPackage(AssetPathName);
	}
	
public:
	// typedef TSet<UPackage*> PendingPackageSet;
	const TSet<FName>& GetAdditionalPackageSet(){ return AdditionalPackageSet; }
	const TSet<FName>& GetPendingPackageSet()const {return PackagesPendingSave; }
	void CleanPaddingSet(){ PackagesPendingSave.Empty(); }
	void SetTracking(bool bEnable){ bIsTracking = bEnable; }
	bool IsTracking()const { return bIsTracking; }
protected:
	void AddTrackPackage(FName PackageName)
	{
		if(!AdditionalPackageSet.Contains(PackageName))
		{
			AdditionalPackageSet.Add(PackageName);
			PackagesPendingSave.Add(PackageName);
		}
	}
	void RemoveTrackPackage(FName PackageName)
	{
		AdditionalPackageSet.Remove(PackageName);
		PackagesPendingSave.Remove(PackageName);
	}
	TSet<FName> AdditionalPackageSet;
	TSet<FName>	 PackagesPendingSave;
	TSet<FName>& ExisitAssets;
	bool bIsTracking = true;
};

struct FClassesPackageTracker : public FPackageTrackerBase
{
	virtual void OnPackageCreated(UPackage* Package) override
	{
		FName ClassName = UFlibAssetManageHelper::GetAssetTypeByPackage(Package);
		
		if(!ClassMapping.Contains(ClassName))
		{
			ClassMapping.Add(ClassName,TArray<UPackage*>{});
		}
		ClassMapping.Find(ClassName)->AddUnique(Package);
	};
	virtual void OnPackageDeleted(UPackage* Package) override
	{
		FName ClassName = UFlibAssetManageHelper::GetAssetTypeByPackage(Package);
		if(ClassMapping.Contains(ClassName))
		{
			ClassMapping.Find(ClassName)->Remove(Package);
		}
	}
	TArray<UPackage*> GetPackagesByClassName(FName ClassName)
	{
		TArray<UPackage*> result;
		if(ClassMapping.Contains(ClassName))
		{
			result = *ClassMapping.Find(ClassName);
		}
		return result;
	}
protected:
	TMap<FName,TArray<UPackage*>> ClassMapping;
};