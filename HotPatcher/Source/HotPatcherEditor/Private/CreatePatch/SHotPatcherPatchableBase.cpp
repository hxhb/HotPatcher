// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

// #include "HotPatcherPrivatePCH.h"
#include "CreatePatch/SHotPatcherPatchableBase.h"
#include "FlibHotPatcherEditorHelper.h"
#include "FlibPatchParserHelper.h"
#include "FHotPatcherVersion.h"
#include "FLibAssetManageHelperEx.h"
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
	FString DefaultEditorIni = FPaths::ProjectConfigDir()/TEXT("DefaultEditor.ini");
	FString DefaultGameIni = FPaths::ProjectConfigDir()/TEXT("DefaultGame.ini");
	auto GameConingLoader = [](const FString& Section,const FString& Key,const FString& Ini)->TArray<FString>
	{
		TArray<FString> result;
		GConfig->GetArray(*Section,*Key,result,Ini);
		return result;
	};
	// TArray<FString> AllMaps = GameConingLoader(TEXT("AllMaps"),TEXT("+Map"),DefaultEditorIni);
	// TArray<FString> AlwayCookMaps = GameConingLoader(TEXT("AlwaysCookMaps"),TEXT("+Map"),DefaultEditorIni);
	// TArray<FString> AddToPatchMaps;
	// AddToPatchMaps.Append(AllMaps);
	// for (const auto& CookMap : AlwayCookMaps)
	// {
	// 	AddToPatchMaps.AddUnique(CookMap);
	// }
	// GetConfigSettings()->GetIncludeSpecifyAssets().Empty();
	// for(const auto& Map: AddToPatchMaps)
	// {
	// 	FPatcherSpecifyAsset Asset;
	// 	FString MapPackagePath;
	// 	UFLibAssetManageHelperEx::ConvLongPackageNameToPackagePath(Map, MapPackagePath);
	// 	Asset.Asset = FSoftObjectPath(MapPackagePath);
	// 	Asset.bAnalysisAssetDependencies = true;
	// 	GetConfigSettings()->GetIncludeSpecifyAssets().Add(Asset);
	// }
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
	// TArray<FString> AlwayCookDirs = GameConingLoader(TEXT("/Script/UnrealEd.ProjectPackagingSettings"),TEXT("+DirectoriesToAlwaysCook"),DefaultGameIni);
	// GetConfigSettings()->GetAssetIncludeFilters().Empty();
	// for(const auto& AlwayCookDir:CleanPath(AlwayCookDirs))
	// {
	// 	FDirectoryPath path;
	// 	path.Path = AlwayCookDir;
	// 	GetConfigSettings()->GetAssetIncludeFilters().Add(path);
	// }
	//
	// TArray<FString> NeverCookDirs = GameConingLoader(TEXT("/Script/UnrealEd.ProjectPackagingSettings"),TEXT("+DirectoriesToNeverCook"),DefaultGameIni);
	// GetConfigSettings()->GetAssetIgnoreFilters().Empty();
	// for(const auto& NeverCookDir:CleanPath(NeverCookDirs))
	// {
	// 	FDirectoryPath path;
	// 	path.Path = NeverCookDir;
	// 	GetConfigSettings()->GetAssetIgnoreFilters().Add(path);
	// }

	TArray<FString> NotUFSDirsToPackage = GameConingLoader(TEXT("/Script/UnrealEd.ProjectPackagingSettings"),TEXT("+DirectoriesToAlwaysStageAsUFS"),DefaultGameIni);

	const FProjectPackageAssetCollection& ProjectPackageAssetCollection = UFlibHotPatcherEditorHelper::ImportProjectSettingsPackages();
	GetConfigSettings()->GetAssetIncludeFilters().Append(ProjectPackageAssetCollection.DirectoryPaths);
	GetConfigSettings()->GetAssetIgnoreFilters().Append(ProjectPackageAssetCollection.NeverCookPaths);
	for(const auto& CookPackage:ProjectPackageAssetCollection.NeedCookPackages)
	{
		FPatcherSpecifyAsset SpecifyAsset;
		SpecifyAsset.Asset = CookPackage;
		SpecifyAsset.bAnalysisAssetDependencies = true;
		SpecifyAsset.AssetRegistryDependencyTypes = {EAssetRegistryDependencyTypeEx::Packages};
		GetConfigSettings()->GetIncludeSpecifyAssets().AddUnique(SpecifyAsset);
	}
	
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
