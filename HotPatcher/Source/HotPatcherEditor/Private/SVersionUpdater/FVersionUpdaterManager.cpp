#include "FVersionUpdaterManager.h"

#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "HttpModule.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

DEFINE_LOG_CATEGORY_STATIC(LogVersionUpdaterManager,All,All)

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
					ToolJsonObject->Get()->TryGetNumberField(TEXT("PatchVersion"),RemoteVersion.PatchVersion);
					const TSharedPtr<FJsonObject>& Actions = ToolJsonObject->Get()->GetObjectField(TEXT("Actions"));
					
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
					
					if(Actions->TryGetStringArrayField(TEXT("ActionNames"),ActionsName))
					{
						for(auto Name:ActionsName)
						{
							RemoteVersion.ActiveActions.Add(*Name,GetActionArray(Actions,*Name));
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