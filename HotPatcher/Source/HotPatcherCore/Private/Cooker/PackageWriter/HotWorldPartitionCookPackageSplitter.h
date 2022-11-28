// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Resources/Version.h"

#if WITH_EDITOR && ENGINE_MAJOR_VERSION > 4

#include "CookPackageSplitter.h"
#include "Engine/World.h"
#include "UObject/WeakObjectPtrTemplates.h"
#include "WorldPartition/Cook/WorldPartitionCookPackageContext.h"

class FHotWorldPartitionCookPackageSplitter : public FGCObject, public ICookPackageSplitter
{
public:
	//~ Begin of ICookPackageSplitter
	static bool ShouldSplit(UObject* SplitData);

	FHotWorldPartitionCookPackageSplitter();
	virtual ~FHotWorldPartitionCookPackageSplitter();

	virtual void Teardown(ETeardown Status) override;
	virtual bool UseInternalReferenceToAvoidGarbageCollect() override { return true; }
	virtual TArray<ICookPackageSplitter::FGeneratedPackage> GetGenerateList(const UPackage* OwnerPackage, const UObject* OwnerObject) override;
	virtual bool PopulateGeneratedPackage(UPackage* OwnerPackage, UObject* OwnerObject,
		const FGeneratedPackageForPopulate& GeneratedPackage, TArray<UObject*>& OutObjectsToMove, TArray<UPackage*>& OutModifiedPackages) override;
	virtual bool PopulateGeneratorPackage(UPackage* OwnerPackage, UObject* OwnerObject,
		const TArray<ICookPackageSplitter::FGeneratedPackageForPreSave>& GeneratedPackages, TArray<UObject*>& OutObjectsToMove,
		TArray<UPackage*>& OutModifiedPackages) override;
	virtual void OnOwnerReloaded(UPackage* OwnerPackage, UObject* OwnerObject) override;
	//~ End of ICookPackageSplitter

private:
	/** FGCObject interface */
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override;

	const UWorld* ValidateDataObject(const UObject* SplitData);
	UWorld* ValidateDataObject(UObject* SplitData);

	void BuildPackagesToGenerateList(TArray<ICookPackageSplitter::FGeneratedPackage>& PackagesToGenerate) const;
	bool MapGeneratePackageToCookPackage(const TArray<ICookPackageSplitter::FGeneratedPackageForPreSave>& GeneratedPackages);
	
	TObjectPtr<UWorld> ReferencedWorld = nullptr;

	FWorldPartitionCookPackageContext CookContext;

	bool bInitializedWorldPartition = false;
	bool bForceInitializedWorld = false;
	bool bInitializedPhysicsSceneForSave = false;
};

#endif