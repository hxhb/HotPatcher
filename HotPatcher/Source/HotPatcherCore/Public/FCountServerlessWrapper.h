#pragma once

#include "CoreMinimal.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"

struct HOTPATCHERCORE_API FProjectVersionDesc
{
	FString ProjectName;
	FString EngineVersion;
	FString PluginVersion;
	FString GameName;
	FString UserName;
};

struct HOTPATCHERCORE_API FServerRequestInfo
{
	FString Host;
	FString AppId;
	FString Key;
};

struct HOTPATCHERCORE_API FCountServerlessWrapper
{
	void Init(const FServerRequestInfo& InRequestInfo,const FProjectVersionDesc& InDesc)
	{
		RequestInfo = InRequestInfo;
		Desc = InDesc;
	}
	~FCountServerlessWrapper();
	
	void Processor();

public:
	static FProjectVersionDesc MakeCurrentProject();
	static FServerRequestInfo MakeServerRequestInfo();
protected:
	void RequestObjectID();
	void OnObjectIdReceived( FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess);
	
	FHttpRequestPtr UpdateToServer(const FProjectVersionDesc& Desc,const FString& ObjectID);
	void OnUpdateToServerReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess);
	FHttpRequestPtr CreateToServer(const FProjectVersionDesc& Desc);
	void CreateToServerReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess);

	FString Decode(const FString& Encode);
protected:
	FServerRequestInfo RequestInfo;
	FProjectVersionDesc Desc;
	FHttpRequestPtr ObjectIDRequest;
	FHttpRequestPtr ToServerRequest;
};
