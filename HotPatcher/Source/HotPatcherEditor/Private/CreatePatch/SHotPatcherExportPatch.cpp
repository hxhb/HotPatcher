// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

// #include "HotPatcherPrivatePCH.h"
#include "CreatePatch/SHotPatcherExportPatch.h"
#include "FlibHotPatcherEditorHelper.h"
#include "FlibPatchParserHelper.h"
#include "FHotPatcherVersion.h"
#include "FLibAssetManageHelperEx.h"
#include "FPakFileInfo.h"
#include "ThreadUtils/FThreadUtils.hpp"
#include "HotPatcherLog.h"
#include "CreatePatch/ScopedSlowTaskContext.h"

// engine header
#include "Misc/FileHelper.h"
#include "Widgets/Input/SHyperlink.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Text/SMultiLineEditableText.h"

#include "Kismet/KismetStringLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/SecureHash.h"
// #include "Misc/ScopedSlowTask.h"
#include "HAL/FileManager.h"

#include "PakFileUtilities.h"
#include "CreatePatch/PatcherProxy.h"
#include "CreatePatch/ScopedSlowTaskContext.h"

#define LOCTEXT_NAMESPACE "SHotPatcherCreatePatch"

void SHotPatcherExportPatch::Construct(const FArguments& InArgs, TSharedPtr<FHotPatcherCreatePatchModel> InCreatePatchModel)
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
					.Text(LOCTEXT("PreviewChunk", "PreviewChunk"))
					.IsEnabled(this, &SHotPatcherExportPatch::CanPreviewChunk)
					.OnClicked(this, &SHotPatcherExportPatch::DoPreviewChunk)
					.Visibility(this, &SHotPatcherExportPatch::VisibilityPreviewChunkButtons)
				]
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Right)
				.AutoWidth()
				.Padding(0, 0, 4, 0)
				[
					SNew(SButton)
					.Text(LOCTEXT("Diff", "Diff"))
					.IsEnabled(this, &SHotPatcherExportPatch::CanDiff)
					.OnClicked(this, &SHotPatcherExportPatch::DoDiff)
					.Visibility(this, &SHotPatcherExportPatch::VisibilityDiffButtons)
				]
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Right)
				.AutoWidth()
				.Padding(0, 0, 4, 0)
				[
					SNew(SButton)
					.Text(LOCTEXT("ClearDiff", "ClearDiff"))
					.IsEnabled(this, &SHotPatcherExportPatch::CanDiff)
					.OnClicked(this, &SHotPatcherExportPatch::DoClearDiff)
					.Visibility(this, &SHotPatcherExportPatch::VisibilityDiffButtons)
				]
				+ SHorizontalBox::Slot()
					.HAlign(HAlign_Right)
					.AutoWidth()
					.Padding(0, 0, 4, 0)
					[
						SNew(SButton)
						.Text(LOCTEXT("PreviewPatch", "PreviewPatch"))
						.IsEnabled(this, &SHotPatcherExportPatch::CanPreviewPatch)
						.OnClicked(this, &SHotPatcherExportPatch::DoPreviewPatch)
					]
				+ SHorizontalBox::Slot()
					.HAlign(HAlign_Right)
					.AutoWidth()
					.Padding(0, 0, 4, 0)
					[
						SNew(SButton)
						.Text(LOCTEXT("GeneratePatch", "GeneratePatch"))
						.IsEnabled(this, &SHotPatcherExportPatch::CanExportPatch)
						.OnClicked(this, &SHotPatcherExportPatch::DoExportPatch)
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

	ExportPatchSetting = UExportPatchSettings::Get();
	SettingsView->SetObject(ExportPatchSetting);

}

void SHotPatcherExportPatch::ImportConfig()
{
	UE_LOG(LogHotPatcher, Log, TEXT("Patch Import Config"));
	TArray<FString> Files = this->OpenFileDialog();
	if (!Files.Num()) return;

	FString LoadFile = Files[0];

	FString JsonContent;
	if (UFLibAssetManageHelperEx::LoadFileToString(LoadFile, JsonContent))
	{
		UFlibHotPatcherEditorHelper::DeserializePatchConfig(ExportPatchSetting,JsonContent);
		SettingsView->ForceRefresh();
	}
	
}
void SHotPatcherExportPatch::ExportConfig()const
{
	UE_LOG(LogHotPatcher, Log, TEXT("Patch Export Config"));
	TArray<FString> Files = this->SaveFileDialog();

	if (!Files.Num()) return;

	FString SaveToFile = Files[0].EndsWith(TEXT(".json")) ? Files[0] : Files[0].Append(TEXT(".json"));

	if (ExportPatchSetting)
	{
		if (ExportPatchSetting->IsSavePatchConfig())
		{
			FString SerializedJsonStr;
			ExportPatchSetting->SerializePatchConfigToString(SerializedJsonStr);

			if (FFileHelper::SaveStringToFile(SerializedJsonStr, *SaveToFile))
			{
				FText Msg = LOCTEXT("SavedPatchConfigMas", "Successd to Export the Patch Config.");
				UFlibHotPatcherEditorHelper::CreateSaveFileNotify(Msg, SaveToFile);
			}
		}
	}
}

