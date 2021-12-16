// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Algo/Find.h"
#include "Containers/StringView.h"
#include "IAssetRegistry.h"
#include "Misc/PackageName.h"

/* This class has marked all of its functions const and its variables mutable so that CookOnTheFlyServer can use its functions from const functions */
struct FPackageNameCache
{
	bool			HasCacheForPackageName(const FName& PackageName) const;

	FString			GetCachedStandardFileNameString(const UPackage* Package) const;

	FName			GetCachedStandardFileName(const FName& PackageName) const;
	FName			GetCachedStandardFileName(const UPackage* Package) const;

	const FName*	GetCachedPackageNameFromStandardFileName(const FName& NormalizedFileName, bool bExactMatchRequired=true, FName* FoundFileName = nullptr) const;

	void			ClearPackageFileNameCache(IAssetRegistry* InAssetRegistry) const;
	bool			ClearPackageFileNameCacheForPackage(const UPackage* Package) const;
	bool			ClearPackageFileNameCacheForPackage(const FName& PackageName) const;

	void			AppendCacheResults(TArray<TTuple<FName, FString>>&& PackageToStandardFileNames) const;

	bool			CalculateCacheData(FName PackageName, FString& OutStandardFilename, FName& OutStandardFileFName) const;

	bool			ContainsPackageName(FName PackageName) const;
	
	void			SetAssetRegistry(IAssetRegistry* InAssetRegistry) const;
	IAssetRegistry* GetAssetRegistry() const;

	/** Normalize the given FileName for use in looking up the cached data associated with the FileName. This normalization is equivalent to FPaths::MakeStandardFilename */
	static FName	GetStandardFileName(const FName& FileName);
	static FName	GetStandardFileName(const FStringView& FileName);


private:
	struct FCachedPackageFilename
	{
		FCachedPackageFilename(FString&& InStandardFilename, FName InStandardFileFName)
			: StandardFileNameString(MoveTemp(InStandardFilename))
			, StandardFileName(InStandardFileFName)
		{
		}

		FCachedPackageFilename(const FCachedPackageFilename& In) = default;

		FCachedPackageFilename(FCachedPackageFilename&& In)
			: StandardFileNameString(MoveTemp(In.StandardFileNameString))
			, StandardFileName(In.StandardFileName)
		{
		}

		FString		StandardFileNameString;
		FName		StandardFileName;
	};

	bool DoesPackageExist(const FName& PackageName, FString* OutFilename) const;
	const FCachedPackageFilename& Cache(const FName& PackageName) const;

	mutable IAssetRegistry* AssetRegistry = nullptr;

	mutable TMap<FName, FCachedPackageFilename> PackageFilenameCache; // filename cache (only process the string operations once)
	mutable TMap<FName, FName>					PackageFilenameToPackageFNameCache;
};

inline FName FPackageNameCache::GetCachedStandardFileName(const FName& PackageName) const
{
	return Cache(PackageName).StandardFileName;
}

inline bool FPackageNameCache::HasCacheForPackageName(const FName& PackageName) const
{
	return PackageFilenameCache.Find(PackageName) != nullptr;
}

inline FString FPackageNameCache::GetCachedStandardFileNameString(const UPackage* Package) const
{
	// check( Package->GetName() == Package->GetFName().ToString() );
	return Cache(Package->GetFName()).StandardFileNameString;
}

inline FName FPackageNameCache::GetCachedStandardFileName(const UPackage* Package) const
{
	// check( Package->GetName() == Package->GetFName().ToString() );
	return Cache(Package->GetFName()).StandardFileName;
}

inline bool FPackageNameCache::ClearPackageFileNameCacheForPackage(const UPackage* Package) const
{
	return ClearPackageFileNameCacheForPackage(Package->GetFName());
}

inline void FPackageNameCache::AppendCacheResults(TArray<TTuple<FName, FString>>&& PackageToStandardFileNames) const
{
	check(IsInGameThread());
	for (auto& Entry : PackageToStandardFileNames)
	{
		FName PackageName = Entry.Get<0>();
		FString& StandardFilename = Entry.Get<1>();

		FName StandardFileFName(*StandardFilename);
		PackageFilenameToPackageFNameCache.Add(StandardFileFName, PackageName);
		PackageFilenameCache.Emplace(PackageName, FCachedPackageFilename(MoveTemp(StandardFilename), StandardFileFName));
	}
}

inline bool FPackageNameCache::ClearPackageFileNameCacheForPackage(const FName& PackageName) const
{
	check(IsInGameThread());

	return PackageFilenameCache.Remove(PackageName) >= 1;
}
#include "Misc/PackageName.h"

