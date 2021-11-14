// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "AssetCollector.h"

#include "Resources/Version.h"
#include "Features/IModularFeatures.h"
#include "Misc/EnumRange.h"
#include "Modules/ModuleManager.h"
#include "UObject/Class.h"
#include "PackageAssetsCollector.h"

void FAssetCollectorModule::StartupModule()
{
	UPackageAssetsCollector* AssetsCollector = NewObject<UPackageAssetsCollector>();
	TArray<FString> Assets = AssetsCollector->StartAssetsCollector(TArray<FString>{TEXT("WindowsNoEditor")});
}

void FAssetCollectorModule::ShutdownModule()
{
	
}

IMPLEMENT_MODULE( FAssetCollectorModule, AssetCollector );
