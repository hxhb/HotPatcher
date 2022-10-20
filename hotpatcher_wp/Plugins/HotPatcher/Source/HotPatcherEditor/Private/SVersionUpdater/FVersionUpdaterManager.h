#pragma once
#include "Interfaces/IHttpRequest.h"
#include "CoreMinimal.h"
#include "Misc/RemoteConfigIni.h"
#include "FCountServerlessWrapper.h"

struct FRemteVersionDescrible
{
	FRemteVersionDescrible()=default;
	FRemteVersionDescrible(const FRemteVersionDescrible&)=default;
	
	int32 Version;
	int32 PatchVersion;
	FString Author;
	FString Website;
	FString URL;
	bool b3rdMods;
	TMap<FName,TSet<FName>> ActiveActions;
};


struct FVersionUpdaterManager
{
	static FVersionUpdaterManager& Get()
	{
		static FVersionUpdaterManager Manager;
		return Manager;
	}
	
	void Reset();
	void Update();
	void RequestRemoveVersion(const FString& URL);
	FRemteVersionDescrible* GetRemoteVersionByName(FName Name){ return Remotes.Find(Name); }
	bool IsRequestFinished()const { return bRequestFinished; }
	void AddOnFinishedCallback(TFunction<void(void)> callback){ OnRequestFinishedCallback.Add(callback); }
protected:
	void OnRequestComplete(FHttpRequestPtr RequestPtr, FHttpResponsePtr ResponsePtr, bool bConnectedSuccessfully);
	FHttpRequestPtr HttpHeadRequest;
	TMap<FName,FRemteVersionDescrible> Remotes;
	bool bRequestFinished = false;
	TArray<TFunction<void(void)>> OnRequestFinishedCallback;
	TSharedPtr<FCountServerlessWrapper> Counter;
};