inline bool FPackageNameCache::DoesPackageExist(const FName& PackageName, FString* OutFilename) const
{
	FString PackageNameStr = PackageName.ToString();

	// "/Extra/" packages are editor-generated in-memory packages which don't have a corresponding 
	// asset file (yet). However, we still want to cook these packages out, producing cooked 
	// asset files for packaged projects.
	// if (FPackageName::IsExtraPackage(PackageNameStr))
	// {
	// 	if (UPackage* ExtraPackage = FindPackage(/*Outer =*/nullptr, *PackageNameStr))
	// 	{
	// 		if (OutFilename)
	// 		{
	// 			*OutFilename = FPackageName::LongPackageNameToFilename(PackageNameStr, FPackageName::GetAssetPackageExtension());
	// 		}
	// 		return true;
	// 	}
	// 	// else, the cooker could be responding to a NotifyUObjectCreated() event, and the object hasn't
	// 	// been fully constructed yet (missing from the FindObject() list) -- in this case, we've found 
	// 	// that the linker loader is creating a dummy object to fill a referencing import slot, not loading
	// 	// the proper object (which means we want to ignore it).
	// }
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	if (!AssetRegistry)
	{
		return FPackageName::DoesPackageExist(PackageNameStr, NULL, OutFilename, false);
	}
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
	TArray<FAssetData> Assets;
	AssetRegistry->GetAssetsByPackageName(PackageName, Assets, /*bIncludeOnlyDiskAssets =*/true);

	if (Assets.Num() <= 0)
	{
		return false;
	}

	if (OutFilename)
	{
		const bool ContainsMap = Algo::FindByPredicate(Assets, [](const FAssetData& Asset) { return Asset.PackageFlags & PKG_ContainsMap; }) != nullptr;
		const FString& PackageExtension = ContainsMap ? FPackageName::GetMapPackageExtension() : FPackageName::GetAssetPackageExtension();
		*OutFilename = FPackageName::LongPackageNameToFilename(PackageNameStr, PackageExtension);
	}

	return true;
}

inline bool FPackageNameCache::CalculateCacheData(FName PackageName, FString& OutStandardFilename, FName& OutStandardFileFName) const
{
	FString FilenameOnDisk;
	if (DoesPackageExist(PackageName, &FilenameOnDisk))
	{
		OutStandardFilename = FPaths::ConvertRelativePathToFull(FilenameOnDisk);

		FPaths::MakeStandardFilename(OutStandardFilename);
		OutStandardFileFName = FName(*OutStandardFilename);
		return true;
	}
	return false;
}

inline bool FPackageNameCache::ContainsPackageName(FName PackageName) const
{
	return PackageFilenameCache.Contains(PackageName);
}
inline IAssetRegistry* FPackageNameCache::GetAssetRegistry() const
{
	return AssetRegistry;
}

inline const FPackageNameCache::FCachedPackageFilename& FPackageNameCache::Cache(const FName& PackageName) const
{
	check(IsInGameThread());

	FCachedPackageFilename *Cached = PackageFilenameCache.Find(PackageName);

	if (Cached != NULL)
	{
		return *Cached;
	}

	// cache all the things, like it's your birthday!

	FString FileNameString;
	FName FileName = NAME_None;

	CalculateCacheData(PackageName, FileNameString, FileName);

	PackageFilenameToPackageFNameCache.Add(FileName, PackageName);

	return PackageFilenameCache.Emplace(PackageName, FCachedPackageFilename(MoveTemp(FileNameString), FileName));
}

inline const FName* FPackageNameCache::GetCachedPackageNameFromStandardFileName(const FName& NormalizedFileName, bool bExactMatchRequired, FName* FoundFileName) const
{
	check(IsInGameThread());
	const FName* Result = PackageFilenameToPackageFNameCache.Find(NormalizedFileName);
	if (Result)
	{
		if (FoundFileName)
		{
			*FoundFileName = NormalizedFileName;
		}
		return Result;
	}

	FName PackageName = NormalizedFileName;
	FString PotentialLongPackageName = NormalizedFileName.ToString();
	if (!FPackageName::IsValidLongPackageName(PotentialLongPackageName))
	{
		if (!FPackageName::TryConvertFilenameToLongPackageName(PotentialLongPackageName, PotentialLongPackageName))
		{
			return nullptr;
		}
		PackageName = FName(*PotentialLongPackageName);
	}

	const FCachedPackageFilename& CachedFilename = Cache(PackageName);

	if (bExactMatchRequired)
	{
		if (FoundFileName)
		{
			*FoundFileName = NormalizedFileName;
		}
		return PackageFilenameToPackageFNameCache.Find(NormalizedFileName);
	}
	else
	{
		check(FoundFileName != nullptr);
		*FoundFileName = CachedFilename.StandardFileName;
		return PackageFilenameToPackageFNameCache.Find(CachedFilename.StandardFileName);
	}
}

inline void FPackageNameCache::ClearPackageFileNameCache(IAssetRegistry* InAssetRegistry) const
{
	check(IsInGameThread());
	PackageFilenameCache.Empty();
	PackageFilenameToPackageFNameCache.Empty();
	AssetRegistry = InAssetRegistry;
}

inline void FPackageNameCache::SetAssetRegistry(IAssetRegistry* InAssetRegistry) const
{
	AssetRegistry = InAssetRegistry;
}

inline FName FPackageNameCache::GetStandardFileName(const FName& FileName)
{
	return GetStandardFileName(FileName.ToString());
}

inline FName FPackageNameCache::GetStandardFileName(const FStringView& InFileName)
{
	FString FileName(InFileName);
	FPaths::MakeStandardFilename(FileName);
	return FName(FileName);
}

