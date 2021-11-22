#pragma once

#if ENGINE_MAJOR_VERSION > 4 && ENGINE_MINOR_VERSION > 0
	#include "ZenStoreWriter.h"
	#include "PackageNameCache.h"
	#include "LooseCookedPackageWriter.h"
	#include "AsyncIODelete.h"
	#include "LooseCookedPackageWriter.cpp"
	#include "CookTypes.cpp"
	#include "AsyncIODelete.cpp"
#endif
