// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

// #include "HotPatcherPrivatePCH.h"
#include "CreatePatch/SHotPatcherPatchWidget.h"
#include "CreatePatch/FExportPatchSettings.h"
#include "CreatePatch/PatcherProxy.h"
#include "CreatePatch/ScopedSlowTaskContext.h"

#include "FlibHotPatcherCoreHelper.h"
#include "FlibPatchParserHelper.h"
#include "FHotPatcherVersion.h"
#include "FlibAssetManageHelper.h"
#include "FPakFileInfo.h"
#include "ThreadUtils/FThreadUtils.hpp"
#include "HotPatcherLog.h"
#include "HotPatcherEditor.h"

// engine header
#include "FlibHotPatcherEditorHelper.h"
#include "Misc/FileHelper.h"
#include "Widgets/Input/SHyperlink.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Text/SMultiLineEditableText.h"
#include "Kismet/KismetStringLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/SecureHash.h"
#include "HAL/FileManager.h"
#include "PakFileUtilities.h"
#include "Kismet/KismetTextLibrary.h"
#include "Misc/EngineVersionComparison.h"

#if !UE_VERSION_OLDER_THAN(5,1,0)
	typedef FAppStyle FEditorStyle;
#endif

#define LOCTEXT_NAMESPACE "SHotPatcherCreatePatch"

void SHotPatcherPatchWidget::Construct(const FArguments& InArgs, TSharedPtr<FHotPatcherContextBase> InCreatePatchModel)
{
	ExportPatchSetting = MakeShareable(new FExportPatchSettings);
	GPatchSettings = ExportPatchSetting.Get();
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
			.HAlign(HAlign_Right)
			.Padding(4, 4, 10, 4)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Right)
				.AutoWidth()
				.Padding(0, 0, 4, 0)
				[
					SNew(SButton)
					.Text(LOCTEXT("AddToPreset", "AddToPreset"))
					.OnClicked(this, &SHotPatcherPatchWidget::DoAddToPreset)
				]
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Right)
				.AutoWidth()
				.Padding(0, 0, 4, 0)
				[
					SNew(SButton)
					.Text(LOCTEXT("PreviewChunk", "PreviewChunk"))
					.IsEnabled(this, &SHotPatcherPatchWidget::CanPreviewChunk)
					.OnClicked(this, &SHotPatcherPatchWidget::DoPreviewChunk)
					.Visibility(this, &SHotPatcherPatchWidget::VisibilityPreviewChunkButtons)
				]
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Right)
				.AutoWidth()
				.Padding(0, 0, 4, 0)
				[
					SNew(SButton)
					.Text(LOCTEXT("Diff", "Diff"))
					.IsEnabled(this, &SHotPatcherPatchWidget::CanDiff)
					.OnClicked(this, &SHotPatcherPatchWidget::DoDiff)
					.Visibility(this, &SHotPatcherPatchWidget::VisibilityDiffButtons)
				]
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Right)
				.AutoWidth()
				.Padding(0, 0, 4, 0)
				[
					SNew(SButton)
					.Text(LOCTEXT("ClearDiff", "ClearDiff"))
					.IsEnabled(this, &SHotPatcherPatchWidget::CanDiff)
					.OnClicked(this, &SHotPatcherPatchWidget::DoClearDiff)
					.Visibility(this, &SHotPatcherPatchWidget::VisibilityDiffButtons)
				]
				+ SHorizontalBox::Slot()
					.HAlign(HAlign_Right)
					.AutoWidth()
					.Padding(0, 0, 4, 0)
					[
						SNew(SButton)
						.Text(LOCTEXT("PreviewPatch", "PreviewPatch"))
						.IsEnabled(this, &SHotPatcherPatchWidget::CanPreviewPatch)
						.OnClicked(this, &SHotPatcherPatchWidget::DoPreviewPatch)
						.ToolTipText(this,&SHotPatcherPatchWidget::GetGenerateTooltipText)
					]
				+ SHorizontalBox::Slot()
					.HAlign(HAlign_Right)
					.AutoWidth()
					.Padding(0, 0, 4, 0)
					[
						SNew(SButton)
						.Text(LOCTEXT("GeneratePatch", "GeneratePatch"))
						.ToolTipText(this,&SHotPatcherPatchWidget::GetGenerateTooltipText)
						.IsEnabled(this, &SHotPatcherPatchWidget::CanExportPatch)
						.OnClicked(this, &SHotPatcherPatchWidget::DoExportPatch)
					]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			.Padding(4, 4, 10, 4)
			[
				SAssignNew(DiffWidget, SHotPatcherInformations)
				.Visibility(EVisibility::Collapsed)
			]

		];
}

