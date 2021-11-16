#include "SGameFeaturePackager.h"
#include "FlibPatchParserHelper.h"
#include "FlibHotPatcherEditorHelper.h"
#include "HotPatcherEditor.h"
#include "FGameFeaturePackagerSettings.h"
#include "CreatePatch/PatcherProxy.h"
#include "CreatePatch/SHotPatcherExportPatch.h"
#include "Interfaces/IPluginManager.h"
#include "Kismet/KismetSystemLibrary.h"

#include "Kismet/KismetTextLibrary.h"

#define LOCTEXT_NAMESPACE "SHotPatcherGameFeaturePackager"

void SHotPatcherGameFeaturePackager::Construct(const FArguments& InArgs,
	TSharedPtr<FHotPatcherCreatePatchModel> InCreateModel)
{
	GameFeaturePackagerSettings = MakeShareable(new FGameFeaturePackagerSettings);
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
                .Text(LOCTEXT("GenerateGameFeature", "Generate GameFeature"))
                .OnClicked(this,&SHotPatcherGameFeaturePackager::DoGameFeaturePackager)
                .IsEnabled(this,&SHotPatcherGameFeaturePackager::CanGameFeaturePackager)
                .ToolTipText(this,&SHotPatcherGameFeaturePackager::GetGenerateTooltipText)
            ]
        ];
}

void SHotPatcherGameFeaturePackager::ImportConfig()
{
	UE_LOG(LogHotPatcher, Log, TEXT("Import Game Feature Packager Config"));
	TArray<FString> Files = this->OpenFileDialog();
	if (!Files.Num()) return;

	FString LoadFile = Files[0];

	FString JsonContent;
	if (UFLibAssetManageHelperEx::LoadFileToString(LoadFile, JsonContent))
	{
		// UFlibHotPatcherEditorHelper::DeserializeReleaseConfig(ExportReleaseSettings, JsonContent);
		UFlibPatchParserHelper::TDeserializeJsonStringAsStruct(JsonContent,*GameFeaturePackagerSettings);
		SettingsView->GetDetailsView()->ForceRefresh();
	}
}

void SHotPatcherGameFeaturePackager::ExportConfig() const
{
	UE_LOG(LogHotPatcher, Log, TEXT("Export Game Feature Packager Config"));
	TArray<FString> Files = this->SaveFileDialog();

	if (!Files.Num()) return;

	FString SaveToFile = Files[0].EndsWith(TEXT(".json")) ? Files[0] : Files[0].Append(TEXT(".json"));

	if (GameFeaturePackagerSettings)
	{
			FString SerializedJsonStr;
			UFlibPatchParserHelper::TSerializeStructAsJsonString(*GameFeaturePackagerSettings,SerializedJsonStr);
			if (FFileHelper::SaveStringToFile(SerializedJsonStr, *SaveToFile))
			{
				FText Msg = LOCTEXT("SavedShaderPatchConfigMas", "Successd to Export the Game Feature Packager Config.");
				UFlibHotPatcherEditorHelper::CreateSaveFileNotify(Msg, SaveToFile);
			}
	}
}

void SHotPatcherGameFeaturePackager::ResetConfig()
{
	UE_LOG(LogHotPatcher, Log, TEXT("Reset Game Feature Packager Config"));
	FString DefaultSettingJson;
	UFlibPatchParserHelper::TSerializeStructAsJsonString(*FGameFeaturePackagerSettings::Get(),DefaultSettingJson);
	UFlibPatchParserHelper::TDeserializeJsonStringAsStruct(DefaultSettingJson,*GameFeaturePackagerSettings);
	SettingsView->GetDetailsView()->ForceRefresh();
}

