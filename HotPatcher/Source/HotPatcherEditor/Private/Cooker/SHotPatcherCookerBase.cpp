// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

// #include "HotPatcherPrivatePCH.h"
#include "SHotPatcherCookerBase.h"
#include "FlibHotPatcherCoreHelper.h"
#include "FlibPatchParserHelper.h"
#include "FHotPatcherVersion.h"
#include "FlibAssetManageHelper.h"
#include "FPakFileInfo.h"
// engine header
#include "IDesktopPlatform.h"
#include "DesktopPlatformModule.h"

#include "Misc/FileHelper.h"
#include "Widgets/Input/SHyperlink.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Text/SMultiLineEditableText.h"

#include "Kismet/KismetSystemLibrary.h"
#include "Misc/SecureHash.h"
#include "Misc/ScopedSlowTask.h"
#include "HAL/FileManager.h"
#include "Kismet/KismetTextLibrary.h"

#define LOCTEXT_NAMESPACE "SHotPatcherCreatePatch"

void SHotPatcherCookerBase::Construct(const FArguments& InArgs, TSharedPtr<FHotPatcherCookerModel> InCreatePatchModel)
{
	mCreatePatchModel = InCreatePatchModel;
	// InitMissionNotificationProxy();
}

void SHotPatcherCookerBase::ImportProjectConfig()
{

}

FText SHotPatcherCookerBase::GetGenerateTooltipText() const
{
	return UKismetTextLibrary::Conv_StringToText(TEXT(""));
}

TArray<FString> SHotPatcherCookerBase::OpenFileDialog()const
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	TArray<FString> SelectedFiles;
	
	if (DesktopPlatform)
	{
		const bool bOpened = DesktopPlatform->OpenFileDialog(
			nullptr,
			LOCTEXT("OpenHotPatchConfigDialog", "Open .json").ToString(),
			FString(TEXT("")),
			TEXT(""),
			TEXT("HotPatcher json (*.json)|*.json"),
			EFileDialogFlags::None,
			SelectedFiles
		);
	}
	return SelectedFiles;
}


TArray<FString> SHotPatcherCookerBase::SaveFileDialog()const
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();

	TArray<FString> SaveFilenames;
	if (DesktopPlatform)
	{
		
		const bool bOpened = DesktopPlatform->SaveFileDialog(
			nullptr,
			LOCTEXT("SvedHotPatcherConfig", "Save .json").ToString(),
			FString(TEXT("")),
			TEXT(""),
			TEXT("HotPatcher json (*.json)|*.json"),
			EFileDialogFlags::None,
			SaveFilenames
		);
	}
	return SaveFilenames;
}

#undef LOCTEXT_NAMESPACE