void SHotPatcherPatchWidget::ImportConfig()
{
	UE_LOG(LogHotPatcher, Log, TEXT("Patch Import Config"));
	TArray<FString> Files = this->OpenFileDialog();
	if (!Files.Num()) return;

	FString LoadFile = Files[0];

	FString JsonContent;
	if (UFlibAssetManageHelper::LoadFileToString(LoadFile, JsonContent))
	{
		THotPatcherTemplateHelper::TDeserializeJsonStringAsStruct(JsonContent,*ExportPatchSetting);
		// adaptor old version config
		UFlibHotPatcherCoreHelper::AdaptorOldVersionConfig(ExportPatchSetting->GetAssetScanConfigRef(),JsonContent);
		SettingsView->GetDetailsView()->ForceRefresh();
	}
}

void SHotPatcherPatchWidget::ExportConfig()const
{
	UE_LOG(LogHotPatcher, Log, TEXT("Patch Export Config"));
	TArray<FString> Files = this->SaveFileDialog();

	if (!Files.Num()) return;

	FString SaveToFile = Files[0].EndsWith(TEXT(".json")) ? Files[0] : Files[0].Append(TEXT(".json"));

	if (ExportPatchSetting)
	{
		if (ExportPatchSetting->IsSaveConfig())
		{
			FString SerializedJsonStr;
			THotPatcherTemplateHelper::TSerializeStructAsJsonString(*ExportPatchSetting,SerializedJsonStr);
			if (FFileHelper::SaveStringToFile(SerializedJsonStr, *SaveToFile))
			{
				FText Msg = LOCTEXT("SavedPatchConfigMas", "Successd to Export the Patch Config.");
				UFlibHotPatcherEditorHelper::CreateSaveFileNotify(Msg, SaveToFile);
			}
		}
	}
}

void SHotPatcherPatchWidget::ResetConfig()
{
	UE_LOG(LogHotPatcher, Log, TEXT("Patch Clear Config"));
	FString DefaultSettingJson;
	THotPatcherTemplateHelper::TSerializeStructAsJsonString(*FExportPatchSettings::Get(),DefaultSettingJson);
	THotPatcherTemplateHelper::TDeserializeJsonStringAsStruct(DefaultSettingJson,*ExportPatchSetting);
	SettingsView->GetDetailsView()->ForceRefresh();

}
void SHotPatcherPatchWidget::DoGenerate()
{
	DoExportPatch();
}



bool SHotPatcherPatchWidget::InformationContentIsVisibility() const
{
	return DiffWidget->GetVisibility() == EVisibility::Visible;
}

void SHotPatcherPatchWidget::SetInformationContent(const FString& InContent)const
{
	DiffWidget->SetContent(InContent);
}

void SHotPatcherPatchWidget::SetInfomationContentVisibility(EVisibility InVisibility)const
{
	DiffWidget->SetVisibility(InVisibility);
}

