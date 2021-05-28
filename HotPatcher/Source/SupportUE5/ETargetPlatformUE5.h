#pragma once
#include "CoreMinimal.h"
#include "ETargetPlatform.generated.h"

UENUM(BlueprintType)
enum class ETargetPlatform : uint8
{
	None,
	AllPlatforms,
	// for UE5
	Android,
	Android_ASTC,
	Android_DXT,
	Android_ETC2,
	AndroidClient,
	Android_ASTCClient,
	Android_DXTClient,
	Android_ETC2Client,
	Android_Multi,
	Android_MultiClient,
	IOS,
	IOSClient,
	Linux,
	LinuxEditor,
	LinuxServer,
	LinuxClient,
	LinuxAArch64,
	LinuxAArch64Server,
	LinuxAArch64Client,
	Lumin,
	LuminClient,
	Mac,
	MacEditor,
	MacServer,
	MacClient,
	TVOS,
	TVOSClient,
	Windows,
	WindowsEditor,
	WindowsServer,
	WindowsClient
};