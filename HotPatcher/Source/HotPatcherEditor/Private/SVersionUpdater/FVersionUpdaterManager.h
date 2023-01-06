#pragma once
#include "Interfaces/IHttpRequest.h"
#include "CoreMinimal.h"
#include "Misc/RemoteConfigIni.h"
#include "FCountServerlessWrapper.h"

struct FChildModDesc
{
	FString ModName;
	float CurrentVersion = 0.f;
	float RemoteVersion = 0.f;
	float MinToolVersion = 0.f;
	FString Description;
	FString URL;
	FString UpdateURL;
	bool bIsBuiltInMod = false;
};

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
	TMap<FName,FChildModDesc> ModsDesc;
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
public:
	// get mod current version.
	TFunction<float(const FString& ModName)> ModCurrentVersionGetter = nullptr;
	// check is valid activite mod callback;
	TFunction<bool(const FString& ModName)> ModIsActivteCallback = nullptr;

	TFunction<TArray<FChildModDesc>()> RequestLocalRegistedMods = nullptr;
	TFunction<TArray<FChildModDesc>()> RequestUnsupportLocalMods = nullptr;
};