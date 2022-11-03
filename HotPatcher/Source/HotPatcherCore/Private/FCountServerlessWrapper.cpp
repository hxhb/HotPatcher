#pragma once
#include "FCountServerlessWrapper.h"

#include "Misc/App.h"
#include "Misc/Base64.h"
#include "HttpModule.h"
#include "SocketSubsystem.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Resources/Version.h"
#include "CoreGlobals.h"

FProjectVersionDesc FCountServerlessWrapper::MakeCurrentProject()
{
	FProjectVersionDesc result;
	result.ProjectName = FApp::GetProjectName();
	result.GameName = GConfig->GetStr(TEXT("/Script/AndroidRuntimeSettings.AndroidRuntimeSettings"),TEXT("ApplicationDisplayName"),GEngineIni);
	result.EngineVersion = FString::Printf(TEXT("%d.%d.%d"),ENGINE_MAJOR_VERSION,ENGINE_MINOR_VERSION,ENGINE_PATCH_VERSION);

	static FString HostName;
	if(HostName.IsEmpty())
	{
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetHostName(HostName);
	}
	result.UserName = HostName;
	return result;
}

#define COUNTER_API_HOST TEXT("https://appcounter.imzlp.com/1.1/classes/HotPatcher")
#define COUNTER_APPID TEXT("Y1diamRWR3pkRlVWcEtNSWkwMFFGdGVxLWd6R3pvSHN6")
#define COUNTER_KEY TEXT("M1VrUldPaHhsRThGcnBKSUZpNlUxNExI")

FServerRequestInfo FCountServerlessWrapper::MakeServerRequestInfo()
{
	FServerRequestInfo Info;
	Info.Host = COUNTER_API_HOST;
	Info.Key = COUNTER_KEY;
	Info.AppId = COUNTER_APPID;
	return Info;
}

void FCountServerlessWrapper::Processor()
{
	if(!UE_BUILD_SHIPPING)
	{
		RequestObjectID();
	}
}
#if ENGINE_MAJOR_VERSION > 4 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION > 25)
using FHttpRequestType = TSharedRef<IHttpRequest,ESPMode::ThreadSafe>;
#else
using FHttpRequestType = TSharedRef<IHttpRequest,ESPMode::NotThreadSafe>;
#endif

void FCountServerlessWrapper::RequestObjectID()
{
	FHttpModule::Get().SetHttpTimeout(5.0);
	FHttpRequestType ObjectIDRequest = FHttpModule::Get().CreateRequest();
	ObjectIDRequest->OnProcessRequestComplete().BindRaw(this, &FCountServerlessWrapper::OnObjectIdReceived);
	ObjectIDRequest->SetURL(RequestInfo.Host);
	ObjectIDRequest->SetHeader(TEXT("X-LC-Id"),Decode(RequestInfo.AppId));
	ObjectIDRequest->SetHeader(TEXT("X-LC-Key"),Decode(RequestInfo.Key));
	ObjectIDRequest->SetHeader(TEXT("Content-Type"),TEXT("application/json"));
	ObjectIDRequest->SetVerb(TEXT("GET"));
	ObjectIDRequest->ProcessRequest();
}

void FCountServerlessWrapper::OnObjectIdReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess)
{
	TMap<FString,FString> NameIdMaps;
	int32 Count = 0;
	if(bSuccess)
	{
		FString ResponseStr = Response->GetContentAsString();
		TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(ResponseStr);
		TSharedPtr<FJsonObject> DeserializeJsonObject;
		if (FJsonSerializer::Deserialize(JsonReader, DeserializeJsonObject))
		{
			const TArray< TSharedPtr<FJsonValue>>* JsonValues;
			bool bDeserialize = DeserializeJsonObject->TryGetArrayField(TEXT("results"),JsonValues);
			if(bDeserialize)
			{
				for(TSharedPtr<FJsonValue> JsonValue:*JsonValues)
				{
					auto ChildJsonValue =JsonValue->AsObject();
					FString ProjectName = ChildJsonValue->GetStringField(TEXT("ProjectName"));
					FString objectId = ChildJsonValue->GetStringField(TEXT("objectId"));
					if(!NameIdMaps.Contains(ProjectName))
					{
						Count++;
					}
					NameIdMaps.Add(ProjectName,objectId);
				}
			}
		}
		if(NameIdMaps.Contains(FApp::GetProjectName()))
		{
			UpdateToServer(Desc,*NameIdMaps.Find(FApp::GetProjectName()));
		}
		else if(!!Count)
		{
			CreateToServer(Desc);
		}
	}
}