void SHotPatcherPatchWidget::ImportProjectConfig()
{
	SHotPatcherWidgetBase::ImportProjectConfig();
	bool bUseIoStore = false;
	bool bAllowBulkDataInIoStore = false;
	
	GConfig->GetBool(TEXT("/Script/UnrealEd.ProjectPackagingSettings"),TEXT("bUseIoStore"),bUseIoStore,GGameIni);
	GConfig->GetBool(TEXT("Core.System"),TEXT("AllowBulkDataInIoStore"),bAllowBulkDataInIoStore,GEngineIni);
	
	GetConfigSettings()->IoStoreSettings.bIoStore = bUseIoStore;
	GetConfigSettings()->IoStoreSettings.bAllowBulkDataInIoStore = bAllowBulkDataInIoStore;

#if ENGINE_MAJOR_VERSION > 4
	bool bMakeBinaryConfig = false;
	GConfig->GetBool(TEXT("/Script/UnrealEd.ProjectPackagingSettings"),TEXT("bMakeBinaryConfig"),bMakeBinaryConfig,GEngineIni);
	GetConfigSettings()->bMakeBinaryConfig = bMakeBinaryConfig;
#endif
	
	FString PakFileCompressionFormats;
	GConfig->GetString(TEXT("/Script/UnrealEd.ProjectPackagingSettings"),TEXT("PakFileCompressionFormats"),PakFileCompressionFormats,GGameIni);
	if(!PakFileCompressionFormats.IsEmpty())
	{
		PakFileCompressionFormats = FString::Printf(TEXT("-compressionformats=%s"),*PakFileCompressionFormats);
		GetConfigSettings()->DefaultCommandletOptions.AddUnique(PakFileCompressionFormats);
	}
	FString PakFileAdditionalCompressionOptions;
	GConfig->GetString(TEXT("/Script/UnrealEd.ProjectPackagingSettings"),TEXT("PakFileAdditionalCompressionOptions"),PakFileAdditionalCompressionOptions,GGameIni);

	if(!PakFileAdditionalCompressionOptions.IsEmpty())
		GetConfigSettings()->DefaultCommandletOptions.AddUnique(PakFileAdditionalCompressionOptions);
	
}

void SHotPatcherPatchWidget::ShowMsg(const FString& InMsg)const
{
	auto ErrorMsgShowLambda = [this](const FString& InErrorMsg)->bool
	{
		bool bHasError = false;
		if (!InErrorMsg.IsEmpty())
		{
			this->SetInformationContent(InErrorMsg);
			this->SetInfomationContentVisibility(EVisibility::Visible);
			bHasError = true;
		}
		else
		{
			if (this->InformationContentIsVisibility())
			{
				this->SetInformationContent(TEXT(""));
				this->SetInfomationContentVisibility(EVisibility::Collapsed);
			}
		}
		return bHasError;
	};

	ErrorMsgShowLambda(InMsg);
}

