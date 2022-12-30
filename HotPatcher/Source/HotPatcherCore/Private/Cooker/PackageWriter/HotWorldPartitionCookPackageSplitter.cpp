// Copyright Epic Games, Inc. All Rights Reserved.

#include "HotWorldPartitionCookPackageSplitter.h"

#if WITH_EDITOR && ENGINE_MAJOR_VERSION > 4

#include "Misc/ConfigCacheIni.h"
#include "WorldPartition/WorldPartitionRuntimeLevelStreamingCell.h"
#include "WorldPartition/WorldPartitionLevelStreamingDynamic.h"
#include "WorldPartition/WorldPartitionRuntimeCell.h"
#include "WorldPartition/WorldPartitionRuntimeHash.h"
#include "WorldPartition/WorldPartitionSubsystem.h"
#include "WorldPartition/WorldPartition.h"
#include "WorldPartition/WorldPartitionLog.h"
#include "Editor.h"

// Register FHotWorldPartitionCookPackageSplitter for UWorld class
// REGISTER_COOKPACKAGE_SPLITTER(FHotWorldPartitionCookPackageSplitter, UWorld);

bool FHotWorldPartitionCookPackageSplitter::ShouldSplit(UObject* SplitData)
{
	UWorld* World = Cast<UWorld>(SplitData);
	return World && World->IsPartitionedWorld();
}

FHotWorldPartitionCookPackageSplitter::FHotWorldPartitionCookPackageSplitter()
{
}

FHotWorldPartitionCookPackageSplitter::~FHotWorldPartitionCookPackageSplitter()
{
	check(!ReferencedWorld);
}

void FHotWorldPartitionCookPackageSplitter::Teardown(ETeardown Status)
{
 	if (bInitializedWorldPartition)
	{
		if (UWorld* LocalWorld = ReferencedWorld.Get())
		{
			UWorldPartition* WorldPartition = LocalWorld->PersistentLevel->GetWorldPartition();
			if (WorldPartition)
			{
				WorldPartition->Uninitialize();
			}
		}
		bInitializedWorldPartition = false;
	}

	if (bInitializedPhysicsSceneForSave)
	{
		GEditor->CleanupPhysicsSceneThatWasInitializedForSave(ReferencedWorld.Get(), bForceInitializedWorld);
		bInitializedPhysicsSceneForSave = false;
		bForceInitializedWorld = false;
	}

	ReferencedWorld = nullptr;
}

void FHotWorldPartitionCookPackageSplitter::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(ReferencedWorld);
}

FString FHotWorldPartitionCookPackageSplitter::GetReferencerName() const
{
	return TEXT("FHotWorldPartitionCookPackageSplitter");
}

UWorld* FHotWorldPartitionCookPackageSplitter::ValidateDataObject(UObject* SplitData)
{
	UWorld* PartitionedWorld = CastChecked<UWorld>(SplitData);
	check(PartitionedWorld);
	check(PartitionedWorld->PersistentLevel);
	check(PartitionedWorld->IsPartitionedWorld());
	return PartitionedWorld;
}

const UWorld* FHotWorldPartitionCookPackageSplitter::ValidateDataObject(const UObject* SplitData)
{
	return ValidateDataObject(const_cast<UObject*>(SplitData));
}

TArray<ICookPackageSplitter::FGeneratedPackage> FHotWorldPartitionCookPackageSplitter::GetGenerateList(const UPackage* OwnerPackage, const UObject* OwnerObject)
{
	// TODO: Make WorldPartition functions const so we can honor the constness of the OwnerObject in this API function
	const UWorld* ConstPartitionedWorld = ValidateDataObject(OwnerObject);
	UWorld* PartitionedWorld = const_cast<UWorld*>(ConstPartitionedWorld);

	// Store the World pointer to declare it to GarbageCollection; we do not want to allow the World to be Garbage Collected
	// until we have finished all of our PreSaveGeneratedPackage calls, because we store information on the World 
	// that is necessary for populate 
	ReferencedWorld = PartitionedWorld;

	check(!bInitializedPhysicsSceneForSave && !bForceInitializedWorld);
	bInitializedPhysicsSceneForSave = GEditor->InitializePhysicsSceneForSaveIfNecessary(PartitionedWorld, bForceInitializedWorld);

	// Manually initialize WorldPartition
	UWorldPartition* WorldPartition = PartitionedWorld->PersistentLevel->GetWorldPartition();
	// We expect the WorldPartition has not yet been initialized
	ensure(!WorldPartition->IsInitialized());
	WorldPartition->Initialize(PartitionedWorld, FTransform::Identity);
	bInitializedWorldPartition = true;

	WorldPartition->BeginCook(CookContext);

	bool bIsSuccess = true;
	for (IWorldPartitionCookPackageGenerator* CookPackageGenerator : CookContext.GetCookPackageGenerators())
	{
		bIsSuccess &= CookPackageGenerator->GatherPackagesToCook(CookContext);
	}

	UE_LOG(LogHotWorldPartition, Log, TEXT("[Cook] Gathered %u packages to generate from %u Generators."), CookContext.NumPackageToGenerate(), CookContext.NumGenerators());

	TArray<ICookPackageSplitter::FGeneratedPackage> PackagesToGenerate;
	BuildPackagesToGenerateList(PackagesToGenerate);

	UE_LOG(LogHotWorldPartition, Log, TEXT("[Cook] Sending %u packages to be generated."), PackagesToGenerate.Num());

	return PackagesToGenerate;
}

