// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

// #include "HotPatcherPrivatePCH.h"
#include "CreatePatch/SHotPatcherReleaseWidget.h"
#include "FlibHotPatcherCoreHelper.h"
#include "FlibAssetManageHelper.h"
#include "AssetManager/FAssetDependenciesInfo.h"
#include "FHotPatcherVersion.h"
#include "HotPatcherLog.h"
#include "CreatePatch/ReleaseSettingsDetails.h"
#include "CreatePatch/FExportReleaseSettings.h"
#include "HotPatcherEditor.h"

// engine header
#include "FlibHotPatcherEditorHelper.h"
#include "CreatePatch/ReleaseProxy.h"
#include "Widgets/Input/SHyperlink.h"
#include "Widgets/Layout/SSeparator.h"
#include "HAL/FileManager.h"
#include "Kismet/KismetTextLibrary.h"
#include "Misc/FileHelper.h"
#include "Misc/ScopedSlowTask.h"
#include "Misc/EngineVersionComparison.h"

#if !UE_VERSION_OLDER_THAN(5,1,0)
	typedef FAppStyle FEditorStyle;
#endif

#define LOCTEXT_NAMESPACE "SHotPatcherExportRelease"

void SHotPatcherReleaseWidget::Construct(const FArguments& InArgs, TSharedPtr<FHotPatcherContextBase> InCreatePatchModel)
{
	ExportReleaseSettings = MakeShareable(new FExportReleaseSettings);
	GReleaseSettings = ExportReleaseSettings.Get();
	CreateExportFilterListView();
	mContext = InCreatePatchModel;

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
				.OnClicked(this,&SHotPatcherReleaseWidget::DoExportRelease)
				.IsEnabled(this,&SHotPatcherReleaseWidget::CanExportRelease)
				.ToolTipText(this,&SHotPatcherReleaseWidget::GetGenerateTooltipText)
			]
		];
}

void SHotPatcherReleaseWidget::ImportConfig()
{
	UE_LOG(LogHotPatcher, Log, TEXT("Release Import Config"));
	TArray<FString> Files = this->OpenFileDialog();
	if (!Files.Num()) return;

	FString LoadFile = Files[0];

	FString JsonContent;
	if (UFlibAssetManageHelper::LoadFileToString(LoadFile, JsonContent))
	{
		// UFlibHotPatcherCoreHelper::DeserializeReleaseConfig(ExportReleaseSettings, JsonContent);
		THotPatcherTemplateHelper::TDeserializeJsonStringAsStruct(JsonContent,*ExportReleaseSettings);
		// adaptor old version config
		UFlibHotPatcherCoreHelper::AdaptorOldVersionConfig(ExportReleaseSettings->GetAssetScanConfigRef(),JsonContent);
		SettingsView->GetDetailsView()->ForceRefresh();
	}
}

void SHotPatcherReleaseWidget::ExportConfig()const
{
	UE_LOG(LogHotPatcher, Log, TEXT("Release Export Config"));
	TArray<FString> Files = this->SaveFileDialog();

	if (!Files.Num()) return;

	FString SaveToFile = Files[0].EndsWith(TEXT(".json")) ? Files[0] : Files[0].Append(TEXT(".json"));

	if (ExportReleaseSettings)
	{

		FString SerializedJsonStr;
		THotPatcherTemplateHelper::TSerializeStructAsJsonString(*ExportReleaseSettings,SerializedJsonStr);
		if (FFileHelper::SaveStringToFile(SerializedJsonStr, *SaveToFile))
		{
			FText Msg = LOCTEXT("SavedPatchConfigMas", "Successd to Export the Patch Config.");
			UFlibHotPatcherEditorHelper::CreateSaveFileNotify(Msg, SaveToFile);
		}
	}
}

void SHotPatcherReleaseWidget::ResetConfig()
{
	UE_LOG(LogHotPatcher, Log, TEXT("Release Clear Config"));
	FString DefaultSettingJson;
	THotPatcherTemplateHelper::TSerializeStructAsJsonString(*FExportReleaseSettings::Get(),DefaultSettingJson);
	THotPatcherTemplateHelper::TDeserializeJsonStringAsStruct(DefaultSettingJson,*ExportReleaseSettings);
	SettingsView->GetDetailsView()->ForceRefresh();
}
void SHotPatcherReleaseWidget::DoGenerate()
{
	UE_LOG(LogHotPatcher, Log, TEXT("Release DoGenerate"));
}

void SHotPatcherReleaseWidget::CreateExportFilterListView()
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

bool SHotPatcherReleaseWidget::CanExportRelease()const
{
	bool bCanExport=false;
	if (ExportReleaseSettings)
	{
		bool bHasVersion = !ExportReleaseSettings->GetVersionId().IsEmpty();
		bool bHasFilter = !!ExportReleaseSettings->GetAssetIncludeFilters().Num();
		bool bHasSpecifyAssets = !!ExportReleaseSettings->GetSpecifyAssets().Num();
		bool bHasSavePath = !(ExportReleaseSettings->GetSaveAbsPath().IsEmpty());
		bool bHasPakInfo = !!ExportReleaseSettings->GetPlatformsPakListFiles().Num();
		bCanExport = bHasVersion && (ExportReleaseSettings->IsImportProjectSettings() || bHasFilter || bHasSpecifyAssets || bHasPakInfo) && bHasSavePath;
	}
	return bCanExport;
}

FReply SHotPatcherReleaseWidget::DoExportRelease()
{
	if(!GetConfigSettings()->IsStandaloneMode())
	{
		UReleaseProxy* ReleaseProxy = NewObject<UReleaseProxy>();
		ReleaseProxy->AddToRoot();
		ReleaseProxy->Init(ExportReleaseSettings.Get());
		ReleaseProxy->DoExport();
	}
	else
	{
		FString CurrentConfig;
		THotPatcherTemplateHelper::TSerializeStructAsJsonString(*GetConfigSettings(),CurrentConfig);
		FString SaveConfigTo = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("HotPatcher"),TEXT("ReleaseConfig.json")));
		FFileHelper::SaveStringToFile(CurrentConfig,*SaveConfigTo);
		FString NoShaderCompile = GetConfigSettings()->bNoShaderCompile ? TEXT("-NoShaderCompile") : TEXT("");
		FString MissionCommand = FString::Printf(TEXT("\"%s\" -run=HotRelease -config=\"%s\" %s %s"),*UFlibPatchParserHelper::GetProjectFilePath(),*SaveConfigTo,*GetConfigSettings()->GetCombinedAdditionalCommandletArgs(),*NoShaderCompile);
		UE_LOG(LogHotPatcher,Log,TEXT("HotPatcher %s Mission: %s %s"),*GetMissionName(),*UFlibHotPatcherCoreHelper::GetUECmdBinary(),*MissionCommand);
		FHotPatcherEditorModule::Get().RunProcMission(UFlibHotPatcherCoreHelper::GetUECmdBinary(),MissionCommand,GetMissionName());
	}
	return FReply::Handled();
}

FText SHotPatcherReleaseWidget::GetGenerateTooltipText() const
{
	FString FinalString;
	if (GetMutableDefault<UHotPatcherSettings>()->bPreviewTooltips && ExportReleaseSettings)
	{
		bool bHasVersion = !ExportReleaseSettings->GetVersionId().IsEmpty();
		bool bHasFilter = !!ExportReleaseSettings->GetAssetIncludeFilters().Num();
		bool bHasSpecifyAssets = !!ExportReleaseSettings->GetSpecifyAssets().Num();
		bool bHasExternFiles = !!ExportReleaseSettings->GetAddExternAssetsToPlatform().Num();
		bool bHasPakInfo = !!ExportReleaseSettings->GetPlatformsPakListFiles().Num();
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
		AllStatus.Emplace((ExportReleaseSettings->IsImportProjectSettings()||bHasFilter||bHasSpecifyAssets||bHasExternFiles||bHasPakInfo),TEXT("ImportProjectSettings or HasFilter or HasSpecifyAssets or bHasExternFiles or bHasPakInfo"));
		AllStatus.Emplace(bHasSavePath,TEXT("HasSavePath"));
		
		for(const auto& Status:AllStatus)
		{
			FinalString+=FString::Printf(TEXT("%s\n"),*Status.GetDisplay());
		}
	}
	return UKismetTextLibrary::Conv_StringToText(FinalString);
}

#undef LOCTEXT_NAMESPACE
