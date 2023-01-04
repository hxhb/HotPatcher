#include "SGameFeaturePackageWidget.h"
#include "FlibPatchParserHelper.h"
#include "FlibHotPatcherCoreHelper.h"
#include "FlibHotPatcherEditorHelper.h"
#include "HotPatcherEditor.h"
#include "GameFeature/FGameFeaturePackagerSettings.h"
#include "CreatePatch/PatcherProxy.h"
#include "CreatePatch/SHotPatcherPatchWidget.h"
#include "GameFeature/GameFeatureProxy.h"
#include "Interfaces/IPluginManager.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetTextLibrary.h"
#include "Misc/EngineVersionComparison.h"

#if !UE_VERSION_OLDER_THAN(5,1,0)
	typedef FAppStyle FEditorStyle;
#endif

#define LOCTEXT_NAMESPACE "SHotPatcherGameFeaturePackager"

void SGameFeaturePackageWidget::Construct(const FArguments& InArgs,
	TSharedPtr<FHotPatcherContextBase> InCreateModel)
{
	GameFeaturePackagerSettings = MakeShareable(new FGameFeaturePackagerSettings);
	CreateExportFilterListView();
	
	SetContext(InCreateModel);
	
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
                .OnClicked(this,&SGameFeaturePackageWidget::DoGameFeaturePackager)
                .IsEnabled(this,&SGameFeaturePackageWidget::CanGameFeaturePackager)
                .ToolTipText(this,&SGameFeaturePackageWidget::GetGenerateTooltipText)
            ]
        ];
}

void SGameFeaturePackageWidget::ImportConfig()
{
	UE_LOG(LogHotPatcher, Log, TEXT("Import Game Feature Packager Config"));
	TArray<FString> Files = this->OpenFileDialog();
	if (!Files.Num()) return;

	FString LoadFile = Files[0];

	FString JsonContent;
	if (UFlibAssetManageHelper::LoadFileToString(LoadFile, JsonContent))
	{
		// UFlibHotPatcherCoreHelper::DeserializeReleaseConfig(ExportReleaseSettings, JsonContent);
		THotPatcherTemplateHelper::TDeserializeJsonStringAsStruct(JsonContent,*GameFeaturePackagerSettings);
		SettingsView->GetDetailsView()->ForceRefresh();
	}
}

void SGameFeaturePackageWidget::ExportConfig() const
{
	UE_LOG(LogHotPatcher, Log, TEXT("Export Game Feature Packager Config"));
	TArray<FString> Files = this->SaveFileDialog();

	if (!Files.Num()) return;

	FString SaveToFile = Files[0].EndsWith(TEXT(".json")) ? Files[0] : Files[0].Append(TEXT(".json"));

	if (GameFeaturePackagerSettings)
	{
			FString SerializedJsonStr;
			THotPatcherTemplateHelper::TSerializeStructAsJsonString(*GameFeaturePackagerSettings,SerializedJsonStr);
			if (FFileHelper::SaveStringToFile(SerializedJsonStr, *SaveToFile))
			{
				FText Msg = LOCTEXT("SavedGameFeatureConfigMas", "Successd to Export the Game Feature Packager Config.");
				UFlibHotPatcherEditorHelper::CreateSaveFileNotify(Msg, SaveToFile);
			}
	}
}

void SGameFeaturePackageWidget::ResetConfig()
{
	UE_LOG(LogHotPatcher, Log, TEXT("Reset Game Feature Packager Config"));
	FString DefaultSettingJson;
	THotPatcherTemplateHelper::TSerializeStructAsJsonString(*FGameFeaturePackagerSettings::Get(),DefaultSettingJson);
	THotPatcherTemplateHelper::TDeserializeJsonStringAsStruct(DefaultSettingJson,*GameFeaturePackagerSettings);
	SettingsView->GetDetailsView()->ForceRefresh();
}

