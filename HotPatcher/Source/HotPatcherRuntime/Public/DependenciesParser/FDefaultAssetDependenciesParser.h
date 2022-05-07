#pragma once

#include "IAssetDependenciesParser.h"


struct FAssetDependenciesParser : public IAssetDependenciesParser
{
	using FScanedCachesType = TMap<FName, TSet<FName>>;
	
	virtual void Parse(const FAssetDependencies& InParseConfig) override;
	virtual const TSet<FName>& GetrParseResults()const { return Results; };
	bool IsIgnoreAsset(const FAssetData& AssetData);
	bool IsIgnoreAsset(const FString& LongPackageName);

	static bool IsForceSkipAsset(const FString& LongPackageName,TSet<FName> IgnoreTypes,TArray<FString> IgnoreFilters);
	TSet<FName> GatherAssetDependicesInfoRecursively(
		FAssetRegistryModule& InAssetRegistryModule,
		FName InLongPackageName,
		const TArray<EAssetRegistryDependencyTypeEx>& InAssetDependencyTypes,
		bool bRecursively,
		const TArray<FString>& IgnoreDirectories,
		TSet<FName> IgnorePackageNames,
		const TSet<FName>& IgnoreAssetTypes,
		FScanedCachesType& InScanedCaches
	);
protected:
	TSet<FName> Results;
	FScanedCachesType ScanedCaches;
	FAssetDependencies ParseConfig;
};