bool SHotPatcherPatchWidget::CanDiff()const
{
	bool bCanDiff = false;
	if (ExportPatchSetting)
	{
		bool bHasBase = !ExportPatchSetting->GetBaseVersion().IsEmpty() && FPaths::FileExists(ExportPatchSetting->GetBaseVersion());
		bool bHasVersionId = !ExportPatchSetting->GetVersionId().IsEmpty();
		bool bHasFilter = !!ExportPatchSetting->GetAssetIncludeFilters().Num();
		bool bHasSpecifyAssets = !!ExportPatchSetting->GetIncludeSpecifyAssets().Num();

		bCanDiff = bHasBase && bHasVersionId && (bHasFilter || bHasSpecifyAssets);
	}
	return bCanDiff;
}
FReply SHotPatcherPatchWidget::DoDiff()const
{
	FString BaseVersionContent;
	FHotPatcherVersion BaseVersion;

	bool bDeserializeStatus = false;

	if (ExportPatchSetting->IsByBaseVersion())
	{
		if (UFlibAssetManageHelper::LoadFileToString(ExportPatchSetting->GetBaseVersion(), BaseVersionContent))
		{
			bDeserializeStatus = THotPatcherTemplateHelper::TDeserializeJsonStringAsStruct(BaseVersionContent, BaseVersion);
		}
		if (!bDeserializeStatus)
		{
			UE_LOG(LogHotPatcher, Error, TEXT("Deserialize Base Version Faild!"));
			return FReply::Handled();
		}
	}
	ExportPatchSetting->Init();
	FHotPatcherVersion CurrentVersion;

	// UFlibPatchParserHelper::ExportReleaseVersionInfo(
	// 	ExportPatchSetting->GetVersionId(),
	// 	BaseVersion.VersionId,
	// 	FDateTime::UtcNow().ToString(),
	// 	UFlibAssetManageHelper::DirectoryPathsToStrings(ExportPatchSetting->GetAssetIncludeFilters()),
	// 		UFlibAssetManageHelper::DirectoryPathsToStrings(ExportPatchSetting->GetAssetIgnoreFilters()),
	// 	ExportPatchSetting->GetAllSkipContents(),
	// 	ExportPatchSetting->GetForceSkipClasses(),
	// 	ExportPatchSetting->GetAssetRegistryDependencyTypes(),
	// 	ExportPatchSetting->GetIncludeSpecifyAssets(),
	// 	ExportPatchSetting->GetAddExternAssetsToPlatform(),
	// 	ExportPatchSetting->IsIncludeHasRefAssetsOnly()
	// );
	CurrentVersion.VersionId = ExportPatchSetting->GetVersionId();
	CurrentVersion.BaseVersionId = BaseVersion.VersionId;
	CurrentVersion.Date = FDateTime::UtcNow().ToString();
	UFlibPatchParserHelper::RunAssetScanner(ExportPatchSetting->GetAssetScanConfig(),CurrentVersion);
	UFlibPatchParserHelper::ExportExternAssetsToPlatform(ExportPatchSetting->GetAddExternAssetsToPlatform(),CurrentVersion,true,ExportPatchSetting->GetHashCalculator());
	
	FPatchVersionDiff VersionDiffInfo = UFlibHotPatcherCoreHelper::DiffPatchVersionWithPatchSetting(*ExportPatchSetting, BaseVersion, CurrentVersion);
	
	bool bShowDeleteAsset = false;
	FString SerializeDiffInfo;
	THotPatcherTemplateHelper::TSerializeStructAsJsonString(VersionDiffInfo,SerializeDiffInfo);
	SetInformationContent(SerializeDiffInfo);
	SetInfomationContentVisibility(EVisibility::Visible);

	return FReply::Handled();
}

FReply SHotPatcherPatchWidget::DoClearDiff()const
{
	SetInformationContent(TEXT(""));
	SetInfomationContentVisibility(EVisibility::Collapsed);

	return FReply::Handled();
}

EVisibility SHotPatcherPatchWidget::VisibilityDiffButtons() const
{
	bool bHasBase = false;
	if (ExportPatchSetting && ExportPatchSetting->IsByBaseVersion())
	{
		FString BaseVersionFile = ExportPatchSetting->GetBaseVersion();
		bHasBase = !BaseVersionFile.IsEmpty() && FPaths::FileExists(BaseVersionFile);
	}

	if (bHasBase && CanExportPatch())
	{
		return EVisibility::Visible;
	}
	else {
		return EVisibility::Collapsed;
	}
}


