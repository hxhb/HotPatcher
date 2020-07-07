// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

// #include "HotPatcherPrivatePCH.h"
#include "CreatePatch/SHotPatcherExportRelease.h"
#include "FlibHotPatcherEditorHelper.h"
#include "FLibAssetManageHelperEx.h"
#include "AssetManager/FAssetDependenciesInfo.h"
#include "FHotPatcherVersion.h"

// engine header
#include "Widgets/Input/SHyperlink.h"
#include "Widgets/Layout/SSeparator.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/ScopedSlowTask.h"

#define LOCTEXT_NAMESPACE "SHotPatcherExportRelease"

void SHotPatcherExportRelease::Construct(const FArguments& InArgs, TSharedPtr<FHotPatcherCreatePatchModel> InCreatePatchModel)
{
	CreateExportFilterListView();

	mCreatePatchModel = InCreatePatchModel;

	ChildSlot
		[

			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				[
					SettingsView->AsShared()
				]
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0, 8.0, 0.0, 0.0)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Right)
			.Padding(4, 4, 10, 4)
			[
				SNew(SButton)
				.Text(LOCTEXT("GenerateRelease", "Export Release"))
				.OnClicked(this,&SHotPatcherExportRelease::DoExportRelease)
				.IsEnabled(this,&SHotPatcherExportRelease::CanExportRelease)
			]
		];

	ExportReleaseSettings = UExportReleaseSettings::Get();
	SettingsView->SetObject(ExportReleaseSettings);
	
}

void SHotPatcherExportRelease::ImportConfig()
{
	UE_LOG(LogTemp, Log, TEXT("Release Import Config"));
	TArray<FString> Files = this->OpenFileDialog();
	if (!Files.Num()) return;

	FString LoadFile = Files[0];

	FString JsonContent;
	if (UFLibAssetManageHelperEx::LoadFileToString(LoadFile, JsonContent))
	{
		UFlibHotPatcherEditorHelper::DeserializeReleaseConfig(ExportReleaseSettings, JsonContent);
		SettingsView->ForceRefresh();
	}
}
void SHotPatcherExportRelease::ExportConfig()const
{
	UE_LOG(LogTemp, Log, TEXT("Release Export Config"));
	TArray<FString> Files = this->SaveFileDialog();

	if (!Files.Num()) return;

	FString SaveToFile = Files[0].EndsWith(TEXT(".json")) ? Files[0] : Files[0].Append(TEXT(".json"));

	if (ExportReleaseSettings)
	{

		FString SerializedJsonStr;
		ExportReleaseSettings->SerializeReleaseConfigToString(SerializedJsonStr);

		if (FFileHelper::SaveStringToFile(SerializedJsonStr, *SaveToFile))
		{
			FText Msg = LOCTEXT("SavedPatchConfigMas", "Successd to Export the Patch Config.");
			UFlibHotPatcherEditorHelper::CreateSaveFileNotify(Msg, SaveToFile);
		}
	}
}

void SHotPatcherExportRelease::ResetConfig()
{
	UE_LOG(LogTemp, Log, TEXT("Release Clear Config"));

	UExportReleaseSettings* DefaultSetting = NewObject<UExportReleaseSettings>();

	FString DefaultSettingJson;
	DefaultSetting->SerializeReleaseConfigToString(DefaultSettingJson);
	UFlibHotPatcherEditorHelper::DeserializeReleaseConfig(ExportReleaseSettings, DefaultSettingJson);
	SettingsView->ForceRefresh();
}
void SHotPatcherExportRelease::DoGenerate()
{
	UE_LOG(LogTemp, Log, TEXT("Release DoGenerate"));
}

void SHotPatcherExportRelease::CreateExportFilterListView()
{
	// Create a property view
	FPropertyEditorModule& EditModule = FModuleManager::Get().GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bUpdatesFromSelection = true;
	DetailsViewArgs.bLockable = true;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::ComponentsAndActorsUseNameArea;
	DetailsViewArgs.bCustomNameAreaLocation = false;
	DetailsViewArgs.bCustomFilterAreaLocation = true;
	DetailsViewArgs.DefaultsOnlyVisibility = EEditDefaultsOnlyNodeVisibility::Hide;

	SettingsView = EditModule.CreateDetailView(DetailsViewArgs);
}

bool SHotPatcherExportRelease::CanExportRelease()const
{
	bool bCanExport=false;
	if (ExportReleaseSettings)
	{
		bool bHasVersion = !ExportReleaseSettings->GetVersionId().IsEmpty();
		bool bHasFilter = !!ExportReleaseSettings->GetAssetIncludeFiltersPaths().Num();
		bool bHasSpecifyAssets = !!ExportReleaseSettings->GetSpecifyAssets().Num();
		bool bHasSavePath = !(ExportReleaseSettings->GetSavePath().IsEmpty());

		bCanExport = bHasVersion && (bHasFilter||bHasSpecifyAssets) && bHasSavePath;
	}
	return bCanExport;
}