void SHotPatcherExportPatch::ResetConfig()
{
	UE_LOG(LogHotPatcher, Log, TEXT("Patch Clear Config"));
	UExportPatchSettings* DefaultSetting = NewObject<UExportPatchSettings>();
	FString DefaultSettingJson;
	DefaultSetting->SerializePatchConfigToString(DefaultSettingJson);
	UFlibHotPatcherEditorHelper::DeserializePatchConfig(ExportPatchSetting, DefaultSettingJson);
	SettingsView->ForceRefresh();

}
void SHotPatcherExportPatch::DoGenerate()
{
	DoExportPatch();
}



bool SHotPatcherExportPatch::InformationContentIsVisibility() const
{
	return DiffWidget->GetVisibility() == EVisibility::Visible;
}

void SHotPatcherExportPatch::SetInformationContent(const FString& InContent)const
{
	DiffWidget->SetContent(InContent);
}

void SHotPatcherExportPatch::SetInfomationContentVisibility(EVisibility InVisibility)const
{
	DiffWidget->SetVisibility(InVisibility);
}


void SHotPatcherExportPatch::ShowMsg(const FString& InMsg)const
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

bool SHotPatcherExportPatch::CanDiff()const
{
	bool bCanDiff = false;
	if (ExportPatchSetting)
	{
		bool bHasBase = !ExportPatchSetting->GetBaseVersion().IsEmpty() && FPaths::FileExists(ExportPatchSetting->GetBaseVersion());
		bool bHasVersionId = !ExportPatchSetting->GetVersionId().IsEmpty();
		bool bHasFilter = !!ExportPatchSetting->GetAssetIncludeFiltersPaths().Num();
		bool bHasSpecifyAssets = !!ExportPatchSetting->GetIncludeSpecifyAssets().Num();

		bCanDiff = bHasBase && bHasVersionId && (bHasFilter || bHasSpecifyAssets);
	}
	return bCanDiff;
}
FReply SHotPatcherExportPatch::DoDiff()const
{
	FString BaseVersionContent;
	FHotPatcherVersion BaseVersion;

	bool bDeserializeStatus = false;

	if (ExportPatchSetting->IsByBaseVersion())
	{
		if (UFLibAssetManageHelperEx::LoadFileToString(ExportPatchSetting->GetBaseVersion(), BaseVersionContent))
		{
			bDeserializeStatus = UFlibPatchParserHelper::DeserializeHotPatcherVersionFromString(BaseVersionContent, BaseVersion);
		}
		if (!bDeserializeStatus)
		{
			UE_LOG(LogHotPatcher, Error, TEXT("Deserialize Base Version Faild!"));
			return FReply::Handled();
		}
	}

	FHotPatcherVersion CurrentVersion = UFlibPatchParserHelper::ExportReleaseVersionInfo(
		ExportPatchSetting->GetVersionId(),
		BaseVersion.VersionId,
		FDateTime::UtcNow().ToString(),
		ExportPatchSetting->GetAssetIncludeFiltersPaths(),
		ExportPatchSetting->GetAssetIgnoreFiltersPaths(),
		ExportPatchSetting->GetAssetRegistryDependencyTypes(),
		ExportPatchSetting->GetIncludeSpecifyAssets(),
		ExportPatchSetting->GetAllExternFiles(true),
		ExportPatchSetting->IsIncludeHasRefAssetsOnly()
	);

	FPatchVersionDiff VersionDiffInfo = UFlibPatchParserHelper::DiffPatchVersion(BaseVersion, CurrentVersion);

	bool bShowDeleteAsset = false;

	FString SerializeDiffInfo =
		FString::Printf(
			TEXT("%s\n%s\n"),
			*UFlibPatchParserHelper::SerializeDiffAssetsInfomationToString(VersionDiffInfo.AddAssetDependInfo, VersionDiffInfo.ModifyAssetDependInfo, bShowDeleteAsset ? VersionDiffInfo.DeleteAssetDependInfo : FAssetDependenciesInfo{}),
			ExportPatchSetting->IsEnableExternFilesDiff() ?
			*UFlibPatchParserHelper::SerializeDiffExternalFilesInfomationToString(VersionDiffInfo.AddExternalFiles, VersionDiffInfo.ModifyExternalFiles, bShowDeleteAsset ? VersionDiffInfo.DeleteExternalFiles : TArray<FExternAssetFileInfo>{}) :
			*UFlibPatchParserHelper::SerializeDiffExternalFilesInfomationToString(ExportPatchSetting->GetAllExternFiles(), TArray<FExternAssetFileInfo>{}, TArray<FExternAssetFileInfo>{})
		);
	SetInformationContent(SerializeDiffInfo);
	SetInfomationContentVisibility(EVisibility::Visible);

	return FReply::Handled();
}