// #if ENGINE_GAME_FEATURE
// #include "GameFeaturesSubsystem.h"
// #endif
void SHotPatcherGameFeaturePackager::DoGenerate()
{

#if ENGINE_GAME_FEATURE
	if(GetConfigSettings()->bAutoLoadFeature)
	{
		FString FeatureName = GetConfigSettings()->FeatureName;
		FString OutPluginURL;
		UWorld* World = GEditor->GetEditorWorldContext().World();

		if(World)
		{
			FString uPluginFile = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(),FString::Printf(TEXT("Plugins/GameFeatures/%s/%s.uplugin"),*FeatureName,*FeatureName)));
			if(FPaths::FileExists(uPluginFile))
			{
				if(IPluginManager::Get().FindPlugin(TEXT("GameFeatures")).IsValid() &&
				IPluginManager::Get().FindPlugin(TEXT("ModularGameplay")).IsValid() 
					)
				{
					UKismetSystemLibrary::ExecuteConsoleCommand(World,FString::Printf(TEXT("LoadGameFeaturePlugin %s"),*FeatureName));
					UKismetSystemLibrary::ExecuteConsoleCommand(World,FString::Printf(TEXT("DeactivateGameFeaturePlugin %s"),*FeatureName));
				}
				else
				{
					UE_LOG(LogHotPatcher,Warning,TEXT("GameFeatures or ModularGameplay is not Enabled!"));
				}
				if(IPluginManager::Get().FindPlugin(FeatureName))
				{
					FeaturePackager();
				}
				else
				{
					UE_LOG(LogHotPatcher,Error,TEXT("%s load faild, %s"),*GetConfigSettings()->FeatureName);
				}
			}
			else
			{
				UE_LOG(LogHotPatcher,Error,TEXT("%s is not valid uplugin"),*uPluginFile);
			}
		}
		// UGameFeaturesSubsystem::Get().GetPluginURLForBuiltInPluginByName(FeatureName,OutPluginURL);
		// UGameFeaturesSubsystem::Get().LoadGameFeaturePlugin(OutPluginURL,FGameFeaturePluginDeactivateComplete::CreateLambda([this](const UE::GameFeatures::FResult& InStatus)
		// {
		// 	if(InStatus.IsValid() && !InStatus.HasError())
		// 	{
		// 		FeaturePackager();
		// 	}
		// 	else
		// 	{
		// 		UE_LOG(LogHotPatcher,Error,TEXT("Package Feature %s faild, %s"),*GetConfigSettings()->FeatureName,**InStatus.TryGetError());
		// 	}
		// }));
	}
#endif
}

