#include "Slate/SShaderPatchWidget.h"
#include "FlibPatchParserHelper.h"
#include "FlibHotPatcherCoreHelper.h"
#include "FlibHotPatcherEditorHelper.h"
#include "HotPatcherEditor.h"
#include "ShaderPatch/FExportShaderPatchSettings.h"
#include "ShaderPatch/FlibShaderPatchHelper.h"
#include "RHI.h"
#include "Kismet/KismetTextLibrary.h"
#include "ShaderPatch/ShaderPatchProxy.h"

#define LOCTEXT_NAMESPACE "SHotPatcherShaderPatch"

void SShaderPatchWidget::Construct(const FArguments& InArgs,
	TSharedPtr<FHotPatcherContextBase> InContext)
{
	SetContext(InContext);
	
	ExportShaderPatchSettings = MakeShareable(new FExportShaderPatchSettings);
	CreateExportFilterListView();
	
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
                .Text(LOCTEXT("GenerateShaderPatch", "Export ShaderPatch"))
                .OnClicked(this,&SShaderPatchWidget::DoExportShaderPatch)
                .IsEnabled(this,&SShaderPatchWidget::CanExportShaderPatch)
                .ToolTipText(this,&SShaderPatchWidget::GetGenerateTooltipText)
            ]
        ];
}

void SShaderPatchWidget::ImportConfig()
{
	UE_LOG(LogHotPatcher, Log, TEXT("Import Shader Patch Config"));
	TArray<FString> Files = this->OpenFileDialog();
	if (!Files.Num()) return;

	FString LoadFile = Files[0];

	FString JsonContent;
	if (UFlibAssetManageHelper::LoadFileToString(LoadFile, JsonContent))
	{
		// UFlibHotPatcherCoreHelper::DeserializeReleaseConfig(ExportReleaseSettings, JsonContent);
		THotPatcherTemplateHelper::TDeserializeJsonStringAsStruct(JsonContent,*ExportShaderPatchSettings);
		SettingsView->GetDetailsView()->ForceRefresh();
	}
}

void SShaderPatchWidget::ExportConfig() const
{
	UE_LOG(LogHotPatcher, Log, TEXT("Export Shader Patch Config"));
	TArray<FString> Files = this->SaveFileDialog();

	if (!Files.Num()) return;

	FString SaveToFile = Files[0].EndsWith(TEXT(".json")) ? Files[0] : Files[0].Append(TEXT(".json"));

	if (ExportShaderPatchSettings)
	{
			FString SerializedJsonStr;
			THotPatcherTemplateHelper::TSerializeStructAsJsonString(*ExportShaderPatchSettings,SerializedJsonStr);
			if (FFileHelper::SaveStringToFile(SerializedJsonStr, *SaveToFile))
			{
				FText Msg = LOCTEXT("SavedShaderPatchConfigMas", "Successd to Export the Shader Patch Config.");
				UFlibHotPatcherEditorHelper::CreateSaveFileNotify(Msg, SaveToFile);
			}
	}
}

void SShaderPatchWidget::ResetConfig()
{
	UE_LOG(LogHotPatcher, Log, TEXT("Reset Shader Patch Config"));
	FString DefaultSettingJson;
	THotPatcherTemplateHelper::TSerializeStructAsJsonString(*FExportShaderPatchSettings::Get(),DefaultSettingJson);
	THotPatcherTemplateHelper::TDeserializeJsonStringAsStruct(DefaultSettingJson,*ExportShaderPatchSettings);
	SettingsView->GetDetailsView()->ForceRefresh();
}

