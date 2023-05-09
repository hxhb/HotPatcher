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
		FName AssetPathName = FName(*Package->GetPathName());
		LoadedPackages.Add(AssetPathName);
		if(!ExisitAssets.Contains(AssetPathName))
		{
			if(FPackageName::DoesPackageExist(AssetPathName.ToString()))
			{
				UE_LOG(LogHotPatcher,Display,TEXT("[PackageTracker] Add %s"),*AssetPathName.ToString());
				PackagesPendingSave.Add(AssetPathName);
			}
			else
			{
				UE_LOG(LogHotPatcher,Display,TEXT("[PackageTracker] %s is not valid package!"),*AssetPathName.ToString());
			}
		}
	}
	virtual void OnPackageDeleted(UPackage* Package) override
	{
		FName AssetPathName = FName(*Package->GetPathName());
		if(PackagesPendingSave.Contains(AssetPathName))
		{
			PackagesPendingSave.Remove(AssetPathName);
		}
	}
	
public:
	// typedef TSet<UPackage*> PendingPackageSet;
	const TSet<FName>& GetPendingPackageSet()const {return PackagesPendingSave; }
protected:
	TSet<FName>	 PackagesPendingSave;
	TSet<FName>& ExisitAssets;
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