FReply SHotPatcherExportRelease::DoExportRelease()
{
	float AmountOfWorkProgress = 4.0f;
	FScopedSlowTask UnrealPakSlowTask(AmountOfWorkProgress);
	UnrealPakSlowTask.MakeDialog();

	FHotPatcherVersion ExportVersion;
	{
		FText DiaLogMsg = FText::Format(NSLOCTEXT("AnalysisRelease", "AnalysisReleaseVersionInfo", "Analysis Release {0} Assets info."), FText::FromString(ExportReleaseSettings->GetVersionId()));
		UnrealPakSlowTask.EnterProgressFrame(1.0, DiaLogMsg);
		ExportVersion = UFlibPatchParserHelper::ExportReleaseVersionInfo(
			ExportReleaseSettings->GetVersionId(),
			TEXT(""),
			FDateTime::UtcNow().ToString(),
			ExportReleaseSettings->GetAssetIncludeFiltersPaths(),
			ExportReleaseSettings->GetAssetIgnoreFiltersPaths(),
			ExportReleaseSettings->GetAssetRegistryDependencyTypes(),
			ExportReleaseSettings->GetSpecifyAssets(),
			ExportReleaseSettings->GetAllExternFiles(true),
			ExportReleaseSettings->IsIncludeHasRefAssetsOnly(),
			ExportReleaseSettings->IsAnalysisFilterDependencies()
		);
	}
	FString SaveVersionDir = FPaths::Combine(ExportReleaseSettings->GetSavePath(), ExportReleaseSettings->GetVersionId());

	// save release asset info
	{
		FText DiaLogMsg = FText::Format(NSLOCTEXT("ExportReleaseJson", "ExportReleaseVersionInfoJson", "Export Release {0} Assets info to file."), FText::FromString(ExportReleaseSettings->GetVersionId()));
		UnrealPakSlowTask.EnterProgressFrame(1.0, DiaLogMsg);
		FString SaveToJson;
		if (UFlibPatchParserHelper::SerializeHotPatcherVersionToString(ExportVersion, SaveToJson))
		{

			FString SaveToFile = FPaths::Combine(
				SaveVersionDir,
				FString::Printf(TEXT("%s_Release.json"), *ExportReleaseSettings->GetVersionId())
			);
			bool runState = UFLibAssetManageHelperEx::SaveStringToFile(SaveToFile, SaveToJson);
			if (runState)
			{
				auto Message = LOCTEXT("ExportReleaseSuccessNotification", "Succeed to export HotPatcher Release Version.");
				UFlibHotPatcherEditorHelper::CreateSaveFileNotify(Message, SaveToFile);
			}
			UE_LOG(LogTemp, Log, TEXT("HotPatcher Export RELEASE is %s."), runState ? TEXT("Success") : TEXT("FAILD"));
		}
	}

	{
		FText DiaLogMsg = FText::Format(NSLOCTEXT("ExportReleaseConfig", "ExportReleaseConfigJson", "Export Release {0} Configuration to file."), FText::FromString(ExportReleaseSettings->GetVersionId()));
		UnrealPakSlowTask.EnterProgressFrame(1.0, DiaLogMsg);
		FString ConfigJson;
		if (ExportReleaseSettings->SerializeReleaseConfigToString(ConfigJson))
		{
			FString SaveToFile = FPaths::Combine(
				SaveVersionDir,
				FString::Printf(TEXT("%s_ReleaseConfig.json"), *ExportReleaseSettings->GetVersionId())
			);
			bool runState = UFLibAssetManageHelperEx::SaveStringToFile(SaveToFile, ConfigJson);
			if (runState)
			{
				auto Message = LOCTEXT("ExportReleaseConfigSuccessNotification", "Succeed to export HotPatcher Release Config.");
				UFlibHotPatcherEditorHelper::CreateSaveFileNotify(Message, SaveToFile);
			}
			UE_LOG(LogTemp, Log, TEXT("HotPatcher Export RELEASE CONFIG is %s."), runState ? TEXT("Success") : TEXT("FAILD"));
		}
	}

	// save asset dependency
	{
		FText DiaLogMsg = FText::Format(NSLOCTEXT("ExportReleaseAssetDependency", "ExportReleaseAssetDependencyJson", "Export Release {0} Asset Dependency to file."), FText::FromString(ExportReleaseSettings->GetVersionId()));
		UnrealPakSlowTask.EnterProgressFrame(1.0, DiaLogMsg);
		if (ExportReleaseSettings->IsSaveAssetRelatedInfo())
		{
			TArray<FAssetRelatedInfo> AssetsDependency = UFlibPatchParserHelper::GetAssetsRelatedInfoByFAssetDependencies(ExportVersion.AssetInfo,ExportReleaseSettings->GetAssetRegistryDependencyTypes());

			FString AssetsDependencyString;
			UFlibPatchParserHelper::SerializeAssetsRelatedInfoAsString(AssetsDependency, AssetsDependencyString);
			FString SaveAssetRelatedInfoToFile = FPaths::Combine(
				SaveVersionDir,
				FString::Printf(TEXT("%s_AssetRelatedInfos.json"), *ExportVersion.VersionId)
			);
			if (UFLibAssetManageHelperEx::SaveStringToFile(SaveAssetRelatedInfoToFile, AssetsDependencyString))
			{
				auto Msg = LOCTEXT("SaveAssetRelatedInfoInfo", "Succeed to export Asset Related info.");
				UFlibHotPatcherEditorHelper::CreateSaveFileNotify(Msg, SaveAssetRelatedInfoToFile);
			}
		}
	}

	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
