#pragma once

#include "IAssetDependenciesParser.h"

struct HOTPATCHERRUNTIME_API FAssetDependenciesParser : public IAssetDependenciesParser
{
	using FScanedCachesType = TMap<FName, TSet<FName>>;
	
	virtual void Parse(const FAssetDependencies& InParseConfig) override;
	virtual const TSet<FName>& GetrParseResults()const { return Results; };
	bool IsIgnoreAsset(const FAssetData& AssetData);
	
	static bool IsForceSkipAsset(
		const FString& LongPackageName,
		const TSet<FName>& IgnoreTypes,
		const TArray<FString>& IgnoreFilters,
		TArray<FString> ForceSkipFilters,
		const TSet<FString>& ForceSkipPackageNames, bool bDispalyLog
	);
	
	TSet<FName> GatherAssetDependicesInfoRecursively(
		FAssetRegistryModule& InAssetRegistryModule,
		FName InLongPackageName,
		const TArray<EAssetRegistryDependencyTypeEx>& InAssetDependencyTypes,
		bool bRecursively,
		const TArray<FString>& IgnoreDirectories,
		const TArray<FString>& ForceSkipDirectories,
		const TSet<FString>& IgnorePackageNames,
		const TSet<FName>& IgnoreAssetTypes, FScanedCachesType& InScanedCaches
	);
protected:
	TSet<FName> Results;
	FScanedCachesType ScanedCaches;
	FAssetDependencies ParseConfig;
};