void FCountServerlessWrapper::UpdateToServer(const FProjectVersionDesc& InDesc,const FString& ObjectID)
{
	FString ContentJsonStr = FString::Printf(
		TEXT("{\"ProjectName\": \"%s\",\"GameName\":\"%s\",\"EngineVersion\":\"%s\",\"PluginVersion\":\"%s\",\"UserName\":\"%s\",%s}"),
		*InDesc.ProjectName,
		*InDesc.GameName,
		*InDesc.EngineVersion,
		*InDesc.PluginVersion,
		*InDesc.UserName,
		TEXT("\"Count\":{\"__op\":\"Increment\",\"amount\":1}")
	);
	
	FHttpRequestType CreateToServerRequest = FHttpModule::Get().CreateRequest();
	FString UpdateObjectURL = FString::Printf(TEXT("%s/%s"),*RequestInfo.Host,*ObjectID);
	CreateToServerRequest->SetURL(UpdateObjectURL);
	CreateToServerRequest->SetHeader(TEXT("X-LC-Id"),Decode(RequestInfo.AppId));
	CreateToServerRequest->SetHeader(TEXT("X-LC-Key"),Decode(RequestInfo.Key));
	CreateToServerRequest->SetHeader(TEXT("Content-Type"),TEXT("application/json"));
	CreateToServerRequest->SetContentAsString(ContentJsonStr);
	CreateToServerRequest->SetVerb(TEXT("PUT"));
	CreateToServerRequest->OnProcessRequestComplete().BindRaw(this, &FCountServerlessWrapper::OnUpdateToServerReceived);
	CreateToServerRequest->ProcessRequest();
}

void FCountServerlessWrapper::OnUpdateToServerReceived(FHttpRequestPtr Request, FHttpResponsePtr Response,
	bool bSuccess)
{
	if(bSuccess)
	{
		FString ResponseStr = Response->GetContentAsString();
	}
}

void FCountServerlessWrapper::CreateToServer(const FProjectVersionDesc& InDesc)
{
	FString ContentJsonStr = FString::Printf(
		TEXT("{\"ProjectName\": \"%s\",\"GameName\":\"%s\",\"EngineVersion\":\"%s\",\"PluginVersion\":\"%s\",\"UserName\":\"%s\",\"Count\":%d}"),
		*InDesc.ProjectName,
		*InDesc.GameName,
		*InDesc.EngineVersion,
		*InDesc.PluginVersion,
		*InDesc.UserName,
		1
	);

	FHttpRequestType UpdateToServerRequest = FHttpModule::Get().CreateRequest();
	UpdateToServerRequest->SetURL(RequestInfo.Host);
	UpdateToServerRequest->SetHeader(TEXT("X-LC-Id"),Decode(RequestInfo.AppId));
	UpdateToServerRequest->SetHeader(TEXT("X-LC-Key"),Decode(RequestInfo.Key));
	UpdateToServerRequest->SetHeader(TEXT("Content-Type"),TEXT("application/json"));
	UpdateToServerRequest->SetContentAsString(ContentJsonStr);
	UpdateToServerRequest->SetVerb(TEXT("POST"));
	UpdateToServerRequest->OnProcessRequestComplete().BindRaw(this, &FCountServerlessWrapper::CreateToServerReceived);
	UpdateToServerRequest->ProcessRequest();
}

void FCountServerlessWrapper::CreateToServerReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess)
{
	if(bSuccess)
	{
		FString ResponseStr = Response->GetContentAsString();
	}
}

FString FCountServerlessWrapper::Decode(const FString& Encode)
{
	FString DecodeStr;
	if(FBase64::Decode(Encode,DecodeStr))
	{
		return DecodeStr;
	}
	else
	{
		return Encode;
	}
}
