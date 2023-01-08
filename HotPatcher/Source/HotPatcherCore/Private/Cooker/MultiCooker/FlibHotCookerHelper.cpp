// Fill out your copyright notice in the Description page of Project Settings.


#include "Cooker/MultiCooker/FlibHotCookerHelper.h"

#include "FlibHotPatcherCoreHelper.h"
#include "FlibPatchParserHelper.h"
#include "Cooker/MultiCooker/FCookShaderCollectionProxy.h"
#include "Interfaces/IPluginManager.h"
#include "ShaderLibUtils/FlibShaderCodeLibraryHelper.h"

// FString UFlibHotCookerHelper::GetMultiCookerBaseDir()
// {
// 	FString SaveConfigTo = FPaths::ConvertRelativePathToFull(
// 		FPaths::Combine(
// 			FPaths::ProjectSavedDir(),
// 			TEXT("HotPatcher/MultiCooker"),
// 			FApp::GetProjectName()
// 			));
// 	return SaveConfigTo;
// }

// FString UFlibHotCookerHelper::GetCookerProcConfigPath(const FString& MissionName, int32 MissionID)
// {
// 	FString SaveConfigTo = FPaths::Combine(
// 			UFlibHotCookerHelper::GetMultiCookerBaseDir(),
// 			// FString::Printf(TEXT("%s_Cooker_%d.json"),*MissionName,MissionID)
// 			FString::Printf(TEXT("%s.json"),*MissionName)
// 			);
// 	return SaveConfigTo;
// }

FString UFlibHotCookerHelper::GetCookedDir()
{
	return FPaths::Combine(
				TEXT("[PROJECTDIR]"),
				TEXT("Saved"),TEXT("Cooked"));
}

FString UFlibHotCookerHelper::GetCookerBaseDir()
{
		FString SaveConfigTo = FPaths::Combine(
				TEXT("[PROJECTDIR]"),
				TEXT("Saved"),
				TEXT("HotPatcher/MultiCooker"),
				FApp::GetProjectName()
				);
		return SaveConfigTo;
}

FString UFlibHotCookerHelper::GetCookerProcFailedResultPath(const FString& BaseDir,const FString& MissionName, int32 MissionID)
{
	FString SaveConfigTo = FPaths::Combine(
			BaseDir,
			FString::Printf(TEXT("%s_FailedAssets.json"),*MissionName)
			);
	return SaveConfigTo;
}

FString UFlibHotCookerHelper::GetProfilingCmd()
{
	return FString::Printf(TEXT("-tracehost=127.0.0.1 -trace=cpu,memory,loadtime -statnamedevents implies -llm"));
}

TSharedPtr<FCookShaderCollectionProxy> UFlibHotCookerHelper::CreateCookShaderCollectionProxyByPlatform(
	const FString& ShaderLibraryName,
	TArray<ETargetPlatform> Platforms,
	bool bShareShader,
	bool bNativeShader,
	bool bMaster,
	const FString& InSavePath,
	bool bCleanSavePath)
{
	SCOPED_NAMED_EVENT_TEXT("UFlibHotCookerHelper::CreateCookShaderCollectionProxyByPlatform",FColor::Red);
	TSharedPtr<FCookShaderCollectionProxy> CookShaderCollection;
	TArray<FString> PlatformNames;
	for(const auto& Platform:Platforms)
	{
		PlatformNames.AddUnique(THotPatcherTemplateHelper::GetEnumNameByValue(Platform));
	}
	
	FString ActualLibraryName = UFlibShaderCodeLibraryHelper::GenerateShaderCodeLibraryName(FApp::GetProjectName(),false);
	if(bCleanSavePath)
	{
		UFlibHotPatcherCoreHelper::DeleteDirectory(InSavePath);
	}
	CookShaderCollection = MakeShareable(
		new FCookShaderCollectionProxy(
			PlatformNames,
			ShaderLibraryName,
			bShareShader,
			bNativeShader,
			bMaster,
			InSavePath
			));
	
	return CookShaderCollection;
}

bool UFlibHotCookerHelper::IsAppleMetalPlatform(ITargetPlatform* TargetPlatform)
{
	SCOPED_NAMED_EVENT_TEXT("IsAppleMetalPlatform",FColor::Red);
	bool bIsMatched = false;
	TArray<FString> ApplePlatforms = {TEXT("IOS"),TEXT("Mac"),TEXT("TVOS")};
	for(const auto& Platform:ApplePlatforms)
	{
		if(TargetPlatform->PlatformName().StartsWith(Platform,ESearchCase::IgnoreCase))
		{
			bIsMatched = true;
			break;
		}
	}
	return bIsMatched;
}