#include "FVersionUpdaterManager.h"

#include "FCountServerlessWrapper.h"
#include "HotPatcherCore.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "HttpModule.h"
#include "Kismet/KismetStringLibrary.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

DEFINE_LOG_CATEGORY_STATIC(LogVersionUpdaterManager,All,All)

void FVersionUpdaterManager::Reset()
{
	if(HttpHeadRequest.IsValid())
	{
		HttpHeadRequest->CancelRequest();
		HttpHeadRequest.Reset();
	}
	Counter.Reset();
}

void FVersionUpdaterManager::Update()
{
	Counter = MakeShareable(new FCountServerlessWrapper);
	FServerRequestInfo RequestInfo = FCountServerlessWrapper::MakeServerRequestInfo();
	auto ProjectInfo = FCountServerlessWrapper::MakeCurrentProject();
	ProjectInfo.PluginVersion = FString::Printf(TEXT("%d.%d"),GToolMainVersion,GToolPatchVersion);
	if(GetDefault<UHotPatcherSettings>()->bServerlessCounter)
	{
		Counter->Init(RequestInfo,ProjectInfo);
		Counter->Processor();
	}
}

void FVersionUpdaterManager::RequestRemoveVersion(const FString& URL)
{
	if(HttpHeadRequest.IsValid())
	{
		HttpHeadRequest->CancelRequest();
		HttpHeadRequest.Reset();
	}
	HttpHeadRequest = FHttpModule::Get().CreateRequest();
	HttpHeadRequest->SetURL(URL);
	HttpHeadRequest->SetVerb(TEXT("GET"));
	HttpHeadRequest->OnProcessRequestComplete().BindRaw(this,&FVersionUpdaterManager::OnRequestComplete);
	if (HttpHeadRequest->ProcessRequest())
	{
		UE_LOG(LogVersionUpdaterManager, Log, TEXT("Request Remove Version."));
	}
}

void FVersionUpdaterManager::OnRequestComplete(FHttpRequestPtr RequestPtr, FHttpResponsePtr ResponsePtr, bool bConnectedSuccessfully)
{
    if(!ResponsePtr.IsValid())
    {
        return;
    }
	FString Result = ResponsePtr->GetContentAsString();
	TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(Result);
	TSharedPtr<FJsonObject> JsonObject;
	if (FJsonSerializer::Deserialize(JsonReader, JsonObject))
	{
		TArray<FString> ToolNames;
		if(JsonObject->TryGetStringArrayField(TEXT("Tools"),ToolNames))
		{
			for(const auto& ToolName:ToolNames)
			{
				const TSharedPtr<FJsonObject>* ToolJsonObject;
				if(JsonObject->TryGetObjectField(ToolName,ToolJsonObject))
				{
					FRemteVersionDescrible RemoteVersion;
					RemoteVersion.Version = ToolJsonObject->Get()->GetIntegerField(TEXT("Version"));
					RemoteVersion.Author = ToolJsonObject->Get()->GetStringField(TEXT("Author"));
					RemoteVersion.URL = ToolJsonObject->Get()->GetStringField(TEXT("URL"));
					RemoteVersion.Website = ToolJsonObject->Get()->GetStringField(TEXT("Website"));
					RemoteVersion.b3rdMods = ToolJsonObject->Get()->GetBoolField(TEXT("3rdMods"));
					ToolJsonObject->Get()->TryGetNumberField(TEXT("PatchVersion"),RemoteVersion.PatchVersion);
					
					auto GetActionArray = [](const TSharedPtr<FJsonObject>& ActionObject,const FString& Name)->TSet<FName>
					{
						TSet<FName> result;
						const TArray<TSharedPtr<FJsonValue>>& ActionValues = ActionObject->GetArrayField(Name);
						for(auto ActionValue:ActionValues)
						{
							FString Value;
							if(ActionValue.Get()->TryGetString(Value))
							{
								result.Add(*Value);
							}
						}
						return result;
					};
					TArray<FString> ActionsName;
					
					const TSharedPtr<FJsonObject>* Actions;
					if(ToolJsonObject->Get()->TryGetObjectField(TEXT("Actions"),Actions))
					{
						if(Actions && (*Actions)->TryGetStringArrayField(TEXT("ActionNames"),ActionsName))
						{
							for(auto Name:ActionsName)
							{
								RemoteVersion.ActiveActions.Add(*Name,GetActionArray(*Actions,*Name));
							}
						}

						const TArray<TSharedPtr<FJsonValue>>* ActionDescs = nullptr;
						if(Actions && (*Actions)->TryGetArrayField(TEXT("ModsDesc"),ActionDescs))
						{
							for(const TSharedPtr<FJsonValue>& ModDescJsonValue:*ActionDescs)
							{
								const TSharedPtr<FJsonObject>& ModDescJsonObject = ModDescJsonValue.Get()->AsObject();
								FChildModDesc ModDesc;
								ModDescJsonObject->TryGetStringField(TEXT("ModName"),ModDesc.ModName);

								auto ReadFloatValue = [](const TSharedPtr<FJsonObject>& ModDescJsonObject,const FString& Name)->float
								{
									float Result = 0.f;
									FString ValueStr;
									if(ModDescJsonObject->TryGetStringField(Name,ValueStr))
									{
										Result = UKismetStringLibrary::Conv_StringToFloat(ValueStr);
									}
									return Result;
								};
								
								ModDesc.RemoteVersion = ReadFloatValue(ModDescJsonObject,TEXT("Version"));
								ModDesc.MinToolVersion = ReadFloatValue(ModDescJsonObject,TEXT("MinToolVersion"));
								ModDescJsonObject->TryGetStringField(TEXT("Desc"),ModDesc.Description);
								ModDescJsonObject->TryGetStringField(TEXT("URL"),ModDesc.URL);
								ModDescJsonObject->TryGetStringField(TEXT("UpdateURL"),ModDesc.UpdateURL);
								ModDescJsonObject->TryGetBoolField(TEXT("bIsBuiltInMod"),ModDesc.bIsBuiltInMod);
								
								if(ModCurrentVersionGetter)
								{
									ModDesc.CurrentVersion = ModCurrentVersionGetter(ModDesc.ModName);
								}
								RemoteVersion.ModsDesc.Add(*ModDesc.ModName,ModDesc);
							}
						}
					}
					Remotes.Add(*ToolName,RemoteVersion);
				}
			}
		}
	}
	if(bConnectedSuccessfully && Remotes.Num())
	{
		bRequestFinished = true;
		for(const auto& callback:OnRequestFinishedCallback)
		{
			callback();
		}
	}
}