FReply SHotPatcherPatchWidget::DoPreviewChunk() const
{

	FHotPatcherVersion BaseVersion;

	if (ExportPatchSetting->IsByBaseVersion() && !ExportPatchSetting->GetBaseVersionInfo(BaseVersion))
	{
		UE_LOG(LogHotPatcher, Error, TEXT("Deserialize Base Version Faild!"));
		return FReply::Handled();
	}
	else
	{
		// 在不进行外部文件diff的情况下清理掉基础版本的外部文件
		if (!ExportPatchSetting->IsEnableExternFilesDiff())
		{
			BaseVersion.PlatformAssets.Empty();
		}
	}
	ExportPatchSetting->Init();
	UFlibAssetManageHelper::UpdateAssetMangerDatabase(true);
	FChunkInfo NewVersionChunk = UFlibHotPatcherCoreHelper::MakeChunkFromPatchSettings(ExportPatchSetting.Get());

	FHotPatcherVersion CurrentVersion = UFlibPatchParserHelper::ExportReleaseVersionInfoByChunk(
		ExportPatchSetting->GetVersionId(),
		BaseVersion.VersionId,
		FDateTime::UtcNow().ToString(),
		NewVersionChunk,
		ExportPatchSetting->IsIncludeHasRefAssetsOnly(),
		ExportPatchSetting->AssetScanConfig.bAnalysisFilterDependencies,
		ExportPatchSetting->GetHashCalculator()
	);

	FString CurrentVersionSavePath = ExportPatchSetting->GetCurrentVersionSavePath();
	FPatchVersionDiff VersionDiffInfo = UFlibHotPatcherCoreHelper::DiffPatchVersionWithPatchSetting(*ExportPatchSetting, BaseVersion, CurrentVersion);

	TArray<FChunkInfo> PatchChunks = ExportPatchSetting->GetChunkInfos();
	
	// create default chunk
	if(ExportPatchSetting->IsCreateDefaultChunk())
	{
		FChunkInfo TotalChunk = UFlibPatchParserHelper::CombineChunkInfos(ExportPatchSetting->GetChunkInfos());

		FChunkAssetDescribe ChunkDiffInfo = UFlibHotPatcherCoreHelper::DiffChunkWithPatchSetting(
			*ExportPatchSetting,
			NewVersionChunk,
			TotalChunk
		);
		if(ChunkDiffInfo.HasValidAssets())
		{
			PatchChunks.Add(ChunkDiffInfo.AsChunkInfo(TEXT("Default")));
		}
	}
	
	FString ShowMsg;
	for (const auto& Chunk : PatchChunks)
	{	
		FChunkAssetDescribe ChunkAssetsDescrible = UFlibPatchParserHelper::CollectFChunkAssetsDescribeByChunk(ExportPatchSetting.Get(), VersionDiffInfo,Chunk, ExportPatchSetting->GetPakTargetPlatforms());
		ShowMsg.Append(FString::Printf(TEXT("Chunk:%s\n"), *Chunk.ChunkName));
		auto AppendFilesToMsg = [&ShowMsg](const FString& CategoryName, const TArray<FName>& InFiles)
		{
			if (!!InFiles.Num())
			{
				ShowMsg.Append(FString::Printf(TEXT("%s:\n"), *CategoryName));
				for (const auto& File : InFiles)
				{
					ShowMsg.Append(FString::Printf(TEXT("\t%s\n"), *File.ToString()));
				}
			}
		};
		AppendFilesToMsg(TEXT("UE Assets"), ChunkAssetsDescrible.GetAssetsStrings());
		
		for(auto Platform:ExportPatchSetting->GetPakTargetPlatforms())
		{
			TArray<FName> PlatformExFiles;
			FString PlatformName = THotPatcherTemplateHelper::GetEnumNameByValue(Platform,false);
			PlatformExFiles.Append(ChunkAssetsDescrible.GetExternalFileNames(Platform));
			AppendFilesToMsg(PlatformName, PlatformExFiles);
		}
		AppendFilesToMsg(TEXT("Internal Files"), ChunkAssetsDescrible.GetInternalFileNames());
		ShowMsg.Append(TEXT("\n"));
	}
	
	
	if (!ShowMsg.IsEmpty())
	{
		this->ShowMsg(ShowMsg);
	}
	return FReply::Handled();
}

bool SHotPatcherPatchWidget::CanPreviewChunk() const
{
	return ExportPatchSetting->IsEnableChunk();
}

EVisibility SHotPatcherPatchWidget::VisibilityPreviewChunkButtons() const
{
	if (CanPreviewChunk())
	{
		return EVisibility::Visible;
	}
	else {
		return EVisibility::Collapsed;
	}
}
bool SHotPatcherPatchWidget::CanExportPatch()const
{
	return UFlibPatchParserHelper::IsValidPatchSettings(ExportPatchSetting.Get(),GetDefault<UHotPatcherSettings>()->bExternalFilesCheck);
}

FReply SHotPatcherPatchWidget::DoExportPatch()
{
	TSharedPtr<FExportPatchSettings> PatchSettings = MakeShareable(new FExportPatchSettings);
	*PatchSettings = *GetConfigSettings();
	FHotPatcherEditorModule::Get().CookAndPakByPatchSettings(PatchSettings,PatchSettings->IsStandaloneMode());

	return FReply::Handled();
}