FReply SHotPatcherExportPatch::DoClearDiff()const
{
	SetInformationContent(TEXT(""));
	SetInfomationContentVisibility(EVisibility::Collapsed);

	return FReply::Handled();
}

EVisibility SHotPatcherExportPatch::VisibilityDiffButtons() const
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


FReply SHotPatcherExportPatch::DoPreviewChunk() const
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
			BaseVersion.ExternalFiles.Empty();
		}
	}

	UFLibAssetManageHelperEx::UpdateAssetMangerDatabase(true);
	FChunkInfo NewVersionChunk = UFlibHotPatcherEditorHelper::MakeChunkFromPatchSettings(ExportPatchSetting);

	FHotPatcherVersion CurrentVersion = UFlibPatchParserHelper::ExportReleaseVersionInfoByChunk(
		ExportPatchSetting->GetVersionId(),
		BaseVersion.VersionId,
		FDateTime::UtcNow().ToString(),
		NewVersionChunk,
		ExportPatchSetting->IsIncludeHasRefAssetsOnly()
	);

	FString CurrentVersionSavePath = ExportPatchSetting->GetCurrentVersionSavePath();
	FPatchVersionDiff VersionDiffInfo = UFlibPatchParserHelper::DiffPatchVersion(BaseVersion, CurrentVersion);

	FString ShowMsg;
	for (const auto& Chunk : ExportPatchSetting->GetChunkInfos())
	{
		FChunkAssetDescribe ChunkAssetsDescrible = UFlibPatchParserHelper::CollectFChunkAssetsDescribeByChunk(VersionDiffInfo, Chunk);
		ShowMsg.Append(FString::Printf(TEXT("Chunk:%s\n"), *Chunk.ChunkName));
		auto AppendFilesToMsg = [&ShowMsg](const FString& CategoryName, const TArray<FString>& InFiles)
		{
			if (!!InFiles.Num())
			{
				ShowMsg.Append(FString::Printf(TEXT("%s:\n"), *CategoryName));
				for (const auto& File : InFiles)
				{
					ShowMsg.Append(FString::Printf(TEXT("\t%s\n"), *File));
				}
			}
		};
		AppendFilesToMsg(TEXT("UE Assets"), ChunkAssetsDescrible.GetAssetsStrings());
		AppendFilesToMsg(TEXT("External Files"), ChunkAssetsDescrible.GetExFileStrings());
		AppendFilesToMsg(TEXT("Internal Files"), ChunkAssetsDescrible.GetInternalFileStrings());
		ShowMsg.Append(TEXT("\n"));
	}
	if (!ShowMsg.IsEmpty())
	{
		this->ShowMsg(ShowMsg);
	}
	return FReply::Handled();
}

bool SHotPatcherExportPatch::CanPreviewChunk() const
{
	return ExportPatchSetting->IsEnableChunk();
}

EVisibility SHotPatcherExportPatch::VisibilityPreviewChunkButtons() const
{
	if (CanPreviewChunk())
	{
		return EVisibility::Visible;
	}
	else {
		return EVisibility::Collapsed;
	}
}
bool SHotPatcherExportPatch::CanExportPatch()const
{
	bool bCanExport = false;
	if (ExportPatchSetting)
	{
		bool bHasBase = false;
		if (ExportPatchSetting->IsByBaseVersion())
			bHasBase = !ExportPatchSetting->GetBaseVersion().IsEmpty() && FPaths::FileExists(ExportPatchSetting->GetBaseVersion());
		else
			bHasBase = true;
		bool bHasVersionId = !ExportPatchSetting->GetVersionId().IsEmpty();
		bool bHasFilter = !!ExportPatchSetting->GetAssetIncludeFiltersPaths().Num();
		bool bHasSpecifyAssets = !!ExportPatchSetting->GetIncludeSpecifyAssets().Num();
		bool bHasExternFiles = !!ExportPatchSetting->GetAddExternFiles().Num();
		bool bHasExDirs = !!ExportPatchSetting->GetAddExternDirectory().Num();
		bool bHasSavePath = !ExportPatchSetting->GetSaveAbsPath().IsEmpty();
		bool bHasPakPlatfotm = !!ExportPatchSetting->GetPakTargetPlatforms().Num();

		bool bHasAnyPakFiles = (
			bHasFilter || bHasSpecifyAssets || bHasExternFiles || bHasExDirs ||
			ExportPatchSetting->IsIncludeEngineIni() ||
			ExportPatchSetting->IsIncludePluginIni() ||
			ExportPatchSetting->IsIncludeProjectIni()
			);
		bCanExport = bHasBase && bHasVersionId && bHasAnyPakFiles && bHasPakPlatfotm && bHasSavePath;
	}
	return bCanExport;
}

