// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

// #include "HotPatcherPrivatePCH.h"
#include "CreatePatch/SHotPatcherExportRelease.h"
#include "FlibHotPatcherEditorHelper.h"
#include "FLibAssetManageHelperEx.h"
#include "AssetManager/FAssetDependenciesInfo.h"
#include "FHotPatcherVersion.h"
#include "HotPatcherLog.h"
// engine header
#include "CreatePatch/ReleaseProxy.h"
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
	UE_LOG(LogHotPatcher, Log, TEXT("Release Import Config"));
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
	UE_LOG(LogHotPatcher, Log, TEXT("Release Export Config"));
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
	UE_LOG(LogHotPatcher, Log, TEXT("Release Clear Config"));

	UExportReleaseSettings* DefaultSetting = NewObject<UExportReleaseSettings>();

	FString DefaultSettingJson;
	DefaultSetting->SerializeReleaseConfigToString(DefaultSettingJson);
	UFlibHotPatcherEditorHelper::DeserializeReleaseConfig(ExportReleaseSettings, DefaultSettingJson);
	SettingsView->ForceRefresh();
}
void SHotPatcherExportRelease::DoGenerate()
{
	UE_LOG(LogHotPatcher, Log, TEXT("Release DoGenerate"));
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
	UReleaseProxy* ReleaseProxy = NewObject<UReleaseProxy>();
	ReleaseProxy->SetProxySettings(ExportReleaseSettings);
	ReleaseProxy->DoExport();
	
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
