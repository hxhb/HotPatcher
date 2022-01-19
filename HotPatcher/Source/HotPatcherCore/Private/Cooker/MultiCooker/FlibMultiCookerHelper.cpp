// Fill out your copyright notice in the Description page of Project Settings.


#include "Cooker/MultiCooker/FlibMultiCookerHelper.h"

#include "FlibPatchParserHelper.h"
#include "Cooker/MultiCooker/FCookShaderCollectionProxy.h"
#include "Interfaces/IPluginManager.h"
#include "ShaderPatch/FlibShaderCodeLibraryHelper.h"

FString UFlibMultiCookerHelper::GetMultiCookerBaseDir()
{
	FString SaveConfigTo = FPaths::ConvertRelativePathToFull(
		FPaths::Combine(
			FPaths::ProjectSavedDir(),
			TEXT("HotPatcher/MultiCooker"),
			FApp::GetProjectName()
			));
	return SaveConfigTo;
}

FString UFlibMultiCookerHelper::GetCookerProcConfigPath(const FString& MissionName, int32 MissionID)
{
	FString SaveConfigTo = FPaths::Combine(
			UFlibMultiCookerHelper::GetMultiCookerBaseDir(),
			FString::Printf(TEXT("%s_Cooker_%d.json"),*MissionName,MissionID)
			);
	return SaveConfigTo;
}

FString UFlibMultiCookerHelper::GetCookerProcFailedResultPath(const FString& BaseDir,const FString& MissionName, int32 MissionID)
{
	FString SaveConfigTo = FPaths::Combine(
			BaseDir,
			FString::Printf(TEXT("%s_Cooker_%d_failed.json"),*MissionName,MissionID)
			);
	return SaveConfigTo;
}

FString UFlibMultiCookerHelper::GetProfilingCmd()
{
	return FString::Printf(TEXT("-trace=cpu,memory,loadtime -statnamedevents implies -llm"));
}

TSharedPtr<FCookShaderCollectionProxy> UFlibMultiCookerHelper::CreateCookShaderCollectionProxyByPlatform(const FString& ShaderLibraryName, TArray<ETargetPlatform> Platforms, bool bShareShader, bool bNativeShader, bool bMaster, const
	FString& InSavePath)
{
	SCOPED_NAMED_EVENT_TCHAR(TEXT("UFlibMultiCookerHelper::CreateCookShaderCollectionProxyByPlatform"),FColor::Red);
	TSharedPtr<FCookShaderCollectionProxy> CookShaderCollection;
	TArray<FString> PlatformNames;
	for(const auto& Platform:Platforms)
	{
		PlatformNames.AddUnique(THotPatcherTemplateHelper::GetEnumNameByValue(Platform));
	}
	
	FString SavePath = InSavePath;
	if(FPaths::DirectoryExists(SavePath))
	{
		IFileManager::Get().DeleteDirectory(*SavePath,true,true);
	}
	FString ActualLibraryName = UFlibShaderCodeLibraryHelper::GenerateShaderCodeLibraryName(FApp::GetProjectName(),false);
	CookShaderCollection = MakeShareable(
		new FCookShaderCollectionProxy(
			PlatformNames,
			ShaderLibraryName,
			bShareShader,
			bNativeShader,
			bMaster,
			SavePath
			));
	
	return CookShaderCollection;
}