void SHotPatcherGameFeaturePackager::FeaturePackager()
{
	PatchSettings = MakeShareable(new FExportPatchSettings);
	// make patch setting
	{
		PatchSettings->bByBaseVersion = false;
		PatchSettings->VersionId = GetConfigSettings()->FeatureName;
		FDirectoryPath FeaturePluginPath;
		FeaturePluginPath.Path = FString::Printf(TEXT("/%s"),*PatchSettings->VersionId);
		
		PatchSettings->AssetIncludeFilters.Add(FeaturePluginPath);

		FPlatformExternAssets PlatformExternAssets;
		{
			PlatformExternAssets.TargetPlatform = ETargetPlatform::AllPlatforms;
			FExternFileInfo FeaturePlugin;
			FeaturePlugin.Type = EPatchAssetType::NEW;
			FeaturePlugin.MountPath = FString::Printf(TEXT("%s/Plugins/GameFeatures/%s/GF_Feature.uplugin"),*FeaturePlugin.MountPath,*PatchSettings->VersionId);
			FeaturePlugin.FilePath.FilePath = FPaths::ConvertRelativePathToFull(
				FPaths::Combine(
					FPaths::ProjectPluginsDir(),
					TEXT("GameFeatures"),
					PatchSettings->VersionId,
					FString::Printf(TEXT("%s.uplugin"),
						*PatchSettings->VersionId)
						));
			PlatformExternAssets.AddExternFileToPak.Add(FeaturePlugin);
		}
		PatchSettings->AddExternAssetsToPlatform.Add(PlatformExternAssets);
		PatchSettings->bCookPatchAssets = GetConfigSettings()->bCookPatchAssets;
		PatchSettings->SerializeAssetRegistryOptions = GetConfigSettings()->SerializeAssetRegistryOptions;
		PatchSettings->CookShaderOptions = GetConfigSettings()->CookShaderOptions;
		PatchSettings->PakTargetPlatforms.Append(GetConfigSettings()->TargetPlatforms);
		PatchSettings->SavePath.Path = GetConfigSettings()->GetSaveAbsPath();
		PatchSettings->bStorageConfig = true;
	}
	if(!GetConfigSettings()->IsStandaloneMode())
	{
		UPatcherProxy* PatcherProxy = NewObject<UPatcherProxy>();
		PatcherProxy->AddToRoot();
		PatcherProxy->SetProxySettings(PatchSettings.Get());
		// PatcherProxy->OnShowMsg.AddRaw(this,&SHotPatcherExportPatch::ShowMsg);
		PatcherProxy->DoExport();
	}
	else
	{
		FString CurrentConfig;
		UFlibPatchParserHelper::TSerializeStructAsJsonString(*PatchSettings,CurrentConfig);
		FString SaveConfigTo = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("HotPatcher"),FString::Printf(TEXT("%s_GameFeatureConfig.json"),*GetConfigSettings()->FeatureName)));
		FFileHelper::SaveStringToFile(CurrentConfig,*SaveConfigTo);
		FString MissionCommand = FString::Printf(TEXT("\"%s\" -run=HotPatcher -config=\"%s\" %s"),*UFlibPatchParserHelper::GetProjectFilePath(),*SaveConfigTo,*GetConfigSettings()->GetCombinedAdditionalCommandletArgs());
		UE_LOG(LogHotPatcher,Log,TEXT("HotPatcher %s Mission: %s %s"),*GetMissionName(),*UFlibHotPatcherEditorHelper::GetUECmdBinary(),*MissionCommand);
		FHotPatcherEditorModule::Get().RunProcMission(UFlibHotPatcherEditorHelper::GetUECmdBinary(),MissionCommand,GetMissionName());
	}

	if(GetConfigSettings()->IsSaveConfig())
	{
		FString SaveToFile = FPaths::Combine(GetConfigSettings()->GetSaveAbsPath(),FString::Printf(TEXT("%s_GameFeatureConfig.json"),*GetConfigSettings()->FeatureName));
		FString SerializedJsonStr;
		UFlibPatchParserHelper::TSerializeStructAsJsonString(*GameFeaturePackagerSettings,SerializedJsonStr);
		FFileHelper::SaveStringToFile(SerializedJsonStr, *SaveToFile);
	}
}
FText SHotPatcherGameFeaturePackager::GetGenerateTooltipText() const
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
	bool bHasSavePath = GameFeaturePackagerSettings->GetSaveAbsPath().IsEmpty()?false:FPaths::DirectoryExists(GameFeaturePackagerSettings->GetSaveAbsPath());
	AllStatus.Emplace(bHasSavePath,TEXT("HasSavePath"));
	
	for(const auto& Status:AllStatus)
	{
		FinalString+=FString::Printf(TEXT("%s\n"),*Status.GetDisplay());
	}
	return UKismetTextLibrary::Conv_StringToText(FinalString);
}

void SHotPatcherGameFeaturePackager::CreateExportFilterListView()
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
	FStructOnScope* Struct = new FStructOnScope(FGameFeaturePackagerSettings::StaticStruct(), (uint8*)GameFeaturePackagerSettings.Get());
	// SettingsView->GetOnFinishedChangingPropertiesDelegate().AddRaw(ExportReleaseSettings.Get(),&FGameFeaturePackagerSettings::OnFinishedChangingProperties);
	// SettingsView->GetDetailsView()->RegisterInstancedCustomPropertyLayout(FGameFeaturePackagerSettings::StaticStruct(),FOnGetDetailCustomizationInstance::CreateStatic(&FReleaseSettingsDetails::MakeInstance));
	SettingsView->SetStructureData(MakeShareable(Struct));
}

bool SHotPatcherGameFeaturePackager::CanGameFeaturePackager() const
{
	bool bHasSavePath = GameFeaturePackagerSettings->GetSaveAbsPath().IsEmpty()?false:FPaths::DirectoryExists(GameFeaturePackagerSettings->GetSaveAbsPath());
	return HasValidConfig() && bHasSavePath;
}

bool SHotPatcherGameFeaturePackager::HasValidConfig() const
{
	bool bHasTarget = !!GetConfigSettings()->TargetPlatforms.Num();
	return bHasTarget;
}

FReply SHotPatcherGameFeaturePackager::DoGameFeaturePackager()
{
	DoGenerate();
	return FReply::Handled();
}



#undef LOCTEXT_NAMESPACE
