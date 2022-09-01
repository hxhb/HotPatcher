#pragma once

#include "CoreMinimal.h"

#define AS_PROJECTDIR_MARK TEXT("[PROJECTDIR]")
#define AS_PLUGINDIR_MARK TEXT("[PLUGINDIR]")

struct FEncryptSetting
{
	// -encryptindex
	bool bEncryptIndex = false;
	bool bEncryptAllAssetFiles = false;
	bool bEncryptUAssetFiles = false;
	bool bEncryptIniFiles = false;
	// sign pak
	bool bSign = false;
};