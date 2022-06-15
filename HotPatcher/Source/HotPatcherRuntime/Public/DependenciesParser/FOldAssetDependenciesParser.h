#pragma once

#include "IAssetDependenciesParser.h"

struct FOldAssetDependenciesParser : public IAssetDependenciesParser
{
	using FScanedCachesType = TMap<FString, FAssetDependenciesInfo>;
	
	virtual void Parse(const FAssetDependencies& ParseConfig) override;
	virtual const FAssetDependenciesInfo& GetrParseResults()const{ return Results; };

protected:
	FAssetDependenciesInfo Results;
	FScanedCachesType ScanedCaches;
};

