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

	FName LongPackageNameToPackagePath(FString InLongPackageName)
	{
		FString AssetName;
		{
			int32 FoundIndex;
			InLongPackageName.FindLastChar('/', FoundIndex);
			if (FoundIndex != INDEX_NONE)
			{
				AssetName = UKismetStringLibrary::GetSubstring(InLongPackageName, FoundIndex + 1, InLongPackageName.Len() - FoundIndex);
			}
		}
		FString OutPackagePath = InLongPackageName + TEXT(".") + AssetName;
		return FName(OutPackagePath);
	}
	virtual void NotifyUObjectCreated(const class UObjectBase *Object, int32 Index) override
	{
		if (Object->GetClass() == UPackage::StaticClass())
		{
			auto Package = const_cast<UPackage*>(static_cast<const UPackage*>(Object));

			if (Package->GetOuter() == nullptr && !Package->GetFName().IsNone())
			{
				FName AssetPathName = LongPackageNameToPackagePath(Package->GetName());
				if(!ExisitAssets.Contains(AssetPathName))
				{
					PackagesPendingSave.Add(AssetPathName);
				}
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
				FName AssetPathName = LongPackageNameToPackagePath(Package->GetName());
				if(PackagesPendingSave.Contains(AssetPathName))
				{
					PackagesPendingSave.Remove(AssetPathName);
				}
			}
		}
	}
	
	virtual void OnUObjectArrayShutdown() override
	{
		GUObjectArray.RemoveUObjectDeleteListener(this);
		GUObjectArray.RemoveUObjectCreateListener(this);
	}
public:
	// typedef TSet<UPackage*> PendingPackageSet;
	const TSet<FName>& GetPendingPackageSet()const {return PackagesPendingSave; }
protected:
	TSet<FName>		PackagesPendingSave;
	TSet<FName>& ExisitAssets;
};