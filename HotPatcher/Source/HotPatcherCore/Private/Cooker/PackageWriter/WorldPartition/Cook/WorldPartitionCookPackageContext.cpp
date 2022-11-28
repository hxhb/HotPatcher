// Copyright Epic Games, Inc. All Rights Reserved.
#include "WorldPartitionCookPackageContext.h"

#if WITH_EDITOR && !UE_VERSION_OLDER_THAN(5,1,0)

#include "WorldPartition/WorldPartitionLog.h"
#include "WorldPartition/Cook/WorldPartitionCookPackageGenerator.h"

DEFINE_LOG_CATEGORY(LogHotWorldPartition)

FWorldPartitionCookPackageContext::FWorldPartitionCookPackageContext()
{
}

void FWorldPartitionCookPackageContext::RegisterPackageCookPackageGenerator(IWorldPartitionCookPackageGenerator* CookPackageGenerator)
{
	check(!CookPackageGenerators.Contains(CookPackageGenerator));
	CookPackageGenerators.Add(CookPackageGenerator);
	check(!PackagesToCookByGenerator.Contains(CookPackageGenerator));
}

const FWorldPartitionCookPackage* FWorldPartitionCookPackageContext::AddLevelStreamingPackageToGenerate(IWorldPartitionCookPackageGenerator* CookPackageGenerator, const FString& Root, const FString& RelativePath)
{
	return AddPackageToGenerateInternal(CookPackageGenerator, Root, RelativePath, FWorldPartitionCookPackage::EType::Level);
}

const FWorldPartitionCookPackage* FWorldPartitionCookPackageContext::AddGenericPackageToGenerate(IWorldPartitionCookPackageGenerator* CookPackageGenerator, const FString& Root, const FString& RelativePath)
{
	return AddPackageToGenerateInternal(CookPackageGenerator, Root, RelativePath, FWorldPartitionCookPackage::EType::Generic);
}

bool FWorldPartitionCookPackageContext::GetCookPackageGeneratorAndPackage(const FString& PackageRoot, const FString& PackageRelativePath, IWorldPartitionCookPackageGenerator*& CookPackageGenerator, FWorldPartitionCookPackage*& CookPackage)
{
	FWorldPartitionCookPackage::IDType PackageId = FWorldPartitionCookPackage::MakeCookPackageID(PackageRoot, PackageRelativePath);
	if (IWorldPartitionCookPackageGenerator** GeneratorPtr = CookGeneratorByPackageId.Find(PackageId))
	{
		if (TUniquePtr<FWorldPartitionCookPackage>* PackagePtr = PackagesToCookById.Find(PackageId))
		{
			check((*PackagePtr)->Root.Equals(PackageRoot, ESearchCase::IgnoreCase) && (*PackagePtr)->RelativePath.Equals(PackageRelativePath, ESearchCase::IgnoreCase));

			CookPackageGenerator = *GeneratorPtr;
			CookPackage = (*PackagePtr).Get();
			return true;
		}
	}

	return false;
}

const FWorldPartitionCookPackage* FWorldPartitionCookPackageContext::AddPackageToGenerateInternal(IWorldPartitionCookPackageGenerator* CookPackageGenerator, const FString& Root, const FString& RelativePath, FWorldPartitionCookPackage::EType Type)
{
	if (CookPackageGenerators.Contains(CookPackageGenerator))
	{
		FWorldPartitionCookPackage::IDType PackageId = FWorldPartitionCookPackage::MakeCookPackageID(FWorldPartitionCookPackage::SanitizePathComponent(Root), FWorldPartitionCookPackage::SanitizePathComponent(RelativePath));
		TUniquePtr<FWorldPartitionCookPackage>* ExistingPackage = PackagesToCookById.Find(PackageId);
		if (ExistingPackage == nullptr)
		{
			TUniquePtr<FWorldPartitionCookPackage>& CookPackage = PackagesToCookById.Emplace(PackageId, MakeUnique<FWorldPartitionCookPackage>(FWorldPartitionCookPackage::SanitizePathComponent(Root), FWorldPartitionCookPackage::SanitizePathComponent(RelativePath), Type));
			check(PackageId == CookPackage->PackageId);

			CookGeneratorByPackageId.Add(PackageId, CookPackageGenerator);

			TArray<FWorldPartitionCookPackage*>& PackagesToCookForHandler = PackagesToCookByGenerator.FindOrAdd(CookPackageGenerator);
			PackagesToCookForHandler.Add(CookPackage.Get());

			UE_LOG(LogHotWorldPartition, Verbose, TEXT("[Cook] Added Package %s with ID %llu in context"), *CookPackage->GetFullGeneratedPath(), PackageId);

			return CookPackage.Get();
		}
		else
		{
			UE_LOG(LogHotWorldPartition, Error, TEXT("[Cook] Trying to add package %s in context but there is already a package to generate with the same ID (%llu). Other package: %s Id %llu"),
				*FWorldPartitionCookPackage::MakeGeneratedFullPath(Root, RelativePath), PackageId, *(*ExistingPackage)->GetFullGeneratedPath(), (*ExistingPackage)->PackageId);
		}
	}
	else
	{
		UE_LOG(LogHotWorldPartition, Error, TEXT("[Cook] Trying to add package %s in context, but its generator is not registered."), *FWorldPartitionCookPackage::MakeGeneratedFullPath(Root, RelativePath));
	}

	return nullptr;
}

#endif