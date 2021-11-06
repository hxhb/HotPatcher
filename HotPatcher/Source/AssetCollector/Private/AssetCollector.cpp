// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "AssetCollector.h"

#include "Resources/Version.h"
#include "Features/IModularFeatures.h"
#include "Misc/EnumRange.h"
#include "Modules/ModuleManager.h"
#include "UObject/Class.h"

void FAssetCollectorModule::StartupModule()
{

}

void FAssetCollectorModule::ShutdownModule()
{
	
}

IMPLEMENT_MODULE( FAssetCollectorModule, AssetCollector );
