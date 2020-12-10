#include "ShaderPatch/SHotPatcherExportShaderPatch.h"
#include "FlibPatchParserHelper.h"
#include "FlibHotPatcherEditorHelper.h"
#include "ShaderPatch/FExportShaderPatchSettings.h"
#include "ShaderPatch/FlibShaderPatchHelper.h"

#define LOCTEXT_NAMESPACE "SHotPatcherShaderPatch"

void SHotPatcherExportShaderPatch::Construct(const FArguments& InArgs,
	TSharedPtr<FHotPatcherCreatePatchModel> InCreateModel)
{
	ExportShaderPatchSettings = MakeShareable(new FExportShaderPatchSettings);
	CreateExportFilterListView();

	mCreatePatchModel = InCreateModel;

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
                .OnClicked(this,&SHotPatcherExportShaderPatch::DoExportShaderPatch)
                .IsEnabled(this,&SHotPatcherExportShaderPatch::CanExportShaderPatch)
            ]
        ];

}

void SHotPatcherExportShaderPatch::ImportConfig()
{
	UE_LOG(LogHotPatcher, Log, TEXT("Import Shader Patch Config"));
	TArray<FString> Files = this->OpenFileDialog();
	if (!Files.Num()) return;

	FString LoadFile = Files[0];

	FString JsonContent;
	if (UFLibAssetManageHelperEx::LoadFileToString(LoadFile, JsonContent))
	{
		// UFlibHotPatcherEditorHelper::DeserializeReleaseConfig(ExportReleaseSettings, JsonContent);
		UFlibPatchParserHelper::TDeserializeJsonStringAsStruct(JsonContent,*ExportShaderPatchSettings);
		SettingsView->GetDetailsView()->ForceRefresh();
	}
}

void SHotPatcherExportShaderPatch::ExportConfig() const
{
	UE_LOG(LogHotPatcher, Log, TEXT("Export Shader Patch Config"));
	TArray<FString> Files = this->SaveFileDialog();

	if (!Files.Num()) return;

	FString SaveToFile = Files[0].EndsWith(TEXT(".json")) ? Files[0] : Files[0].Append(TEXT(".json"));

	if (ExportShaderPatchSettings)
	{
			FString SerializedJsonStr;
			UFlibPatchParserHelper::TSerializeStructAsJsonString(*ExportShaderPatchSettings,SerializedJsonStr);
			if (FFileHelper::SaveStringToFile(SerializedJsonStr, *SaveToFile))
			{
				FText Msg = LOCTEXT("SavedShaderPatchConfigMas", "Successd to Export the Shader Patch Config.");
				UFlibHotPatcherEditorHelper::CreateSaveFileNotify(Msg, SaveToFile);
			}
	}
}

void SHotPatcherExportShaderPatch::ResetConfig()
{
	UE_LOG(LogHotPatcher, Log, TEXT("Reset Shader Patch Config"));
	FString DefaultSettingJson;
	UFlibPatchParserHelper::TSerializeStructAsJsonString(*FExportPatchSettings::Get(),DefaultSettingJson);
	UFlibPatchParserHelper::TDeserializeJsonStringAsStruct(DefaultSettingJson,*ExportShaderPatchSettings);
	SettingsView->GetDetailsView()->ForceRefresh();
}

void SHotPatcherExportShaderPatch::DoGenerate()
{
	for(const auto& PlatformConfig:ExportShaderPatchSettings->ShaderPatchConfigs)
	{
		FString SaveToPath = FPaths::Combine(FPaths::ConvertRelativePathToFull(ExportShaderPatchSettings->SaveTo.Path),UFlibPatchParserHelper::GetEnumNameByValue(PlatformConfig.Platform));
		UFlibShaderPatchHelper::CreateShaderCodePatch(
        UFlibShaderPatchHelper::ConvDirectoryPathToStr(PlatformConfig.OldMetadataDir),
        FPaths::ConvertRelativePathToFull(PlatformConfig.NewMetadataDir.Path),
        SaveToPath,
        PlatformConfig.bNativeFormat
        );
	}
	
}

void SHotPatcherExportShaderPatch::CreateExportFilterListView()
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

bool SHotPatcherExportShaderPatch::CanExportShaderPatch() const
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
	return HasValidPatchConfig && FPaths::DirectoryExists(ExportShaderPatchSettings->SaveTo.Path);
}

FReply SHotPatcherExportShaderPatch::DoExportShaderPatch()
{
	DoGenerate();
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE