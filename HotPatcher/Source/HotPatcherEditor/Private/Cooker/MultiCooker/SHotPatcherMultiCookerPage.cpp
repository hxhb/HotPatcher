// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

// #include "HotPatcherPrivatePCH.h"
#include "SHotPatcherMultiCookerPage.h"
#include "FlibHotPatcherEditorHelper.h"
#include "FLibAssetManageHelperEx.h"
#include "HotPatcherLog.h"
#include "HotPatcherEditor.h"
#include "Cooker/MultiCooker/FMultiCookerSettings.h"
// engine header
#include "FlibMultiCookerHelper.h"
#include "Cooker/MultiCooker/MultiCookerProxy.h"
#include "Widgets/Input/SHyperlink.h"
#include "Widgets/Layout/SSeparator.h"
#include "HAL/FileManager.h"
#include "Kismet/KismetTextLibrary.h"
#include "Misc/FileHelper.h"
#include "Misc/ScopedSlowTask.h"

#define LOCTEXT_NAMESPACE "SHotPatcherMultiCookerPage"

void SHotPatcherMultiCookerPage::Construct(const FArguments& InArgs, TSharedPtr<FHotPatcherCookerModel> InCreatePatchModel)
{
	CookerSettings = MakeShareable(new FMultiCookerSettings);
	CreateExportFilterListView();

	MissionNotifyProay = NewObject<UMissionNotificationProxy>();
	MissionNotifyProay->AddToRoot();
	
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
				.Text(LOCTEXT("CookContent", "Cook Content"))
				.OnClicked(this,&SHotPatcherMultiCookerPage::RunCook)
				.IsEnabled(this,&SHotPatcherMultiCookerPage::CanCook)
				.ToolTipText(this,&SHotPatcherMultiCookerPage::GetGenerateTooltipText)
			]
		];
}

void SHotPatcherMultiCookerPage::ImportConfig()
{
	UE_LOG(LogHotPatcher, Log, TEXT("Release Import Config"));
	TArray<FString> Files = this->OpenFileDialog();
	if (!Files.Num()) return;

	FString LoadFile = Files[0];

	FString JsonContent;
	if (UFLibAssetManageHelperEx::LoadFileToString(LoadFile, JsonContent))
	{
		// UFlibHotPatcherEditorHelper::DeserializeReleaseConfig(ExportReleaseSettings, JsonContent);
		UFlibPatchParserHelper::TDeserializeJsonStringAsStruct(JsonContent,*CookerSettings);
		SettingsView->GetDetailsView()->ForceRefresh();
	}
}

void SHotPatcherMultiCookerPage::ExportConfig()const
{
	UE_LOG(LogHotPatcher, Log, TEXT("Release Export Config"));
	TArray<FString> Files = this->SaveFileDialog();

	if (!Files.Num()) return;

	FString SaveToFile = Files[0].EndsWith(TEXT(".json")) ? Files[0] : Files[0].Append(TEXT(".json"));

	if (CookerSettings)
	{

		FString SerializedJsonStr;
		UFlibPatchParserHelper::TSerializeStructAsJsonString(*CookerSettings,SerializedJsonStr);
		if (FFileHelper::SaveStringToFile(SerializedJsonStr, *SaveToFile))
		{
			FText Msg = LOCTEXT("SavedPatchConfigMas", "Successd to Export the Patch Config.");
			UFlibHotPatcherEditorHelper::CreateSaveFileNotify(Msg, SaveToFile);
		}
	}
}

void SHotPatcherMultiCookerPage::ResetConfig()
{
	UE_LOG(LogHotPatcher, Log, TEXT("Release Clear Config"));
	FString DefaultSettingJson;
	UFlibPatchParserHelper::TSerializeStructAsJsonString(*FExportReleaseSettings::Get(),DefaultSettingJson);
	UFlibPatchParserHelper::TDeserializeJsonStringAsStruct(DefaultSettingJson,*CookerSettings);
	SettingsView->GetDetailsView()->ForceRefresh();
}
void SHotPatcherMultiCookerPage::DoGenerate()
{
	UE_LOG(LogHotPatcher, Log, TEXT("Release DoGenerate"));
}

void SHotPatcherMultiCookerPage::ImportProjectConfig()
{
	GetConfigSettings()->ImportProjectSettings();
	SHotPatcherCookerBase::ImportProjectConfig();
}

void SHotPatcherMultiCookerPage::CreateExportFilterListView()
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
	FStructOnScope* Struct = new FStructOnScope(FMultiCookerSettings::StaticStruct(), (uint8*)CookerSettings.Get());
	// SettingsView->GetOnFinishedChangingPropertiesDelegate().AddRaw(CookerSettings.Get(),&FExportReleaseSettings::OnFinishedChangingProperties);
	// SettingsView->GetDetailsView()->RegisterInstancedCustomPropertyLayout(FExportReleaseSettings::StaticStruct(),FOnGetDetailCustomizationInstance::CreateStatic(&FMultiCookerSettings::MakeInstance));
	SettingsView->SetStructureData(MakeShareable(Struct));
}

bool SHotPatcherMultiCookerPage::CanCook()const
{
	return ((SHotPatcherMultiCookerPage*)(this))->GetConfigSettings()->IsValidConfig() && (MultiCookerProxy ? !MultiCookerProxy->IsRunning() : true);
}

FReply SHotPatcherMultiCookerPage::RunCook()
{
	if(!GetConfigSettings()->IsStandaloneMode())
	{
		MultiCookerProxy = NewObject<UMultiCookerProxy>();
		MultiCookerProxy->AddToRoot();
		MultiCookerProxy->SetProxySettings(GetConfigSettings());
		MultiCookerProxy->OnMultiCookerBegining.AddLambda([this](UMultiCookerProxy*)
		{
			MissionNotifyProay->SpawnRuningMissionNotification(NULL);
		});
		MultiCookerProxy->OnMultiCookerFinished.AddLambda([this](UMultiCookerProxy* InMultiCookerProxy)
		{
            MultiCookerProxy->Shutdown();
			if(MultiCookerProxy->HasError())
			{
				MissionNotifyProay->SpawnMissionFaildNotification(NULL);
			}
			else
			{
				MissionNotifyProay->SpawnMissionSuccessedNotification(NULL);
			}
		});
		MissionNotifyProay->MissionCanceled.AddLambda([this]()
		{
			if(MultiCookerProxy && MultiCookerProxy->IsRunning())
			{
				MultiCookerProxy->Cancel();
			}
		});
		FString MissionName = GetMissionName();
		MissionNotifyProay->SetMissionName(*FString::Printf(TEXT("%s"),*MissionName));
		MissionNotifyProay->SetMissionNotifyText(
			FText::FromString(FString::Printf(TEXT("%s in progress"),*MissionName)),
			LOCTEXT("RunningCookNotificationCancelButton", "Cancel"),
			FText::FromString(FString::Printf(TEXT("%s Mission Finished!"),*MissionName)),
			FText::FromString(FString::Printf(TEXT("%s Failed!"),*MissionName))
		);
		MultiCookerProxy->Init();
		MultiCookerProxy->DoExport();
	}
	else
	{
		FString CurrentConfig;
		UFlibPatchParserHelper::TSerializeStructAsJsonString(*GetConfigSettings(),CurrentConfig);
		FString SaveConfigTo = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("HotPatcher/MultiCooker/"),TEXT("MultiCookerConfig.json")));
		FFileHelper::SaveStringToFile(CurrentConfig,*SaveConfigTo);
		FString ProfilingCmd = GetConfigSettings()->bProfilingMultiCooker ? UFlibMultiCookerHelper::GetProfilingCmd() : TEXT("");
		
		FString MissionCommand = FString::Printf(
			TEXT("\"%s\" -run=HotMultiCooker -config=\"%s\" -norenderthread %s %s"),
			*UFlibPatchParserHelper::GetProjectFilePath(),*SaveConfigTo,
			*ProfilingCmd,
			*GetConfigSettings()->GetCombinedAdditionalCommandletArgs()
			);
		
		UE_LOG(LogHotPatcher,Log,TEXT("HotPatcher %s Mission: %s %s"),*GetMissionName(),*UFlibHotPatcherEditorHelper::GetUECmdBinary(),*MissionCommand);
		FHotPatcherEditorModule::Get().RunProcMission(UFlibHotPatcherEditorHelper::GetUECmdBinary(),MissionCommand,GetMissionName());
    
	}
	return FReply::Handled();
}

FText SHotPatcherMultiCookerPage::GetGenerateTooltipText() const
{
	FString FinalString;
	return UKismetTextLibrary::Conv_StringToText(FinalString);
}

#undef LOCTEXT_NAMESPACE
