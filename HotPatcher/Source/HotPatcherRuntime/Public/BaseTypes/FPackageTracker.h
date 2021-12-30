#pragma once
#include "FlibAssetManageHelper.h"
#include "CoreMinimal.h"
#include "UObject/UObjectArray.h"
#include "HAL/PlatformFilemanager.h"
#include "HAL/FileManager.h"

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

	virtual void OnUObjectArrayShutdown() override
	{
		GUObjectArray.RemoveUObjectDeleteListener(this);
		GUObjectArray.RemoveUObjectCreateListener(this);
	}
	virtual void OnPackageCreated(UPackage* Package) = 0;
	virtual void OnPackageDeleted(UPackage* Package) = 0;
};

struct FPackageTracker : public FPackageTrackerBase
{
	FPackageTracker(TSet<FName>& InExisitAssets):ExisitAssets(InExisitAssets)
	{}

	virtual ~FPackageTracker(){}

	virtual void OnPackageCreated(UPackage* Package) override
	{
		FName AssetPathName = FName(UFlibAssetManageHelper::LongPackageNameToPackagePath(Package->GetName()));
		if(!ExisitAssets.Contains(AssetPathName))
		{
			PackagesPendingSave.Add(AssetPathName);
		}
	}
	virtual void OnPackageDeleted(UPackage* Package) override
	{
		FName AssetPathName = FName(UFlibAssetManageHelper::LongPackageNameToPackagePath(Package->GetName()));
		if(PackagesPendingSave.Contains(AssetPathName))
		{
			PackagesPendingSave.Remove(AssetPathName);
		}
	}
	
public:
	// typedef TSet<UPackage*> PendingPackageSet;
	const TSet<FName>& GetPendingPackageSet()const {return PackagesPendingSave; }
protected:
	TSet<FName>		PackagesPendingSave;
	TSet<FName>& ExisitAssets;
};
