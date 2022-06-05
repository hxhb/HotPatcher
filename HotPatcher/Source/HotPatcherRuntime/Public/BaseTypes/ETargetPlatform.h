#pragma once

#include "Templates/HotPatcherTemplateHelper.hpp"

#include "CoreMinimal.h"
#include "Misc/EnumRange.h"
#include "ETargetPlatform.generated.h"


UENUM(BlueprintType)
enum class ETargetPlatform : uint8
{
	None,
	AllPlatforms,
	Count UMETA(Hidden)
};
ENUM_RANGE_BY_COUNT(ETargetPlatform, ETargetPlatform::Count);

DECAL_GETCPPTYPENAME_SPECIAL(ETargetPlatform)

// static TArray<FString> AppendPlatformEnums = {
// #if ENGINE_MAJOR_VERSION > 4
// 	TEXT("Android"),
// 	TEXT("Android_ASTC"),
// 	TEXT("Android_DXT"),
// 	TEXT("Android_ETC2"),
// 	TEXT("AndroidClient"),
// 	TEXT("Android_ASTCClient"),
// 	TEXT("Android_DXTClient"),
// 	TEXT("Android_ETC2Client"),
// 	TEXT("Android_Multi"),
// 	TEXT("Android_MultiClient"),
// 	TEXT("IOS"),
// 	TEXT("IOSClient"),
// 	TEXT("Linux"),
// 	TEXT("LinuxEditor"),
// 	TEXT("LinuxServer"),
// 	TEXT("LinuxClient"),
// 	TEXT("LinuxAArch64"),
// 	TEXT("LinuxAArch64Server"),
// 	TEXT("LinuxAArch64Client"),
// 	TEXT("Lumin"),
// 	TEXT("LuminClient"),
// 	TEXT("Mac"),
// 	TEXT("MacEditor"),
// 	TEXT("MacServer"),
// 	TEXT("MacClient"),
// 	TEXT("TVOS"),
// 	TEXT("TVOSClient"),
// 	TEXT("Windows"),
// 	TEXT("WindowsEditor"),
// 	TEXT("WindowsServer"),
// 	TEXT("WindowsClient")
// #else
// // for UE4
// 	TEXT("AllDesktop"),
// 	TEXT("MacClient"),
// 	TEXT("MacNoEditor"),
// 	TEXT("MacServer"),
// 	TEXT("Mac"),
// 	TEXT("WindowsClient"),
// 	TEXT("WindowsNoEditor"),
// 	TEXT("WindowsServer"),
// 	TEXT("Windows"),
// 	TEXT("Android"),
// 	TEXT("Android_ASTC"),
// 	TEXT("Android_ATC"),
// 	TEXT("Android_DXT"),
// 	TEXT("Android_ETC1"),
// 	TEXT("Android_ETC1a"),
// 	TEXT("Android_ETC2"),
// 	TEXT("Android_PVRTC"),
// 	TEXT("AndroidClient"),
// 	TEXT("Android_ASTCClient"),
// 	TEXT("Android_ATCClient"),
// 	TEXT("Android_DXTClient"),
// 	TEXT("Android_ETC1Client"),
// 	TEXT("Android_ETC1aClient"),
// 	TEXT("Android_ETC2Client"),
// 	TEXT("Android_PVRTCClient"),
// 	TEXT("Android_Multi"),
// 	TEXT("Android_MultiClient"),
// 	TEXT("HTML5"),
// 	TEXT("IOSClient"),
// 	TEXT("IOS"),
// 	TEXT("TVOSClient"),
// 	TEXT("TVOS"),
// 	TEXT("LinuxClient"),
// 	TEXT("LinuxNoEditor"),
// 	TEXT("LinuxServer"),
// 	TEXT("Linux"),
// 	TEXT("Lumin"),
// 	TEXT("LuminClient")
// #endif
// };