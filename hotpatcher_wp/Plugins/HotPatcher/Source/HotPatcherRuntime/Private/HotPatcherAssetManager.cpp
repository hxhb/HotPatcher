// Fill out your copyright notice in the Description page of Project Settings.


#include "HotPatcherAssetManager.h"

#include "HotPatcherLog.h"

void UHotPatcherAssetManager::ScanPrimaryAssetTypesFromConfig()
{
	SCOPED_NAMED_EVENT_TEXT("ScanPrimaryAssetTypesFromConfig",FColor::Red);
	// ++[RSTUDIO][lipengzha] allow disable scan primaryasset at engine init
	static const bool bNoScanPrimaryAsset = FParse::Param(FCommandLine::Get(), TEXT("NoScanPrimaryAsset"));
	if(bNoScanPrimaryAsset)
	{
		UE_LOG(LogHotPatcher,Display,TEXT("Skip ScanPrimaryAssetTypesFromConfig"));
		return;
	}
	// --[RSTUDIO]
	Super::ScanPrimaryAssetTypesFromConfig();
}