FText SHotPatcherPatchWidget::GetGenerateTooltipText() const
{
	FString FinalString;
	if (GetMutableDefault<UHotPatcherSettings>()->bPreviewTooltips && ExportPatchSetting)
	{
		bool bHasBase = false;
		if (ExportPatchSetting->IsByBaseVersion())
			bHasBase = !ExportPatchSetting->GetBaseVersion().IsEmpty() && FPaths::FileExists(ExportPatchSetting->GetBaseVersion());
		else
			bHasBase = true;
		bool bHasVersionId = !ExportPatchSetting->GetVersionId().IsEmpty();
		bool bHasFilter = !!ExportPatchSetting->GetAssetIncludeFilters().Num();
		bool bHasSpecifyAssets = !!ExportPatchSetting->GetIncludeSpecifyAssets().Num();
		// bool bHasExternFiles = !!ExportPatchSetting->GetAddExternFiles().Num();
		// bool bHasExDirs = !!ExportPatchSetting->GetAddExternDirectory().Num();
		
		bool bHasExternFiles = true;
		if(GetDefault<UHotPatcherSettings>()->bExternalFilesCheck)
		{
			bHasExternFiles = !!ExportPatchSetting->GetAllPlatfotmExternFiles().Num();
		}
		bool bHasExDirs = !!ExportPatchSetting->GetAddExternAssetsToPlatform().Num();
		bool bHasSavePath = !ExportPatchSetting->GetSaveAbsPath().IsEmpty();
		bool bHasPakPlatfotm = !!ExportPatchSetting->GetPakTargetPlatforms().Num();
		
		bool bHasAnyPakFiles = (
			bHasFilter || bHasSpecifyAssets || bHasExternFiles || bHasExDirs ||
			ExportPatchSetting->IsIncludeEngineIni() ||
			ExportPatchSetting->IsIncludePluginIni() ||
			ExportPatchSetting->IsIncludeProjectIni()
			);
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
		AllStatus.Emplace(bHasBase,TEXT("BaseVersion"));
		AllStatus.Emplace(bHasVersionId,TEXT("HasVersionId"));
		AllStatus.Emplace(bHasAnyPakFiles,TEXT("HasAnyPakFiles"));
		AllStatus.Emplace(bHasPakPlatfotm,TEXT("HasPakPlatfotm"));
		AllStatus.Emplace(bHasSavePath,TEXT("HasSavePath"));
		
		for(const auto& Status:AllStatus)
		{
			FinalString+=FString::Printf(TEXT("%s\n"),*Status.GetDisplay());
		}
	}
	return UKismetTextLibrary::Conv_StringToText(FinalString);
}

bool SHotPatcherPatchWidget::CanPreviewPatch() const
{
	bool bHasFilter = !!ExportPatchSetting->GetAssetIncludeFilters().Num();
	bool bHasSpecifyAssets = !!ExportPatchSetting->GetIncludeSpecifyAssets().Num();
	
	auto HasExFilesLambda = [this]()
	{
		bool result = false;
		const TMap<ETargetPlatform,FPlatformExternFiles>& ExFiles = ExportPatchSetting->GetAllPlatfotmExternFiles(false);
		if(!!ExFiles.Num())
		{
			TArray<ETargetPlatform> Platforms;
			ExFiles.GetKeys(Platforms);
			for(const auto& Platform:Platforms)
			{
				if(!!ExFiles.Find(Platform)->ExternFiles.Num())
				{
					result=true;
					break;
				}
			}
		}
		return result;
	};
	bool bHasExternFiles = true;
	if(GetDefault<UHotPatcherSettings>()->bExternalFilesCheck)
	{
		bHasExternFiles = HasExFilesLambda();		
	}
	
	bool bHasAnyPakFiles = (
		bHasFilter || bHasSpecifyAssets || bHasExternFiles ||
		ExportPatchSetting->IsIncludeEngineIni() ||
		ExportPatchSetting->IsIncludePluginIni() ||
		ExportPatchSetting->IsIncludeProjectIni()
		);

	return bHasFilter || bHasSpecifyAssets || bHasExternFiles || bHasAnyPakFiles;
}


