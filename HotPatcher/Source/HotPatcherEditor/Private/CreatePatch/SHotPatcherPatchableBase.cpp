// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

// #include "HotPatcherPrivatePCH.h"
#include "CreatePatch/SHotPatcherPatchableBase.h"
#include "FlibHotPatcherEditorHelper.h"
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

void SHotPatcherPatchableBase::Construct(const FArguments& InArgs, TSharedPtr<FHotPatcherCreatePatchModel> InCreatePatchModel)
{
	mCreatePatchModel = InCreatePatchModel;
	// InitMissionNotificationProxy();
}

void SHotPatcherPatchableBase::ImportProjectConfig()
{
	// import uasset
	UFlibHotPatcherEditorHelper::ImportProjectSettingsToSettingBase(GetConfigSettings());
	
	FString DefaultEditorIni = FPaths::ProjectConfigDir()/TEXT("DefaultEditor.ini");
	FString DefaultGameIni = FPaths::ProjectConfigDir()/TEXT("DefaultGame.ini");
	auto GameConingLoader = [](const FString& Section,const FString& Key,const FString& Ini)->TArray<FString>
	{
		TArray<FString> result;
		GConfig->GetArray(*Section,*Key,result,Ini);
		return result;
	};

	auto CleanPath= [](const TArray<FString>& List)->TArray<FString>
	{
		TArray<FString> result;
		for(const auto& Item:List)
		{
			if(Item.StartsWith(TEXT("(Path=\"")) && Item.EndsWith(TEXT("\")")))
			{
				FString str = Item;
				str.RemoveFromStart(TEXT("(Path=\""));
				str.RemoveFromEnd(TEXT("\")"));
				result.AddUnique(str);
			}
			
		}
		return result;
	};
	
	TArray<FString> NotUFSDirsToPackage = GameConingLoader(TEXT("/Script/UnrealEd.ProjectPackagingSettings"),TEXT("+DirectoriesToAlwaysStageAsUFS"),DefaultGameIni);
	FPlatformExternAssets AddToAllPlatform;
	AddToAllPlatform.TargetPlatform = ETargetPlatform::AllPlatforms;
	for(const auto& UFSDir:CleanPath(NotUFSDirsToPackage))
	{
		FExternDirectoryInfo DirInfo;
		// DirInfo.DirectoryPath.Path = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectContentDir(),UFSDir));
		DirInfo.DirectoryPath.Path = FPaths::Combine(TEXT("[PROJECT_CONTENT_DIR]"),UFSDir);
		DirInfo.MountPoint = FString::Printf(TEXT("../../../%s/Content/%s"),FApp::GetProjectName(),*UFSDir);
		AddToAllPlatform.AddExternDirectoryToPak.Add(DirInfo);
	}
	if(AddToAllPlatform.AddExternDirectoryToPak.Num() || AddToAllPlatform.AddExternFileToPak.Num())
	{
		GetConfigSettings()->GetAddExternAssetsToPlatform().Empty();
		GetConfigSettings()->GetAddExternAssetsToPlatform().Add(AddToAllPlatform);
	}
}

FText SHotPatcherPatchableBase::GetGenerateTooltipText() const
{
	return UKismetTextLibrary::Conv_StringToText(TEXT(""));
}

TArray<FString> SHotPatcherPatchableBase::OpenFileDialog()const
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


TArray<FString> SHotPatcherPatchableBase::SaveFileDialog()const
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
