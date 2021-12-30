#pragma once

#include "IAssetDependenciesParser.h"



struct FAssetDependenciesParser : public IAssetDependenciesParser
{
	using FScanedCachesType = TMap<FName, TSet<FName>>;
	
	virtual void Parse(const FAssetDependencies& ParseConfig) override;
	virtual const TSet<FName>& GetrParseResults()const { return Results; };
	
	static TSet<FName> GatherAssetDependicesInfoRecursively(
		FAssetRegistryModule& InAssetRegistryModule,
		FName InLongPackageName,
		const TArray<EAssetRegistryDependencyTypeEx>& InAssetDependencyTypes,
		bool bRecursively,
		TSet<FName> IgnorePackageNames,
		FScanedCachesType& ScanedCaches
	);
protected:
	TSet<FName> Results;
	FScanedCachesType ScanedCaches;
};

