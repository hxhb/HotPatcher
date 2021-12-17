#pragma once

#include "CoreMinimal.h"
#include "UObject/UObjectArray.h"
#include "HAL/PlatformFilemanager.h"
#include "HAL/FileManager.h"

struct FPackageTracker : public FUObjectArray::FUObjectCreateListener, public FUObjectArray::FUObjectDeleteListener
{
	FPackageTracker(TSet<FName>& InExisitAssets):ExisitAssets(InExisitAssets)
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

	~FPackageTracker()
	{
		GUObjectArray.RemoveUObjectDeleteListener(this);
		GUObjectArray.RemoveUObjectCreateListener(this);
	}

	virtual void NotifyUObjectCreated(const class UObjectBase *Object, int32 Index) override
	{
		if (Object->GetClass() == UPackage::StaticClass())
		{
			auto Package = const_cast<UPackage*>(static_cast<const UPackage*>(Object));

			if (Package->GetOuter() == nullptr)
			{
				if(!ExisitAssets.Contains(Package->GetFName()))
				{
					PackagesPendingSave.Add(Package);
				}
			}
		}
	}

	virtual void NotifyUObjectDeleted(const class UObjectBase *Object, int32 Index) override
	{
		if (Object->GetClass() == UPackage::StaticClass())
		{
			auto Package = const_cast<UPackage*>(static_cast<const UPackage*>(Object));

			if(PackagesPendingSave.Contains(Package))
			{
				PackagesPendingSave.Remove(Package);
			}
		}
	}
	
	virtual void OnUObjectArrayShutdown() override
	{
		GUObjectArray.RemoveUObjectDeleteListener(this);
		GUObjectArray.RemoveUObjectCreateListener(this);
	}
public:
	typedef TSet<UPackage*> PendingPackageSet;
	const PendingPackageSet& GetPendingPackageSet()const {return PackagesPendingSave; }
protected:
	PendingPackageSet		PackagesPendingSave;
	TSet<FName>& ExisitAssets;
};