// #if ENGINE_GAME_FEATURE
// #include "GameFeaturesSubsystem.h"
// #endif
void SGameFeaturePackageWidget::DoGenerate()
{
#if ENGINE_GAME_FEATURE
	if(GetConfigSettings()->bAutoLoadFeaturePlugin)
	{
		// FString FeatureName = GetConfigSettings()->FeatureName;
		FString OutPluginURL;
		UWorld* World = GEditor->GetEditorWorldContext().World();

		for(const auto& FeatureName:GetConfigSettings()->FeatureNames)
		{
			if(World)
			{
				auto GameFeatureFounder = [](const FString& FeatureName)
				{
					bool bFound = false;
					FString FeaturePluginDir = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectPluginsDir(),TEXT("GameFeatures"),FeatureName));
					FString FeatureUPluginPath = FPaths::Combine(FeaturePluginDir,FString::Printf(TEXT("%s.uplugin"),*FeatureName));
				
					if(FPaths::DirectoryExists(FeaturePluginDir) && FPaths::FileExists(FeatureUPluginPath))
					{
						bFound = true;
					}
					return bFound;
				};
			
				if(IPluginManager::Get().FindPlugin(TEXT("GameFeatures")).IsValid() &&
					IPluginManager::Get().FindPlugin(TEXT("ModularGameplay")).IsValid()
						)
				{
					if(GameFeatureFounder(FeatureName))
					{
						UKismetSystemLibrary::ExecuteConsoleCommand(World,FString::Printf(TEXT("LoadGameFeaturePlugin %s"),*FeatureName));
						UKismetSystemLibrary::ExecuteConsoleCommand(World,FString::Printf(TEXT("DeactivateGameFeaturePlugin %s"),*FeatureName));
					}
				}
				else
				{
					UE_LOG(LogHotPatcher,Warning,TEXT("GameFeatures or ModularGameplay is not Enabled!"));
				}
			}
		}
		TArray<FString> FeatureNamesTemp = GetConfigSettings()->FeatureNames;
		for(const auto& FeatureName:FeatureNamesTemp)
		{
			if(!IPluginManager::Get().FindPlugin(FeatureName))
			{
				GetConfigSettings()->FeatureNames.Remove(FeatureName);
				UE_LOG(LogHotPatcher,Error,TEXT("%s load faild, %s"),*FeatureName);
			}
		}
		FeaturePackager();
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

void SGameFeaturePackageWidget::FeaturePackager()
{
	if(!GetConfigSettings()->IsStandaloneMode())
	{
		UGameFeatureProxy* GameFeatureProxy = NewObject<UGameFeatureProxy>();
		GameFeatureProxy->AddToRoot();
		GameFeatureProxy->Init(GetConfigSettings());
		GameFeatureProxy->DoExport();
	}
	else
	{
		FString CurrentConfig;
		THotPatcherTemplateHelper::TSerializeStructAsJsonString(*GetConfigSettings(),CurrentConfig);
		FString SaveConfigTo = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("HotPatcher"),FString::Printf(TEXT("GameFeatureConfig.json"))));
		FFileHelper::SaveStringToFile(CurrentConfig,*SaveConfigTo);
		FString MissionCommand = FString::Printf(TEXT("\"%s\" -run=HotPlugin -config=\"%s\" %s"),*UFlibPatchParserHelper::GetProjectFilePath(),*SaveConfigTo,*GetConfigSettings()->GetCombinedAdditionalCommandletArgs());
		UE_LOG(LogHotPatcher,Log,TEXT("HotPatcher %s Mission: %s %s"),*GetMissionName(),*UFlibHotPatcherCoreHelper::GetUECmdBinary(),*MissionCommand);
		FHotPatcherEditorModule::Get().RunProcMission(UFlibHotPatcherCoreHelper::GetUECmdBinary(),MissionCommand,GetMissionName());
	}
}

FText SGameFeaturePackageWidget::GetGenerateTooltipText() const
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
	AllStatus.Emplace(HasValidConfig(),TEXT("HasValidGameFeatureConfig"));
	bool bHasSavePath = GameFeaturePackagerSettings->GetSaveAbsPath().IsEmpty()?false:FPaths::DirectoryExists(GameFeaturePackagerSettings->GetSaveAbsPath());
	AllStatus.Emplace(bHasSavePath,TEXT("HasSavePath"));
	
	for(const auto& Status:AllStatus)
	{
		FinalString+=FString::Printf(TEXT("%s\n"),*Status.GetDisplay());
	}
	return UKismetTextLibrary::Conv_StringToText(FinalString);
}

// #include "DetailsCustomization/CustomGameFeatursDetails.h"

void SGameFeaturePackageWidget::CreateExportFilterListView()
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
	// SettingsView->GetDetailsView()->RegisterInstancedCustomPropertyTypeLayout(FGameFeaturePackagerSettings::StaticStruct()->GetFName(),FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FCustomGameFeaturePackagerSettingsDetails::MakeInstance));
	SettingsView->SetStructureData(MakeShareable(Struct));
}

bool SGameFeaturePackageWidget::CanGameFeaturePackager() const
{
	bool bHasSavePath = GameFeaturePackagerSettings->GetSaveAbsPath().IsEmpty()?false:FPaths::DirectoryExists(GameFeaturePackagerSettings->GetSaveAbsPath());
	return HasValidConfig() && bHasSavePath;
}

bool SGameFeaturePackageWidget::HasValidConfig() const
{
	bool bHasTarget = !!GetConfigSettings()->TargetPlatforms.Num();
	return bHasTarget;
}

FReply SGameFeaturePackageWidget::DoGameFeaturePackager()
{
	DoGenerate();
	return FReply::Handled();
}



#undef LOCTEXT_NAMESPACE
