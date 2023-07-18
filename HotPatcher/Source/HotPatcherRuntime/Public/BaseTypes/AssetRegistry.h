#pragma once

#if WITH_UE5
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryState.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/ARFilter.h"
#include "AssetRegistry/IAssetRegistry.h"
#else
#include "AssetRegistryModule.h"
#include "IAssetRegistry.h"
#include "AssetRegistryState.h"
#include "ARFilter.h"
#include "AssetData.h"
#endif