FReply SHotPatcherPatchWidget::DoPreviewPatch()
{
	ExportPatchSetting->Init();
	FChunkInfo DefaultChunk;
	FHotPatcherVersion BaseVersion;

	if (ExportPatchSetting->IsByBaseVersion())
	{
		ExportPatchSetting->GetBaseVersionInfo(BaseVersion);
		DefaultChunk = UFlibHotPatcherCoreHelper::MakeChunkFromPatchVerison(BaseVersion);
		if (!ExportPatchSetting->IsEnableExternFilesDiff())
		{
			BaseVersion.PlatformAssets.Empty();
		}
	}

	FChunkInfo NewVersionChunk = UFlibHotPatcherCoreHelper::MakeChunkFromPatchSettings(ExportPatchSetting.Get());
	
	FChunkAssetDescribe ChunkAssetsDescrible = UFlibHotPatcherCoreHelper::DiffChunkByBaseVersionWithPatchSetting(*ExportPatchSetting.Get(),NewVersionChunk, DefaultChunk, BaseVersion);

	TArray<FName> AllUnselectedAssets = ChunkAssetsDescrible.GetAssetsStrings();
	TArray<FName> UnSelectedInternalFiles = ChunkAssetsDescrible.GetInternalFileNames();

	FString TotalMsg;
	auto ChunkCheckerMsg = [&TotalMsg](const FString& Category, const TArray<FName>& InAssetList)
	{
		if (!!InAssetList.Num())
		{
			TotalMsg.Append(FString::Printf(TEXT("\n%s:\n"), *Category));
			for (const auto& Asset : InAssetList)
			{
				TotalMsg.Append(FString::Printf(TEXT("\t%s\n"), *Asset.ToString()));
			}
		}
	};
	ChunkCheckerMsg(TEXT("Unreal Asset"), AllUnselectedAssets);
	ChunkCheckerMsg(TEXT("External Files"), TArray<FName>{});
	for(auto Platform:ExportPatchSetting->GetPakTargetPlatforms())
	{
		TArray<FName> PlatformExFiles;
		FString PlatformName = THotPatcherTemplateHelper::GetEnumNameByValue(Platform,false);
		PlatformExFiles.Append(ChunkAssetsDescrible.GetExternalFileNames(Platform));
		PlatformExFiles.Append(ChunkAssetsDescrible.GetExternalFileNames(ETargetPlatform::AllPlatforms));
		ChunkCheckerMsg(PlatformName, PlatformExFiles);
	}
	
	ChunkCheckerMsg(TEXT("Internal Files"), UnSelectedInternalFiles);

	if (!TotalMsg.IsEmpty())
	{
		ShowMsg(FString::Printf(TEXT("Patch Assets:\n%s"), *TotalMsg));
		return FReply::Handled();
	}

	return FReply::Handled();
}

FReply SHotPatcherPatchWidget::DoAddToPreset() const
{
	UHotPatcherSettings* Settings = GetMutableDefault<UHotPatcherSettings>();
	Settings->PresetConfigs.Add(*const_cast<SHotPatcherPatchWidget*>(this)->GetConfigSettings());
	Settings->SaveConfig();
	return FReply::Handled();
}

void SHotPatcherPatchWidget::CreateExportFilterListView()
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
	}

	FStructureDetailsViewArgs StructureViewArgs;
	{
		StructureViewArgs.bShowObjects = true;
		StructureViewArgs.bShowAssets = true;
		StructureViewArgs.bShowClasses = true;
		StructureViewArgs.bShowInterfaces = true;
	}

	SettingsView = EditModule.CreateStructureDetailView(DetailsViewArgs, StructureViewArgs, nullptr);
	FStructOnScope* Struct = new FStructOnScope(FExportPatchSettings::StaticStruct(), (uint8*)ExportPatchSetting.Get());
	SettingsView->SetStructureData(MakeShareable(Struct));
}


#undef LOCTEXT_NAMESPACE
