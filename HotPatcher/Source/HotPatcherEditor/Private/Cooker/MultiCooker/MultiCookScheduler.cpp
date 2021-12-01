// Fill out your copyright notice in the Description page of Project Settings.


#include "Cooker/MultiCooker/MultiCookScheduler.h"

TArray<FSingleCookerSettings> UMultiCookScheduler::MultiCookScheduler_Implementation(FMultiCookerSettings MultiCookerSettings,const TArray<FAssetDetail>& AllDetails)
{
	SCOPED_NAMED_EVENT_TCHAR(TEXT("UMultiCookScheduler::MultiCookScheduler"),FColor::Red);
	int32 ProcessNumber = MultiCookerSettings.ProcessNumber;
	
	TArray<FSingleCookerSettings> AllSingleCookerSettings;

	TMap<FString,TArray<FAssetDetail>> TypeAssetDetails;

	for(int32 index = 0;index<ProcessNumber;++index)
	{
		FSingleCookerSettings EmptySetting;
		EmptySetting.MissionID = index;
		EmptySetting.MissionName = FString::Printf(TEXT("%s_Cooker_%d"),FApp::GetProjectName(),EmptySetting.MissionID);
		EmptySetting.ShaderLibName = FString::Printf(TEXT("%s_Shader_%d"),FApp::GetProjectName(),EmptySetting.MissionID);
		// EmptySetting.ShaderLibName = FApp::GetProjectName();
		EmptySetting.MultiCookerSettings = MultiCookerSettings;
		AllSingleCookerSettings.Add(EmptySetting);
	}
	
	for(const auto& AssetDetail:AllDetails)
	{
		TArray<FAssetDetail>& Assets = TypeAssetDetails.FindOrAdd(AssetDetail.mAssetType);
		Assets.AddUnique(AssetDetail);
	}
	for(auto& TypeAssets:TypeAssetDetails)
	{
		TArray<TArray<FAssetDetail>> SplitInfo = UFlibPatchParserHelper::SplitArray(TypeAssets.Value,ProcessNumber);
		for(int32 index = 0;index < ProcessNumber;++index)
		{
			AllSingleCookerSettings[index].CookAssets.Append(SplitInfo[index]);
		}
	}
	
	return AllSingleCookerSettings;
}
