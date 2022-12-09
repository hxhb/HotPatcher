// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

// #include "HotPatcherPrivatePCH.h"
#include "SHotPatcherWidgetBase.h"
#include "FlibHotPatcherCoreHelper.h"
#include "FlibPatchParserHelper.h"
#include "FHotPatcherVersion.h"
#include "FlibAssetManageHelper.h"
#include "FPakFileInfo.h"
// engine header
#include "IDesktopPlatform.h"
#include "DesktopPlatformModule.h"
#include "FlibHotPatcherEditorHelper.h"

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

void SHotPatcherWidgetBase::Construct(const FArguments& InArgs, TSharedPtr<FHotPatcherContextBase> InContext)
{
	SetContext(InContext);
	// InitMissionNotificationProxy();
}

void SHotPatcherWidgetBase::ImportProjectConfig()
{
	// import uasset
	UFlibHotPatcherCoreHelper::ImportProjectSettingsToScannerConfig(GetConfigSettings()->GetAssetScanConfigRef());
	UFlibHotPatcherCoreHelper::ImportProjectNotAssetDir(GetConfigSettings()->GetAddExternAssetsToPlatform(),ETargetPlatform::AllPlatforms);
}

FText SHotPatcherWidgetInterface::GetGenerateTooltipText() const
{
	return UKismetTextLibrary::Conv_StringToText(TEXT(""));
}

TArray<FString> SHotPatcherWidgetInterface::OpenFileDialog()const
{
	TArray<FString> SelectedFiles;
	SelectedFiles = UFlibHotPatcherEditorHelper::OpenFileDialog(
			LOCTEXT("OpenHotPatchConfigDialog", "Open .json").ToString(),
			FString(TEXT("")),
			TEXT(""),
			TEXT("HotPatcher json (*.json)|*.json"),
			EFileDialogFlags::None
		);
	return SelectedFiles;
}


TArray<FString> SHotPatcherWidgetInterface::SaveFileDialog()const
{
	TArray<FString> SaveFilenames;
	SaveFilenames = UFlibHotPatcherEditorHelper::SaveFileDialog(
			LOCTEXT("SvedHotPatcherConfig", "Save .json").ToString(),
			FString(TEXT("")),
			TEXT(""),
			TEXT("HotPatcher json (*.json)|*.json"),
			EFileDialogFlags::None
		);
	return SaveFilenames;
}


#undef LOCTEXT_NAMESPACE