bool FHotWorldPartitionCookPackageSplitter::PopulateGeneratedPackage(UPackage* OwnerPackage, UObject* OwnerObject,
	const FGeneratedPackageForPopulate& GeneratedPackage, TArray<UObject*>& OutObjectsToMove,
	TArray<UPackage*>& OutModifiedPackages)
{
	UE_LOG(LogHotWorldPartition, Verbose, TEXT("[Cook][PopulateGeneratedPackage] Processing %s"), *FWorldPartitionCookPackage::MakeGeneratedFullPath(GeneratedPackage.GeneratedRootPath, GeneratedPackage.RelativePath));

	bool bIsSuccess = true;

	IWorldPartitionCookPackageGenerator* CookPackageGenerator = nullptr;
	FWorldPartitionCookPackage* CookPackage = nullptr;
	TArray<UPackage*> ModifiedPackages;
	if (CookContext.GetCookPackageGeneratorAndPackage(GeneratedPackage.GeneratedRootPath, GeneratedPackage.RelativePath, CookPackageGenerator, CookPackage))
	{
		bIsSuccess = CookPackageGenerator->PopulateGeneratedPackageForCook(CookContext, *CookPackage, ModifiedPackages);
	}
	else
	{
		UE_LOG(LogHotWorldPartition, Error, TEXT("[Cook][PopulateGeneratedPackage] Could not find WorldPartitionCookPackage for %s"), *FWorldPartitionCookPackage::MakeGeneratedFullPath(GeneratedPackage.GeneratedRootPath, GeneratedPackage.RelativePath));
		bIsSuccess = false;
	}

	UE_LOG(LogHotWorldPartition, Verbose, TEXT("[Cook][PopulateGeneratedPackage] Gathered %u modified packages for %s"), ModifiedPackages.Num() , *FWorldPartitionCookPackage::MakeGeneratedFullPath(GeneratedPackage.GeneratedRootPath, GeneratedPackage.RelativePath));
	OutModifiedPackages = MoveTemp(ModifiedPackages);

	return bIsSuccess;
}

bool FHotWorldPartitionCookPackageSplitter::PopulateGeneratorPackage(UPackage* OwnerPackage, UObject* OwnerObject,
	const TArray<ICookPackageSplitter::FGeneratedPackageForPreSave>& GeneratedPackages, TArray<UObject*>& OutObjectsToMove,
	TArray<UPackage*>& OutModifiedPackages)
{
	UE_LOG(LogHotWorldPartition, Log, TEXT("[Cook][PopulateGeneratorPackage] Processing %u packages"), GeneratedPackages.Num());

	bool bIsSuccess = true;
	if (GeneratedPackages.Num() != CookContext.NumPackageToGenerate())
	{
		UE_LOG(LogHotWorldPartition, Error, TEXT("[Cook][PopulateGeneratorPackage] Receieved %u generated packages. Was expecting %u"), GeneratedPackages.Num(), CookContext.NumPackageToGenerate());
		bIsSuccess = false;
	}

	TArray<UPackage*> ModifiedPackages;
	for (IWorldPartitionCookPackageGenerator* CookPackageGenerator : CookContext.GetCookPackageGenerators())
	{
		bIsSuccess &= CookPackageGenerator->PrepareGeneratorPackageForCook(CookContext, ModifiedPackages);
		if (const TArray<FWorldPartitionCookPackage*>* CookPackages = CookContext.GetCookPackages(CookPackageGenerator))
		{
			bIsSuccess &= CookPackageGenerator->PopulateGeneratorPackageForCook(CookContext, *CookPackages, ModifiedPackages);
		}
	}

	UE_LOG(LogHotWorldPartition, Log, TEXT("[Cook][PopulateGeneratorPackage] Gathered %u modified packages"), ModifiedPackages.Num());
	OutModifiedPackages = MoveTemp(ModifiedPackages);

	return bIsSuccess;
}

void FHotWorldPartitionCookPackageSplitter::OnOwnerReloaded(UPackage* OwnerPackage, UObject* OwnerObject)
{
	// It should not be possible for the owner to reload due to garbage collection while we are active and keeping it referenced
	check(!ReferencedWorld);
}

void FHotWorldPartitionCookPackageSplitter::BuildPackagesToGenerateList(TArray<ICookPackageSplitter::FGeneratedPackage>& PackagesToGenerate) const
{
	for (const IWorldPartitionCookPackageGenerator* CookPackageGenerator : CookContext.GetCookPackageGenerators())
	{
		if (const TArray<FWorldPartitionCookPackage*>* CookPackages = CookContext.GetCookPackages(CookPackageGenerator))
		{
			PackagesToGenerate.Reserve(CookPackages->Num());

			for (const FWorldPartitionCookPackage* CookPackage : *CookPackages)
			{
				ICookPackageSplitter::FGeneratedPackage& GeneratedPackage = PackagesToGenerate.Emplace_GetRef();
				GeneratedPackage.GeneratedRootPath = CookPackage->Root;
				GeneratedPackage.RelativePath = CookPackage->RelativePath;

				CookPackage->Type == FWorldPartitionCookPackage::EType::Level ? GeneratedPackage.SetCreateAsMap(true) : GeneratedPackage.SetCreateAsMap(false);

				// @todo_ow: Set dependencies once we get iterative cooking working
			}
		}
	}
}

#endif
