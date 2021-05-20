// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

// #include "HotPatcherPrivatePCH.h"
#include "CreatePatch/SHotPatcherExportRelease.h"
#include "FlibHotPatcherEditorHelper.h"
#include "FLibAssetManageHelperEx.h"
#include "AssetManager/FAssetDependenciesInfo.h"
#include "FHotPatcherVersion.h"
#include "HotPatcherLog.h"
#include "CreatePatch/ReleaseSettingsDetails.h"
#include "CreatePatch/FExportReleaseSettings.h"
#include "HotPatcherEditor.h"

// engine header
#include "CreatePatch/ReleaseProxy.h"
#include "Widgets/Input/SHyperlink.h"
#include "Widgets/Layout/SSeparator.h"
#include "HAL/FileManager.h"
#include "Kismet/KismetTextLibrary.h"
#include "Misc/FileHelper.h"
#include "Misc/ScopedSlowTask.h"

#define LOCTEXT_NAMESPACE "SHotPatcherExportRelease"

void SHotPatcherExportRelease::Construct(const FArguments& InArgs, TSharedPtr<FHotPatcherCreatePatchModel> InCreatePatchModel)
{
	ExportReleaseSettings = MakeShareable(new FExportReleaseSettings);
	GReleaseSettings = ExportReleaseSettings.Get();
	CreateExportFilterListView();

	mCreatePatchModel = InCreatePatchModel;
	InitMissionNotificationProxy();
	
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
					SettingsView->GetWidget()->AsShared()
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
				.ToolTipText(this,&SHotPatcherExportRelease::GetGenerateTooltipText)
			]
		];
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
		// UFlibHotPatcherEditorHelper::DeserializeReleaseConfig(ExportReleaseSettings, JsonContent);
		UFlibPatchParserHelper::TDeserializeJsonStringAsStruct(JsonContent,*ExportReleaseSettings);
		SettingsView->GetDetailsView()->ForceRefresh();
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
		UFlibPatchParserHelper::TSerializeStructAsJsonString(*ExportReleaseSettings,SerializedJsonStr);
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
	FString DefaultSettingJson;
	UFlibPatchParserHelper::TSerializeStructAsJsonString(*FExportReleaseSettings::Get(),DefaultSettingJson);
	UFlibPatchParserHelper::TDeserializeJsonStringAsStruct(DefaultSettingJson,*ExportReleaseSettings);
	SettingsView->GetDetailsView()->ForceRefresh();
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
	{
		DetailsViewArgs.bAllowSearch = true;
		DetailsViewArgs.bHideSelectionTip = true;
		DetailsViewArgs.bLockable = false;
		DetailsViewArgs.bSearchInitialKeyFocus = true;
		DetailsViewArgs.bUpdatesFromSelection = false;
		DetailsViewArgs.NotifyHook = nullptr;
		DetailsViewArgs.bShowOptions = true;
		DetailsViewArgs.bShowModifiedPropertiesOption = false;
		DetailsViewArgs.bShowScrollBar = false;
		DetailsViewArgs.bShowOptions = true;
		DetailsViewArgs.bUpdatesFromSelection= true;
	}

	FStructureDetailsViewArgs StructureViewArgs;
	{
		StructureViewArgs.bShowObjects = true;
		StructureViewArgs.bShowAssets = true;
		StructureViewArgs.bShowClasses = true;
		StructureViewArgs.bShowInterfaces = true;
	}

	SettingsView = EditModule.CreateStructureDetailView(DetailsViewArgs, StructureViewArgs, nullptr);
	FStructOnScope* Struct = new FStructOnScope(FExportReleaseSettings::StaticStruct(), (uint8*)ExportReleaseSettings.Get());
	SettingsView->GetOnFinishedChangingPropertiesDelegate().AddRaw(ExportReleaseSettings.Get(),&FExportReleaseSettings::OnFinishedChangingProperties);
	SettingsView->GetDetailsView()->RegisterInstancedCustomPropertyLayout(FExportReleaseSettings::StaticStruct(),FOnGetDetailCustomizationInstance::CreateStatic(&FReleaseSettingsDetails::MakeInstance));
	SettingsView->SetStructureData(MakeShareable(Struct));
}

