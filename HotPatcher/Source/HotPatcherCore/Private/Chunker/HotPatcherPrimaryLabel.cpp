// Fill out your copyright notice in the Description page of Project Settings.

#include "Chunker/HotPatcherPrimaryLabel.h"

#include "FlibHotPatcherCoreHelper.h"
#include "FlibPatchParserHelper.h"
const FName UHotPatcherPrimaryLabel::HotPatherBundleName = FName("Directory");

void UHotPatcherPrimaryLabel::UpdateAssetBundleData()
{
	Super::UpdateAssetBundleData();

	if (!UAssetManager::IsValid())
	{
		return;
	}

	UAssetManager& Manager = UAssetManager::Get();
	IAssetRegistry& AssetRegistry = Manager.GetAssetRegistry();
	SetPriority(Priority);

	TArray<FSoftObjectPath> NewPaths = LabelRules.GetManagedAssets();
	if(NewPaths.Num())
	{
		// Fast set, destroys NewPaths
		AssetBundleData.SetBundleAssets(HotPatherBundleName, MoveTemp(NewPaths));
	}
	
	// Update rules
	FPrimaryAssetId PrimaryAssetId = GetPrimaryAssetId();
	Manager.SetPrimaryAssetRules(PrimaryAssetId, Rules);
}

void UHotPatcherPrimaryLabel::SetPriority(int32 NewPriority)
{
	if(NewPriority != Priority)
	{
		Priority = NewPriority;
	}
	Rules.Priority = NewPriority;
}