FReply SHotPatcherExportPatch::DoExportPatch()
{
	UPatcherProxy* PatcherProxy = NewObject<UPatcherProxy>();
	PatcherProxy->SetProxySettings(ExportPatchSetting);
	PatcherProxy->OnShowMsg.AddRaw(this,&SHotPatcherExportPatch::ShowMsg);
	PatcherProxy->DoExport();

	return FReply::Handled();
}

bool SHotPatcherExportPatch::CanPreviewPatch() const
{
	bool bHasFilter = !!ExportPatchSetting->GetAssetIncludeFiltersPaths().Num();
	bool bHasSpecifyAssets = !!ExportPatchSetting->GetIncludeSpecifyAssets().Num();
	bool bHasExternFiles = !!ExportPatchSetting->GetAddExternFiles().Num();
	bool bHasExDirs = !!ExportPatchSetting->GetAddExternDirectory().Num();
	bool bHasAnyPakFiles = (
		bHasFilter || bHasSpecifyAssets || bHasExternFiles || bHasExDirs ||
		ExportPatchSetting->IsIncludeEngineIni() ||
		ExportPatchSetting->IsIncludePluginIni() ||
		ExportPatchSetting->IsIncludeProjectIni()
		);

	return bHasFilter || bHasSpecifyAssets || bHasExternFiles || bHasExDirs || bHasAnyPakFiles;
}


FReply SHotPatcherExportPatch::DoPreviewPatch()
{
	FChunkInfo DefaultChunk;
	FHotPatcherVersion BaseVersion;

	if (ExportPatchSetting->IsByBaseVersion())
	{
		ExportPatchSetting->GetBaseVersionInfo(BaseVersion);
		DefaultChunk = UFlibHotPatcherEditorHelper::MakeChunkFromPatchVerison(BaseVersion);
		if (!ExportPatchSetting->IsEnableExternFilesDiff())
		{
			BaseVersion.ExternalFiles.Empty();
			DefaultChunk.AddExternDirectoryToPak.Empty();
			DefaultChunk.AddExternFileToPak.Empty();
		}
	}

	FChunkInfo NewVersionChunk = UFlibHotPatcherEditorHelper::MakeChunkFromPatchSettings(ExportPatchSetting);
	
	FChunkAssetDescribe ChunkAssetsDescrible = UFlibPatchParserHelper::DiffChunkByBaseVersion(NewVersionChunk,DefaultChunk, BaseVersion, ExportPatchSetting->IsIncludeHasRefAssetsOnly());

	TArray<FString> AllUnselectedAssets = ChunkAssetsDescrible.GetAssetsStrings();
	TArray<FString> AllUnselectedExFiles = ChunkAssetsDescrible.GetExFileStrings();
	TArray<FString> UnSelectedInternalFiles = ChunkAssetsDescrible.GetInternalFileStrings();

	FString TotalMsg;
	auto ChunkCheckerMsg = [&TotalMsg](const FString& Category, const TArray<FString>& InAssetList)
	{
		if (!!InAssetList.Num())
		{
			TotalMsg.Append(FString::Printf(TEXT("\n%s:\n"), *Category));
			for (const auto& Asset : InAssetList)
			{
				TotalMsg.Append(FString::Printf(TEXT("\t%s\n"), *Asset));
			}
		}
	};
	ChunkCheckerMsg(TEXT("Unreal Asset"), AllUnselectedAssets);
	ChunkCheckerMsg(TEXT("External Files"), AllUnselectedExFiles);
	ChunkCheckerMsg(TEXT("Internal Files"), UnSelectedInternalFiles);

	if (!TotalMsg.IsEmpty())
	{
		ShowMsg(FString::Printf(TEXT("Patch Assets:\n%s"), *TotalMsg));
		return FReply::Handled();
	}

	return FReply::Handled();
}

void SHotPatcherExportPatch::CreateExportFilterListView()
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


#undef LOCTEXT_NAMESPACE