bool SHotPatcherExportRelease::CanExportRelease()const
{
	bool bCanExport=false;
	if (ExportReleaseSettings)
	{
		bool bHasVersion = !ExportReleaseSettings->GetVersionId().IsEmpty();
		bool bHasFilter = !!ExportReleaseSettings->GetAssetIncludeFiltersPaths().Num();
		bool bHasSpecifyAssets = !!ExportReleaseSettings->GetSpecifyAssets().Num();
		bool bHasSavePath = !(ExportReleaseSettings->GetSaveAbsPath().IsEmpty());

		bCanExport = bHasVersion && (bHasFilter||bHasSpecifyAssets) && bHasSavePath;
	}
	return bCanExport;
}

FReply SHotPatcherExportRelease::DoExportRelease()
{
	if(!GetConfigSettings()->IsStandaloneMode())
	{
		UReleaseProxy* ReleaseProxy = NewObject<UReleaseProxy>();
		ReleaseProxy->AddToRoot();
		ReleaseProxy->SetProxySettings(ExportReleaseSettings.Get());
		ReleaseProxy->DoExport();
	}
	else
	{
		FString CurrentConfig;
		UFlibPatchParserHelper::TSerializeStructAsJsonString(*GetConfigSettings(),CurrentConfig);
		FString SaveConfigTo = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("HotPacther"),TEXT("ReleaseConfig.json")));
		FFileHelper::SaveStringToFile(CurrentConfig,*SaveConfigTo);
		FString MissionCommand = FString::Printf(TEXT("\"%s\" -run=HotRelease -config=\"%s\""),*UFlibPatchParserHelper::GetProjectFilePath(),*SaveConfigTo);
		UE_LOG(LogHotPatcher,Log,TEXT("HotPatcher %s Mission: %s %s"),*GetMissionName(),*UFlibHotPatcherEditorHelper::GetUE4CmdBinary(),*MissionCommand);
		RunProcMission(UFlibHotPatcherEditorHelper::GetUE4CmdBinary(),MissionCommand);
	}
	return FReply::Handled();
}

FText SHotPatcherExportRelease::GetGenerateTooltipText() const
{
	FString FinalString;
	if (ExportReleaseSettings)
	{
		bool bHasVersion = !ExportReleaseSettings->GetVersionId().IsEmpty();
		bool bHasFilter = !!ExportReleaseSettings->GetAssetIncludeFiltersPaths().Num();
		bool bHasSpecifyAssets = !!ExportReleaseSettings->GetSpecifyAssets().Num();
		bool bHasExternFiles = !!ExportReleaseSettings->GetAddExternAssetsToPlatform().Num();
		bool bHasSavePath = !(ExportReleaseSettings->GetSaveAbsPath().IsEmpty());

		struct FStatus
		{
			FStatus(bool InMatch,const FString& InDisplay):bMatch(InMatch)
			{
				Display = FString::Printf(TEXT("%s:%s"),*InDisplay,InMatch?TEXT("true"):TEXT("false"));
			}
			FString GetDisplay()const{return Display;}
			bool bMatch;
			FString Display;
		};
		TArray<FStatus> AllStatus;
		AllStatus.Emplace(bHasVersion,TEXT("HasVersion"));
		AllStatus.Emplace((bHasFilter||bHasSpecifyAssets||bHasExternFiles),TEXT("HasFilter or HasSpecifyAssets or bHasExternFiles"));
		AllStatus.Emplace(bHasSavePath,TEXT("HasSavePath"));
		
		for(const auto& Status:AllStatus)
		{
			FinalString+=FString::Printf(TEXT("%s\n"),*Status.GetDisplay());
		}
	}
	return UKismetTextLibrary::Conv_StringToText(FinalString);
}

#undef LOCTEXT_NAMESPACE
