#include "HotPatcherDelegates.h"

FHotPatcherDelegates& FHotPatcherDelegates::Get()
{
	// return the singleton object
	static FHotPatcherDelegates Singleton;
	return Singleton;
}
