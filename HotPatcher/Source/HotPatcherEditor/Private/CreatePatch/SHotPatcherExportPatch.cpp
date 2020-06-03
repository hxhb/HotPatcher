// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

// #include "HotPatcherPrivatePCH.h"
#include "CreatePatch/SHotPatcherExportPatch.h"
#include "FlibHotPatcherEditorHelper.h"
#include "FlibPatchParserHelper.h"
#include "FHotPatcherVersion.h"
#include "FLibAssetManageHelperEx.h"
#include "FPakFileInfo.h"
#include "ThreadUtils/FThreadUtils.hpp"

// engine header
#include "Misc/FileHelper.h"
#include "Widgets/Input/SHyperlink.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Text/SMultiLineEditableText.h"

#include "Kismet/KismetStringLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/SecureHash.h"
#include "Misc/ScopedSlowTask.h"
#include "HAL/FileManager.h"

#include "PakFileUtilities.h"

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
	UE_LOG(LogTemp, Log, TEXT("Patch Import Config"));
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
	UE_LOG(LogTemp, Log, TEXT("Patch Export Config"));
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
	UE_LOG(LogTemp, Log, TEXT("Patch Clear Config"));
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


bool SHotPatcherExportPatch::CheckSelectedAssetsCookStatus(const TArray<FString>& PlatformNames, const FAssetDependenciesInfo& SelectedAssets, FString& OutMsg)const
{
	OutMsg.Empty();
	// 检查所修改的资源是否被Cook过
	for (const auto& PlatformName : PlatformNames)
	{
			TArray<FAssetDetail> ValidCookAssets;
			TArray<FAssetDetail> InvalidCookAssets;

			UFlibHotPatcherEditorHelper::CheckInvalidCookFilesByAssetDependenciesInfo(UKismetSystemLibrary::GetProjectDirectory(), PlatformName, SelectedAssets, ValidCookAssets, InvalidCookAssets);

			if (InvalidCookAssets.Num() > 0)
			{
				OutMsg.Append(FString::Printf(TEXT("%s UnCooked Assets:\n"), *PlatformName));

				for (const auto& Asset : InvalidCookAssets)
				{
					FString AssetLongPackageName;
					UFLibAssetManageHelperEx::ConvPackagePathToLongPackageName(Asset.mPackagePath, AssetLongPackageName);
					OutMsg.Append(FString::Printf(TEXT("\t%s\n"), *AssetLongPackageName));
				}
			}
	}

	return OutMsg.IsEmpty();
}



bool SHotPatcherExportPatch::CheckPatchRequire(const FPatchVersionDiff& InDiff)const
{
	bool Status = false;
	// 错误处理
	{
		FString GenErrorMsg;
		FAssetDependenciesInfo AllChangedAssetInfo = UFLibAssetManageHelperEx::CombineAssetDependencies(InDiff.AddAssetDependInfo, InDiff.ModifyAssetDependInfo);
		bool bSelectedCookStatus = CheckSelectedAssetsCookStatus(ExportPatchSetting->GetPakTargetPlatformNames(), AllChangedAssetInfo, GenErrorMsg);

		// 如果有错误信息 则输出后退出
		if (!bSelectedCookStatus)
		{
			ShowMsg(GenErrorMsg);
			Status = false;
		}
		else
		{
			ShowMsg(TEXT(""));
			Status = true;
		}
	}
	return Status;
}

bool SHotPatcherExportPatch::ShowMsg(const FString& InMsg)const
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

	return ErrorMsgShowLambda(InMsg);
}

bool SHotPatcherExportPatch::SavePatchVersionJson(const FHotPatcherVersion& InSaveVersion, const FString& InSavePath, FPakVersion& OutPakVersion)
{
	bool bStatus = false;
	OutPakVersion = UExportPatchSettings::GetPakVersion(InSaveVersion, FDateTime::UtcNow().ToString());
	{
		if (ExportPatchSetting->IsIncludePakVersion())
		{
			FString SavePakVersionFilePath = UExportPatchSettings::GetSavePakVersionPath(InSavePath, InSaveVersion);

			FString OutString;
			if (UFlibPakHelper::SerializePakVersionToString(OutPakVersion, OutString))
			{
				bStatus = UFLibAssetManageHelperEx::SaveStringToFile(SavePakVersionFilePath, OutString);
			}
		}
	}
	return bStatus;
}

