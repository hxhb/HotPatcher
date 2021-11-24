// Fill out your copyright notice in the Description page of Project Settings.


#include "FlibMultiCookerHelper.h"
#include "Interfaces/IPluginManager.h"

FString UFlibMultiCookerHelper::GetMultiCookerBaseDir()
{
	FString SaveConfigTo = FPaths::ConvertRelativePathToFull(
		FPaths::Combine(
			FPaths::ProjectSavedDir(),
			TEXT("HotPatcher/MultiCooker")));
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

FString UFlibMultiCookerHelper::GetCookerProcFailedResultPath(const FString& MissionName, int32 MissionID)
{
	FString SaveConfigTo = FPaths::Combine(
			UFlibMultiCookerHelper::GetMultiCookerBaseDir(),
			FString::Printf(TEXT("%s_Cooker_%d_failed.json"),*MissionName,MissionID)
			);
	return SaveConfigTo;
}
