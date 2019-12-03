#pragma once


#include "CoreMinimal.h"
#include "ETargetPlatform.generated.h"

UENUM(BlueprintType)
enum class ETargetPlatform : uint8
{
	AllDesktop,
	MacClient,
	MacNoEditor,
	MacServer,
	Mac,
	WindowsClient,
	WindowsNoEditor,
	WindowsServer,
	Windows,
	Android,
	Android_ASTC,
	Android_ATC,
	Android_DXT,
	Android_ETC1,
	Android_ETC1a,
	Android_ETC2,
	Android_PVRTC,
	AndroidClient,
	Android_ASTCClient,
	Android_ATCClient,
	Android_DXTClient,
	Android_ETC1Client,
	Android_ETC1aClient,
	Android_ETC2Client,
	Android_PVRTCClient,
	Android_Multi,
	Android_MultiClient,
	HTML5,
	IOSClient,
	IOS,
	TVOSClient,
	TVOS,
	LinuxClient,
	LinuxNoEditor,
	LinuxServer,
	Linux,
	Lumin,
	LuminClient
};