bool SHotPatcherExportPatch::SavePatchDiffJson(const FHotPatcherVersion& InSaveVersion, const FPatchVersionDiff& InDiff)
{
	bool bStatus = false;
	if (ExportPatchSetting->IsSaveDiffAnalysis())
	{
		FString SerializeDiffInfo =
			FString::Printf(TEXT("%s\n%s\n"),
				*UFlibPatchParserHelper::SerializeDiffAssetsInfomationToString(InDiff.AddAssetDependInfo, InDiff.ModifyAssetDependInfo, InDiff.DeleteAssetDependInfo),
				ExportPatchSetting->IsEnableExternFilesDiff() ?
				*UFlibPatchParserHelper::SerializeDiffExternalFilesInfomationToString(InDiff.AddExternalFiles, InDiff.ModifyExternalFiles, InDiff.DeleteExternalFiles) :
				*UFlibPatchParserHelper::SerializeDiffExternalFilesInfomationToString(ExportPatchSetting->GetAllExternFiles(), TArray<FExternAssetFileInfo>{}, TArray<FExternAssetFileInfo>{})
			);


		FString SaveDiffToFile = FPaths::Combine(
			ExportPatchSetting->GetCurrentVersionSavePath(),
			FString::Printf(TEXT("%s_%s_Diff.json"), *InSaveVersion.BaseVersionId, *InSaveVersion.VersionId)
		);
		if (UFLibAssetManageHelperEx::SaveStringToFile(SaveDiffToFile, SerializeDiffInfo))
		{
			bStatus = true;
			auto Msg = LOCTEXT("SavePatchDiffInfo", "Succeed to export New Patch Diff Info.");
			UFlibHotPatcherEditorHelper::CreateSaveFileNotify(Msg, SaveDiffToFile);
		}
	}
	return bStatus;
}


FProcHandle SHotPatcherExportPatch::DoUnrealPak(TArray<FString> UnrealPakOptions, bool block)
{
	FString UnrealPakBinary = UFlibPatchParserHelper::GetUnrealPakBinary();

	FString CommandLine;
	for (const auto& Option : UnrealPakOptions)
	{
		CommandLine.Append(FString::Printf(TEXT(" %s"), *Option));
	}

	// create UnrealPak process

	uint32 *ProcessID = NULL;
	FProcHandle ProcHandle = FPlatformProcess::CreateProc(*UnrealPakBinary, *CommandLine, true, false, false, ProcessID, 0, NULL, NULL, NULL);

	if (ProcHandle.IsValid())
	{
		if (block)
		{
			FPlatformProcess::WaitForProc(ProcHandle);
		}
	}
	return ProcHandle;
}