void SShaderPatchWidget::DoGenerate()
{
	UShaderPatchProxy* ShaderPatchProxy = NewObject<UShaderPatchProxy>();
	ShaderPatchProxy->AddToRoot();
	ShaderPatchProxy->Init(ExportShaderPatchSettings.Get());
	if(!ShaderPatchProxy->GetSettingObject()->bStandaloneMode)
	{
		ShaderPatchProxy->DoExport();
	}
	else
	{
		FString CurrentConfig;
		THotPatcherTemplateHelper::TSerializeStructAsJsonString(*GetConfigSettings(),CurrentConfig);
		FString SaveConfigTo = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("HotPatcher"),TEXT("ShaderPatchConfig.json")));
		FFileHelper::SaveStringToFile(CurrentConfig,*SaveConfigTo);
		FString MissionCommand = FString::Printf(TEXT("\"%s\" -run=HotShaderPatch -config=\"%s\" %s"),*UFlibPatchParserHelper::GetProjectFilePath(),*SaveConfigTo,*GetConfigSettings()->GetCombinedAdditionalCommandletArgs());
		UE_LOG(LogHotPatcher,Log,TEXT("HotPatcher %s Mission: %s %s"),*GetMissionName(),*UFlibHotPatcherCoreHelper::GetUECmdBinary(),*MissionCommand);
		FHotPatcherEditorModule::Get().RunProcMission(UFlibHotPatcherCoreHelper::GetUECmdBinary(),MissionCommand,GetMissionName());
	}
}

FText SShaderPatchWidget::GetGenerateTooltipText() const
{
	FString FinalString;
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
	AllStatus.Emplace(HasValidConfig(),TEXT("HasValidShaderPatchConfig"));
	bool bHasSavePath = ExportShaderPatchSettings->GetSaveAbsPath().IsEmpty()?false:FPaths::DirectoryExists(ExportShaderPatchSettings->GetSaveAbsPath());
	AllStatus.Emplace(bHasSavePath,TEXT("HasSavePath"));
	
	for(const auto& Status:AllStatus)
	{
		FinalString+=FString::Printf(TEXT("%s\n"),*Status.GetDisplay());
	}
	return UKismetTextLibrary::Conv_StringToText(FinalString);
}

void SShaderPatchWidget::CreateExportFilterListView()
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
	FStructOnScope* Struct = new FStructOnScope(FExportShaderPatchSettings::StaticStruct(), (uint8*)ExportShaderPatchSettings.Get());
	// SettingsView->GetOnFinishedChangingPropertiesDelegate().AddRaw(ExportReleaseSettings.Get(),&FExportShaderPatchSettings::OnFinishedChangingProperties);
	// SettingsView->GetDetailsView()->RegisterInstancedCustomPropertyLayout(FExportShaderPatchSettings::StaticStruct(),FOnGetDetailCustomizationInstance::CreateStatic(&FReleaseSettingsDetails::MakeInstance));
	SettingsView->SetStructureData(MakeShareable(Struct));
}

bool SShaderPatchWidget::CanExportShaderPatch() const
{
	bool bHasSavePath = ExportShaderPatchSettings->GetSaveAbsPath().IsEmpty()?false:FPaths::DirectoryExists(ExportShaderPatchSettings->GetSaveAbsPath());
	return HasValidConfig() && bHasSavePath;
}

bool SShaderPatchWidget::HasValidConfig() const
{
	auto HasValidDir = [](const TArray<FDirectoryPath>& Dirs)->bool
	{
		bool bresult = false;
		if(!!Dirs.Num())
		{
			for(const auto& Dir:Dirs)
			{
				if(FPaths::DirectoryExists(FPaths::ConvertRelativePathToFull(Dir.Path)))
				{
					bresult = true;
					break;
				}
			}
		}
		return bresult;
	};
	bool HasValidPatchConfig = false;
	for(const auto& Platform:ExportShaderPatchSettings->ShaderPatchConfigs)
	{
		if(HasValidDir(Platform.OldMetadataDir) &&
		FPaths::DirectoryExists(Platform.NewMetadataDir.Path))
		{
			HasValidPatchConfig = true;
			break;
		}
	}
	return HasValidPatchConfig;
}

FReply SShaderPatchWidget::DoExportShaderPatch()
{
	DoGenerate();
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE