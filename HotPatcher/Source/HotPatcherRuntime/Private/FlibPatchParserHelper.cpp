// Fill out your copyright notice in the Description page of Project Settings.


#include "FlibPatchParserHelper.h"
#include "FlibAssetManageHelper.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Interfaces/IPluginManager.h"

TArray<FString> UFlibPatchParserHelper::GetAvailableMaps(FString GameName, bool IncludeEngineMaps, bool Sorted)
{
	TArray<FString> Result;
	TArray<FString> EnginemapNames;
	TArray<FString> ProjectMapNames;

	const FString WildCard = FString::Printf(TEXT("*%s"), *FPackageName::GetMapPackageExtension());

	// Scan all Content folder, because not all projects follow Content/Maps convention
	IFileManager::Get().FindFilesRecursive(ProjectMapNames, *FPaths::Combine(*FPaths::RootDir(), *GameName, TEXT("Content")), *WildCard, true, false);

	// didn't find any, let's check the base GameName just in case it is a full path
	if (ProjectMapNames.Num() == 0)
	{
		IFileManager::Get().FindFilesRecursive(ProjectMapNames, *FPaths::Combine(*GameName, TEXT("Content")), *WildCard, true, false);
	}

	for (int32 i = 0; i < ProjectMapNames.Num(); i++)
	{
		Result.Add(FPaths::GetBaseFilename(ProjectMapNames[i]));
	}

	if (IncludeEngineMaps)
	{
		IFileManager::Get().FindFilesRecursive(EnginemapNames, *FPaths::Combine(*FPaths::RootDir(), TEXT("Engine"), TEXT("Content"), TEXT("Maps")), *WildCard, true, false);

		for (int32 i = 0; i < EnginemapNames.Num(); i++)
		{
			Result.Add(FPaths::GetBaseFilename(EnginemapNames[i]));
		}
	}

	if (Sorted)
	{
		Result.Sort();
	}

	return Result;
}