bool SHotPatcherExportPatch::SavePakCommands(const FString& InPlatformName, const FPatchVersionDiff& InDiffInfo, const FString& InSavePath)
{
	bool bStatus = false;
	// combine all pak commands
	{
		FString ProjectDir = UKismetSystemLibrary::GetProjectDirectory();

		// generated cook command form asset list
		TArray<FString> OutPakCommand = ExportPatchSetting->MakeAllPakCommandsByTheSetting(InPlatformName, InDiffInfo, ExportPatchSetting->IsEnableExternFilesDiff());

		// save paklist to file
		if (FFileHelper::SaveStringArrayToFile(OutPakCommand, *InSavePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
		{
			if (ExportPatchSetting->IsSavePakList())
			{
				auto Msg = LOCTEXT("SavePatchPakCommand", "Succeed to export the Patch Packaghe Pak Command.");
				UFlibHotPatcherEditorHelper::CreateSaveFileNotify(Msg, InSavePath);
				bStatus = true;
			}
		}
	}
	return bStatus;
}


FHotPatcherVersion SHotPatcherExportPatch::MakeNewRelease(const FHotPatcherVersion& InBaseVersion, const FHotPatcherVersion& InCurrentVersion)const
{
	FHotPatcherVersion BaseVersion = InBaseVersion;
	FHotPatcherVersion NewRelease = InCurrentVersion;
	FPatchVersionDiff DiffInfo = UFlibPatchParserHelper::DiffPatchVersion(BaseVersion,InCurrentVersion);
	
	FAssetDependenciesInfo& BaseAssetInfoRef = BaseVersion.AssetInfo;
	TMap<FString, FExternAssetFileInfo>& BaseExternalFilesRef = BaseVersion.ExternalFiles;

	// Modify Asset
	auto DeleteOldAssetLambda = [&BaseAssetInfoRef](const FAssetDependenciesInfo& InAssetDependenciesInfo)
	{
		for (const auto& AssetsModulePair : InAssetDependenciesInfo.mDependencies)
		{
			FAssetDependenciesDetail* NewReleaseModuleAssets = BaseAssetInfoRef.mDependencies.Find(AssetsModulePair.Key);

			for (const auto& NeedDeleteAsset : AssetsModulePair.Value.mDependAssetDetails)
			{
				if (NewReleaseModuleAssets->mDependAssetDetails.Contains(NeedDeleteAsset.Key))
				{
					NewReleaseModuleAssets->mDependAssetDetails.Remove(NeedDeleteAsset.Key);
				}
			}
		}
	};
	
	DeleteOldAssetLambda(DiffInfo.ModifyAssetDependInfo);
	// DeleteOldAssetLambda(DiffInfo.DeleteAssetDependInfo);

	// Add Asset
	BaseAssetInfoRef = UFLibAssetManageHelperEx::CombineAssetDependencies(BaseAssetInfoRef, DiffInfo.AddAssetDependInfo);
	// modify Asset
	BaseAssetInfoRef = UFLibAssetManageHelperEx::CombineAssetDependencies(BaseAssetInfoRef, DiffInfo.ModifyAssetDependInfo);
	NewRelease.AssetInfo = BaseAssetInfoRef;

	// external files
	auto DeleteOldExternalFilesLambda = [&BaseExternalFilesRef](const TArray<FExternAssetFileInfo>& InFiles)
	{
		for (const auto& File : InFiles)
		{
			if (BaseExternalFilesRef.Contains(File.FilePath.FilePath))
			{
				BaseExternalFilesRef.Remove(File.FilePath.FilePath);
			}
		}
	};
	DeleteOldExternalFilesLambda(DiffInfo.ModifyExternalFiles);
	// DeleteOldExternalFilesLambda(DiffInfo.DeleteExternalFiles);

	auto AddExternalFilesLambda = [&BaseExternalFilesRef](const TArray<FExternAssetFileInfo>& InFiles)
	{
		for (const auto& File : InFiles)
		{
			if (!BaseExternalFilesRef.Contains(File.FilePath.FilePath))
			{
				BaseExternalFilesRef.Add(File.FilePath.FilePath,File);
			}
		}
	};
	AddExternalFilesLambda(DiffInfo.AddExternalFiles);
	AddExternalFilesLambda(DiffInfo.ModifyExternalFiles);
	NewRelease.ExternalFiles = BaseExternalFilesRef;

	return NewRelease;
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
			UE_LOG(LogTemp, Error, TEXT("Deserialize Base Version Faild!"));
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
		UE_LOG(LogTemp, Error, TEXT("Deserialize Base Version Faild!"));
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
	FHotPatcherVersion BaseVersion;

	if (ExportPatchSetting->IsByBaseVersion() && !ExportPatchSetting->GetBaseVersionInfo(BaseVersion))
	{
		UE_LOG(LogTemp, Error, TEXT("Deserialize Base Version Faild!"));
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

	if (!CheckPatchRequire(VersionDiffInfo))
	{
		return FReply::Handled();
	}

	int32 ChunkNum = ExportPatchSetting->IsEnableChunk() ? ExportPatchSetting->GetChunkInfos().Num() : 1;
	

	// save pakversion.json
	//FPakVersion CurrentPakVersion;
	//SavePatchVersionJson(CurrentVersion, CurrentVersionSavePath, CurrentPakVersion);

	// package all selected platform
	TMap<FString,TArray<FPakFileInfo>> PakFilesInfoMap;

	// Check Chunk
	if(ExportPatchSetting->IsEnableChunk())
	{
		FString TotalMsg;
		FChunkInfo TotalChunk = UFlibPatchParserHelper::CombineChunkInfos(ExportPatchSetting->GetChunkInfos());

		FChunkAssetDescribe ChunkDiffInfo = UFlibPatchParserHelper::DiffChunk(NewVersionChunk, TotalChunk, ExportPatchSetting->IsIncludeHasRefAssetsOnly());

		TArray<FString> AllUnselectedAssets = ChunkDiffInfo.GetAssetsStrings();
		TArray<FString> AllUnselectedExFiles = ChunkDiffInfo.GetExFileStrings();
		TArray<FString> UnSelectedInternalFiles = ChunkDiffInfo.GetInternalFileStrings();

		auto ChunkCheckerMsg = [&TotalMsg](const FString& Category,const TArray<FString>& InAssetList)
		{
			if (!!InAssetList.Num())
			{
				TotalMsg.Append(FString::Printf(TEXT("\n%s:\n"),*Category));
				for (const auto& Asset : InAssetList)
				{
					TotalMsg.Append(FString::Printf(TEXT("%s\n"), *Asset));
				}
			}
		};
		ChunkCheckerMsg(TEXT("Unreal Asset"), AllUnselectedAssets);
		ChunkCheckerMsg(TEXT("External Files"), AllUnselectedExFiles);
		ChunkCheckerMsg(TEXT("Internal Files(Patch & Chunk setting not match)"), UnSelectedInternalFiles);

		if (!TotalMsg.IsEmpty())
		{
			ShowMsg(FString::Printf(TEXT("Unselect in Chunk:\n%s"), *TotalMsg));
			return FReply::Handled();
		}
		else
		{
			ShowMsg(TEXT(""));
		}
	}
	else
	{
		// 分析所选过滤器中的资源所依赖的过滤器添加到Chunk中
		// 因为默认情况下Chunk的过滤器不会进行依赖分析，当bEnableChunk未开启时，之前导出的Chunk中的过滤器不包含Patch中所选过滤器进行依赖分析之后的所有资源的模块。
		{
			TArray<FString> DependenciesFilters;

			auto GetKeysLambda = [&DependenciesFilters](const FAssetDependenciesInfo& Assets)
			{
				TArray<FAssetDetail> AllAssets;
				UFLibAssetManageHelperEx::GetAssetDetailsByAssetDependenciesInfo(Assets, AllAssets);
				for (const auto& Asset : AllAssets)
				{
					FString Path;
					FString Filename;
					FString Extension;
					FPaths::Split(Asset.mPackagePath, Path, Filename, Extension);
					DependenciesFilters.AddUnique(Path);
				}
			};
			GetKeysLambda(VersionDiffInfo.AddAssetDependInfo);
			GetKeysLambda(VersionDiffInfo.ModifyAssetDependInfo);

			TArray<FDirectoryPath> DepOtherModule;

			for (const auto& DependenciesFilter : DependenciesFilters)
			{
				if (!!NewVersionChunk.AssetIncludeFilters.Num())
				{
					for (const auto& includeFilter : NewVersionChunk.AssetIncludeFilters)
					{
						if (!includeFilter.Path.StartsWith(DependenciesFilter))
						{
							FDirectoryPath FilterPath;
							FilterPath.Path = DependenciesFilter;
							DepOtherModule.Add(FilterPath);
						}
					}
				}
				else
				{
					FDirectoryPath FilterPath;
					FilterPath.Path = DependenciesFilter;
					DepOtherModule.Add(FilterPath);

				}
			}
			NewVersionChunk.AssetIncludeFilters.Append(DepOtherModule);
		}
	}

	bool bEnableChunk = ExportPatchSetting->IsEnableChunk();

	TArray<FChunkInfo> PakChunks;
	if (bEnableChunk)
	{
		PakChunks = ExportPatchSetting->GetChunkInfos();
	}
	else
	{
		PakChunks.Add(NewVersionChunk);
	}

	float AmountOfWorkProgress = 2.f * (ExportPatchSetting->GetPakTargetPlatforms().Num() * ChunkNum) + 4.0f;
	FScopedSlowTask UnrealPakSlowTask(AmountOfWorkProgress);
	UnrealPakSlowTask.MakeDialog();

	for(const auto& PlatformName :ExportPatchSetting->GetPakTargetPlatformNames())
	{
		// PakModeSingleLambda(PlatformName, CurrentVersionSavePath);
		for (const auto& Chunk : PakChunks)
		{
			// Update Progress Dialog
			{
				FText Dialog = FText::Format(NSLOCTEXT("ExportPatch", "GeneratedPakCommands", "Generating UnrealPak Commands of {0} Platform Chunk {1}."), FText::FromString(PlatformName),FText::FromString(Chunk.ChunkName));
				UnrealPakSlowTask.EnterProgressFrame(1.0, Dialog);
			}
			
			
			FString ChunkSaveBasePath = FPaths::Combine(ExportPatchSetting->SavePath.Path, CurrentVersion.VersionId, PlatformName);
			TArray<FPakFileProxy> PakFileProxys;

			TArray<FPakCommand> ChunkPakCommands = UFlibPatchParserHelper::CollectPakCommandByChunk(VersionDiffInfo, Chunk, PlatformName, ExportPatchSetting->GetPakCommandOptions());
			FString PakPostfix = TEXT("_001_P");
			if(!Chunk.bMonolithic)
			{
				FPakFileProxy SinglePakForChunk;
				SinglePakForChunk.PakCommands = ChunkPakCommands;

				FString ChunkSaveName = ExportPatchSetting->IsEnableChunk() ? FString::Printf(TEXT("%s_%s"), *CurrentVersion.VersionId, *Chunk.ChunkName) : FString::Printf(TEXT("%s"), *CurrentVersion.VersionId);
				SinglePakForChunk.PakCommandSavePath = FPaths::Combine(ChunkSaveBasePath, FString::Printf(TEXT("%s_%s_PakCommands.txt"), *ChunkSaveName, *PlatformName));
				SinglePakForChunk.PakSavePath = FPaths::Combine(ChunkSaveBasePath, FString::Printf(TEXT("%s_%s%s.pak"), *ChunkSaveName, *PlatformName,*PakPostfix));
				PakFileProxys.Add(SinglePakForChunk);
			}
			else
			{
				for (const auto& PakCommand : ChunkPakCommands)
				{
					FPakFileProxy CurrentPak;
					CurrentPak.PakCommands.Add(PakCommand);
					FString Path;
					switch (Chunk.MonolithicPathMode)
					{
						case EMonolithicPathMode::MountPath:
						{
							Path = UFlibPatchParserHelper::MountPathToRelativePath(PakCommand.GetMountPath());
							break;

						};
						case  EMonolithicPathMode::PackagePath:
						{
							Path = PakCommand.AssetPackage;
							break;
						}
					}
					
					CurrentPak.PakCommandSavePath = FPaths::Combine(ChunkSaveBasePath, Chunk.ChunkName, FString::Printf(TEXT("%s_PakCommands.txt"), *Path));
					CurrentPak.PakSavePath = FPaths::Combine(ChunkSaveBasePath, Chunk.ChunkName, FString::Printf(TEXT("%s.pak"), *Path));
					PakFileProxys.Add(CurrentPak);
				}
			}

			// Update SlowTask Progress
			{
			FText Dialog = FText::Format(NSLOCTEXT("ExportPatch", "GeneratedPak", "Generating Pak list of {0} Platform Chunk {1}."), FText::FromString(PlatformName), FText::FromString(Chunk.ChunkName));
			UnrealPakSlowTask.EnterProgressFrame(1.0, Dialog);
			}

			TArray<FString> UnrealPakOptions = ExportPatchSetting->GetUnrealPakOptions();
			TArray<FReplaceText> ReplacePakCommandTexts = ExportPatchSetting->GetReplacePakCommandTexts();
			// 创建chunk的pak文件
			for (const auto& PakFileProxy : PakFileProxys)
			{

				FThread CurrentPakThread(*PakFileProxy.PakSavePath, [/*CurrentPakVersion, */PlatformName, UnrealPakOptions, ReplacePakCommandTexts, PakFileProxy, &Chunk, &PakFilesInfoMap]()
				{

					bool PakCommandSaveStatus = FFileHelper::SaveStringArrayToFile(
						UFlibPatchParserHelper::GetPakCommandStrByCommands(PakFileProxy.PakCommands, ReplacePakCommandTexts),
						*PakFileProxy.PakCommandSavePath,
						FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);

					if (PakCommandSaveStatus)
					{
						TArray<FString> UnrealPakOptionsSinglePak = UnrealPakOptions;
						UnrealPakOptionsSinglePak.Add(
							FString::Printf(
								TEXT("%s -create=%s"),
								*(TEXT("\"") + PakFileProxy.PakSavePath + TEXT("\"")),
								*(TEXT("\"") + PakFileProxy.PakCommandSavePath + TEXT("\""))
							)
						);
						FString CommandLine;
						for (const auto& Option : UnrealPakOptionsSinglePak)
						{
							CommandLine.Append(FString::Printf(TEXT(" %s"), *Option));
						}
						ExecuteUnrealPak(*CommandLine);
						// FProcHandle ProcessHandle = UFlibPatchParserHelper::DoUnrealPak(UnrealPakOptionsSinglePak, true);

						if (FPaths::FileExists(PakFileProxy.PakSavePath))
						{
							FText Msg = LOCTEXT("SavedPakFileMsg", "Successd to Package the patch as Pak.");
							UFlibHotPatcherEditorHelper::CreateSaveFileNotify(Msg, PakFileProxy.PakSavePath);

							FPakFileInfo CurrentPakInfo;
							if (UFlibPatchParserHelper::GetPakFileInfo(PakFileProxy.PakSavePath, CurrentPakInfo))
							{
								// CurrentPakInfo.PakVersion = CurrentPakVersion;
								if (!PakFilesInfoMap.Contains(PlatformName))
								{
									PakFilesInfoMap.Add(PlatformName, TArray<FPakFileInfo>{CurrentPakInfo});
								}
								else
								{
									PakFilesInfoMap.Find(PlatformName)->Add(CurrentPakInfo);
								}
							}
						}
						if (!Chunk.bSavePakCommands)
						{
							IFileManager::Get().Delete(*PakFileProxy.PakCommandSavePath);
						}
					}

				});
				CurrentPakThread.Run();
			}
		}
	}

	// delete pakversion.json
	//{
	//	FString PakVersionSavedPath = UExportPatchSettings::GetSavePakVersionPath(CurrentVersionSavePath,CurrentVersion);
	//	if (ExportPatchSetting->IsIncludePakVersion() && FPaths::FileExists(PakVersionSavedPath))
	//	{
	//		IFileManager::Get().Delete(*PakVersionSavedPath);
	//	}
	//}



	// save asset dependency
	{
		if (ExportPatchSetting->IsSaveAssetRelatedInfo())
		{
			TArray<EAssetRegistryDependencyTypeEx> AssetDependencyTypes;
			AssetDependencyTypes.Add(EAssetRegistryDependencyTypeEx::Packages);

			TArray<FAssetRelatedInfo> AssetsDependency = UFlibPatchParserHelper::GetAssetsRelatedInfoByFAssetDependencies(
				UFLibAssetManageHelperEx::CombineAssetDependencies(VersionDiffInfo.AddAssetDependInfo, VersionDiffInfo.ModifyAssetDependInfo),
				AssetDependencyTypes
			);

			FString AssetsDependencyString;
			UFlibPatchParserHelper::SerializeAssetsRelatedInfoAsString(AssetsDependency, AssetsDependencyString);
			FString SaveAssetRelatedInfoToFile = FPaths::Combine(
				CurrentVersionSavePath,
				FString::Printf(TEXT("%s_AssetRelatedInfos.json"), *CurrentVersion.VersionId)
			);
			if (UFLibAssetManageHelperEx::SaveStringToFile(SaveAssetRelatedInfoToFile, AssetsDependencyString))
			{
				auto Msg = LOCTEXT("SaveAssetRelatedInfo", "Succeed to export Asset Related infos.");
				UFlibHotPatcherEditorHelper::CreateSaveFileNotify(Msg, SaveAssetRelatedInfoToFile);
			}
		}
	}

	// save difference to file
	{
		FText DiaLogMsg = FText::Format(NSLOCTEXT("ExportPatch", "ExportPatchDiffFile", "Generating Diff info of version {0}"), FText::FromString(CurrentVersion.VersionId));
		UnrealPakSlowTask.EnterProgressFrame(1.0, DiaLogMsg);
		SavePatchDiffJson(CurrentVersion, VersionDiffInfo);
	}

	// save Patch Tracked asset info to file
	{
		{
			FText DiaLogMsg = FText::Format(NSLOCTEXT("ExportPatch", "ExportPatchAssetInfo", "Generating Patch Tacked Asset info of version {0}"), FText::FromString(CurrentVersion.VersionId));
			UnrealPakSlowTask.EnterProgressFrame(1.0, DiaLogMsg);
		}
		FString SerializeReleaseVersionInfo;
		FHotPatcherVersion NewReleaseVersion = MakeNewRelease(BaseVersion, CurrentVersion);
		UFlibPatchParserHelper::SerializeHotPatcherVersionToString(NewReleaseVersion, SerializeReleaseVersionInfo);

		FString SaveCurrentVersionToFile = FPaths::Combine(
			CurrentVersionSavePath,
			FString::Printf(TEXT("%s_Release.json"), *CurrentVersion.VersionId)
		);
		if (UFLibAssetManageHelperEx::SaveStringToFile(SaveCurrentVersionToFile, SerializeReleaseVersionInfo))
		{
			auto Msg = LOCTEXT("SavePatchDiffInfo", "Succeed to export New Release Info.");
			UFlibHotPatcherEditorHelper::CreateSaveFileNotify(Msg, SaveCurrentVersionToFile);
		}
	}

	// serialize all pak file info
	{
		{
			FText DiaLogMsg = FText::Format(NSLOCTEXT("ExportPatch", "ExportPatchPakFileInfo", "Generating All Platform Pak info of version {0}"), FText::FromString(CurrentVersion.VersionId));
			UnrealPakSlowTask.EnterProgressFrame(1.0, DiaLogMsg);
		}
		FString PakFilesInfoStr;
		UFlibPatchParserHelper::SerializePlatformPakInfoToString(PakFilesInfoMap, PakFilesInfoStr);

		if (!PakFilesInfoStr.IsEmpty())
		{
			FString SavePakFilesPath = FPaths::Combine(
				CurrentVersionSavePath,
				FString::Printf(TEXT("%s_PakFilesInfo.json"), *CurrentVersion.VersionId)
			);
			if (UFLibAssetManageHelperEx::SaveStringToFile(SavePakFilesPath, PakFilesInfoStr) && FPaths::FileExists(SavePakFilesPath))
			{
				FText Msg = LOCTEXT("SavedPakFileMsg", "Successd to Export the Pak File info.");
				UFlibHotPatcherEditorHelper::CreateSaveFileNotify(Msg, SavePakFilesPath);
			}
		}
	}

	// serialize patch config
	{
		{
			FText DiaLogMsg = FText::Format(NSLOCTEXT("ExportPatch", "ExportPatchConfig", "Generating Current Patch Config of version {0}"), FText::FromString(CurrentVersion.VersionId));
			UnrealPakSlowTask.EnterProgressFrame(1.0, DiaLogMsg);
		}

		FString SaveConfigPath = FPaths::Combine(
			CurrentVersionSavePath,
			FString::Printf(TEXT("%s_PatchConfig.json"),*CurrentVersion.VersionId)
		);

		if (ExportPatchSetting->IsSavePatchConfig())
		{
			FString SerializedJsonStr;
			ExportPatchSetting->SerializePatchConfigToString(SerializedJsonStr);

			if (FFileHelper::SaveStringToFile(SerializedJsonStr, *SaveConfigPath))
			{
				FText Msg = LOCTEXT("SavedPatchConfigMas", "Successd to Export the Patch Config.");
				UFlibHotPatcherEditorHelper::CreateSaveFileNotify(Msg, SaveConfigPath);
			}
		}
	}

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
