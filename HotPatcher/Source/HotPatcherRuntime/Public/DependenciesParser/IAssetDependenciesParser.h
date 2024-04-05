#pragma once

#include "CoreMinimal.h"
#include "FlibAssetManageHelper.h"
#include "FPatcherSpecifyAsset.h"

struct FAssetDependencies
{
	TArray<FString> IncludeFilters;
	TArray<FString> IgnoreFilters;
	TArray<EAssetRegistryDependencyTypeEx> AssetRegistryDependencyTypes;
	TArray<FPatcherSpecifyAsset> InIncludeSpecifyAsset;
	// like /Game/EditorOnly /Engine/VREditor
	TArray<FString> ForceSkipContents;
	TSet<FString> ForceSkipPackageNames;
	TSet<FName> IgnoreAseetTypes;
	bool bSupportWorldComposition = true;
	bool bRedirector = true;
	bool AnalysicFilterDependencies = true;
	bool IncludeHasRefAssetsOnly = false;
	
};

struct IAssetDependenciesParser
{
	virtual void Parse(const FAssetDependencies& ParseConfig) = 0;
	// virtual const TSet<FName>& GetrParseResults()const = 0;
	virtual ~IAssetDependenciesParser(){}
};
