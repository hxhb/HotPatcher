// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Resources/Version.h"

#if WITH_EDITOR && ENGINE_MAJOR_VERSION > 4

#include "CookPackageSplitter.h"
#include "Engine/World.h"
#include "UObject/WeakObjectPtrTemplates.h"

class FHotWorldPartitionCookPackageSplitter : public ICookPackageSplitter
{
public:
	//~ Begin of ICookPackageSplitter
	static bool ShouldSplit(UObject* SplitData);

	FHotWorldPartitionCookPackageSplitter();
	virtual ~FHotWorldPartitionCookPackageSplitter();

	virtual TArray<ICookPackageSplitter::FGeneratedPackage> GetGenerateList(const UPackage* OwnerPackage, const UObject* OwnerObject) override;
	virtual bool TryPopulatePackage(const UPackage* OwnerPackage, const UObject* OwnerObject,
		const ICookPackageSplitter::FGeneratedPackageForPopulate& GeneratedPackage, bool bWasOwnerReloaded) override;
	virtual void PreSaveGeneratorPackage(UPackage* OwnerPackage, UObject* OwnerObject,
		const TArray<ICookPackageSplitter::FGeneratedPackageForPreSave>& GeneratedPackages) override;
	//~ End of ICookPackageSplitter
	static ICookPackageSplitter* CreateInstance(UObject* SplitData) { return new FHotWorldPartitionCookPackageSplitter(); }

private:
	const UWorld* ValidateDataObject(const UObject* SplitData);
	UWorld* ValidateDataObject(UObject* SplitData);
	void PreGarbageCollect();
	void TeardownWorldPartition();

	TWeakObjectPtr<UWorld> ReferencedWorld;
	bool bWorldPartitionNeedsTeardown = false;
};

#endif