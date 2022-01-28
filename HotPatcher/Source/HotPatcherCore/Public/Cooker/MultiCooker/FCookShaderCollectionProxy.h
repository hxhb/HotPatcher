#pragma once
#include "Resources/Version.h"
#include "ShaderCodeLibrary.h"
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Misc/AES.h"

struct FCookShaderCollectionProxy
{
	FCookShaderCollectionProxy(const TArray<FString>& InPlatformNames,const FString& InLibraryName,bool bShareShader,bool InIsNative,bool bInMaster,const FString& InSaveBaseDir);
	virtual ~FCookShaderCollectionProxy();
	virtual void Init();
	virtual void Shutdown();
	virtual bool IsSuccessed()const { return bSuccessed; }
private:
	TArray<ITargetPlatform*> TargetPlatforms;
	TArray<FString> PlatformNames;
	FString LibraryName;
	bool bShareShader;
	bool bIsNative;
	bool bMaster;
	FString SaveBaseDir;
	bool bSuccessed = false;
};