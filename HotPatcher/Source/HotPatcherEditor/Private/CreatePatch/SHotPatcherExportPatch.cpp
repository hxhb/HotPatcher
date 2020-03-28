// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

// #include "HotPatcherPrivatePCH.h"
#include "SHotPatcherExportPatch.h"
#include "FlibHotPatcherEditorHelper.h"
#include "FlibPatchParserHelper.h"
#include "FHotPatcherVersion.h"
#include "FLibAssetManageHelperEx.h"
#include "FPakFileInfo.h"
// engine header
#include "Misc/FileHelper.h"
#include "Widgets/Input/SHyperlink.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Text/SMultiLineEditableText.h"

#include "Kismet/KismetSystemLibrary.h"
#include "Misc/SecureHash.h"
#include "Misc/ScopedSlowTask.h"
#include "HAL/FileManager.h"

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
	TSharedPtr<UExportPatchSettings> DefaultSetting = MakeShareable(NewObject<UExportPatchSettings>());
	FString DefaultSettingJson;
	DefaultSetting->SerializePatchConfigToString(DefaultSettingJson);
	UFlibHotPatcherEditorHelper::DeserializePatchConfig(ExportPatchSetting, DefaultSettingJson);
	SettingsView->ForceRefresh();

}
void SHotPatcherExportPatch::DoGenerate()
{
	DoExportPatch();
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

	FHotPatcherVersion CurrentVersion = UFlibHotPatcherEditorHelper::ExportReleaseVersionInfo(
		ExportPatchSetting->GetVersionId(),
		BaseVersion.VersionId,
		FDateTime::UtcNow().ToString(),
		ExportPatchSetting->GetAssetIncludeFilters(),
		ExportPatchSetting->GetAssetIgnoreFilters(),
		ExportPatchSetting->GetIncludeSpecifyAssets(),
		ExportPatchSetting->GetAllExternFiles(true),
		ExportPatchSetting->IsIncludeHasRefAssetsOnly()
	);

	FString CurrentVersionSavePath = FPaths::Combine(ExportPatchSetting->GetSaveAbsPath(), CurrentVersion.VersionId);

	// parser version difference

	FAssetDependenciesInfo BaseVersionAssetDependInfo = BaseVersion.AssetInfo;
	FAssetDependenciesInfo CurrentVersionAssetDependInfo = CurrentVersion.AssetInfo;

	FAssetDependenciesInfo AddAssetDependInfo;
	FAssetDependenciesInfo ModifyAssetDependInfo;
	FAssetDependenciesInfo DeleteAssetDependInfo;

	UFlibPatchParserHelper::DiffVersionAssets(
		CurrentVersionAssetDependInfo,
		BaseVersionAssetDependInfo,
		AddAssetDependInfo,
		ModifyAssetDependInfo,
		DeleteAssetDependInfo
	);

	TArray<FExternAssetFileInfo> AddExternalFiles;
	TArray<FExternAssetFileInfo> ModifyExternalFiles;
	TArray<FExternAssetFileInfo> DeleteExternalFiles;

	UFlibPatchParserHelper::DiffVersionExFiles(CurrentVersion, BaseVersion, AddExternalFiles, ModifyExternalFiles, DeleteExternalFiles);

	// debug
	//{
	//	auto VersionPrint = [](const FHotPatcherVersion& CurrentVersion)
	//	{
	//		TArray<FString> Keys;
	//		CurrentVersion.ExternalFiles.GetKeys(Keys);

	//		for (const auto& File : Keys)
	//		{
	//			FExternAssetFileInfo CurrentFile = *CurrentVersion.ExternalFiles.Find(File);
	//			UE_LOG(LogTemp, Warning, TEXT("new External Files filepath %s"), *CurrentFile.FilePath.FilePath);
	//			UE_LOG(LogTemp, Warning, TEXT("new External Files filehash %s"), *CurrentFile.FileHash);
	//			UE_LOG(LogTemp, Warning, TEXT("new External Files mountpath %s"), *CurrentFile.MountPath);
	//		}
	//	};

	//	UE_LOG(LogTemp, Warning, TEXT("---------------------"));
	//	VersionPrint(CurrentVersion);
	//	UE_LOG(LogTemp, Warning, TEXT("---------------------"));
	//	VersionPrint(BaseVersion);
	//	UE_LOG(LogTemp, Warning, TEXT("---------------------"));
	//}
	FString SerializeDiffInfo =
		FString::Printf(TEXT("%s\n%s\n"),
			*UFlibPatchParserHelper::SerializeDiffAssetsInfomationToString(AddAssetDependInfo, ModifyAssetDependInfo, DeleteAssetDependInfo),
			ExportPatchSetting->IsEnableExternFilesDiff()?
			*UFlibPatchParserHelper::SerializeDiffExternalFilesInfomationToString(AddExternalFiles, ModifyExternalFiles, DeleteExternalFiles):
			*UFlibPatchParserHelper::SerializeDiffExternalFilesInfomationToString(ExportPatchSetting->GetAllExternFiles(), TArray<FExternAssetFileInfo>{}, TArray<FExternAssetFileInfo>{})
		);
	SetInformationContent(SerializeDiffInfo);
	SetInfomationContentVisibility(EVisibility::Visible);

	return FReply::Handled();
}
bool SHotPatcherExportPatch::CanDiff()const
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
		bool bHasFilter = !!ExportPatchSetting->GetAssetIncludeFilters().Num();
		bool bHasSpecifyAssets = !!ExportPatchSetting->GetIncludeSpecifyAssets().Num();
		bool bHasExternFiles = !!ExportPatchSetting->GetAddExternFiles().Num();
		bool bHasExDirs = !!ExportPatchSetting->GetAddExternDirectory().Num();
		bool bHasSavePath = !ExportPatchSetting->GetSaveAbsPath().IsEmpty();
		bool bHasPakPlatfotm = !!ExportPatchSetting->GetPakTargetPlatforms().Num();

		bool bHasAnyPakFiles = (
			bHasFilter || bHasSpecifyAssets || bHasExternFiles || bHasExDirs ||
			ExportPatchSetting->IsIncludeEngineIni()||
			ExportPatchSetting->IsIncludePluginIni()||
			ExportPatchSetting->IsIncludeProjectIni()
		);
		bCanExport = bHasBase && bHasVersionId && bHasAnyPakFiles && bHasPakPlatfotm && bHasSavePath;
	}
	return bCanExport;
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

FPatchVersionDiff SHotPatcherExportPatch::DiffPatchVersion(const FHotPatcherVersion& Base, const FHotPatcherVersion& New)const
{
	FPatchVersionDiff VersionDiffInfo;
	FAssetDependenciesInfo BaseVersionAssetDependInfo = Base.AssetInfo;
	FAssetDependenciesInfo CurrentVersionAssetDependInfo = New.AssetInfo;

	UFlibPatchParserHelper::DiffVersionAssets(
		CurrentVersionAssetDependInfo,
		BaseVersionAssetDependInfo,
		VersionDiffInfo.AddAssetDependInfo,
		VersionDiffInfo.ModifyAssetDependInfo,
		VersionDiffInfo.DeleteAssetDependInfo
	);

	UFlibPatchParserHelper::DiffVersionExFiles(
		New,
		Base,
		VersionDiffInfo.AddExternalFiles,
		VersionDiffInfo.ModifyExternalFiles,
		VersionDiffInfo.DeleteExternalFiles
	);

	return VersionDiffInfo;
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


FProcHandle SHotPatcherExportPatch::DoUnrealPak(const FString& PakCommandPath, const FString& SavePakPath, TArray<FString> UnrealPakOptions)
{
	FString UnrealPakBinary = UFlibPatchParserHelper::GetUnrealPakBinary();

	FString CommandLine = FString::Printf(
		TEXT("%s -create=%s"),
		*(TEXT("\"") + SavePakPath + TEXT("\"")),
		*(TEXT("\"") + PakCommandPath + TEXT("\""))
	);

	for (const auto& Option : UnrealPakOptions)
	{
		CommandLine.Append(FString::Printf(TEXT(" %s"), *Option));
	}

	// create UnrealPak process

	uint32 *ProcessID = NULL;
	return FPlatformProcess::CreateProc(*UnrealPakBinary, *CommandLine, true, false, false, ProcessID, 0, NULL, NULL, NULL);
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
	FPatchVersionDiff DiffInfo = DiffPatchVersion(BaseVersion,InCurrentVersion);
	
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

FReply SHotPatcherExportPatch::DoExportPatch()
{
	FHotPatcherVersion BaseVersion;

	if (ExportPatchSetting->IsByBaseVersion() && !ExportPatchSetting->GetBaseVersionInfo(BaseVersion))
	{
		UE_LOG(LogTemp, Error, TEXT("Deserialize Base Version Faild!"));
		return FReply::Handled();
	}

	UFLibAssetManageHelperEx::UpdateAssetMangerDatabase(true);
	FHotPatcherVersion CurrentVersion = ExportPatchSetting->GetNewPatchVersionInfo();
	FString CurrentVersionSavePath = ExportPatchSetting->GetCurrentVersionSavePath();
	FPatchVersionDiff VersionDiffInfo = DiffPatchVersion(BaseVersion, CurrentVersion);

	if (!CheckPatchRequire(VersionDiffInfo))
	{
		return FReply::Handled();
	}

	float AmountOfWorkProgress = 2.f * ExportPatchSetting->GetPakTargetPlatforms().Num() + 4.0f;
	FScopedSlowTask UnrealPakSlowTask(AmountOfWorkProgress);
	UnrealPakSlowTask.MakeDialog();

	// save pakversion.json
	FPakVersion CurrentPakVersion;
	SavePatchVersionJson(CurrentVersion, CurrentVersionSavePath, CurrentPakVersion);

	// package all selected platform
	TMap<FString,FPakFileInfo> PakFilesInfoMap;
	for(const auto& PlatformName :ExportPatchSetting->GetPakTargetPlatformNames())
	{
		// Update Progress Dialog
		{
			FText Dialog = FText::Format(NSLOCTEXT("ExportPatch", "GeneratedPakCommands", "Generating UnrealPak Commands of {0} Platform."), FText::FromString(PlatformName));
			UnrealPakSlowTask.EnterProgressFrame(1.0, Dialog);
		}

		FString SavePakCommandPath = UExportPatchSettings::GetPakCommandsSaveToPath(CurrentVersionSavePath, PlatformName, CurrentVersion);

		SavePakCommands(PlatformName, VersionDiffInfo, SavePakCommandPath);
		
		// Update SlowTask Progress
		{
			FText Dialog = FText::Format(NSLOCTEXT("ExportPatch", "GeneratedPak", "Generating Pak list of {0} Platform."), FText::FromString(PlatformName));
			UnrealPakSlowTask.EnterProgressFrame(1.0, Dialog);
		}

		// create UnrealPak.exe create .pak file
		{
			FString SavePakFilePath = FPaths::Combine(
				CurrentVersionSavePath,
				PlatformName,
				FString::Printf(TEXT("%s_%s_001_P.pak"), *CurrentVersion.VersionId, *PlatformName)
			);

			FProcHandle ProcessHandle = DoUnrealPak(SavePakCommandPath, SavePakFilePath, ExportPatchSetting->GetUnrealPakOptions());
			FPlatformProcess::WaitForProc(ProcessHandle);

			if (FPaths::FileExists(SavePakFilePath))
			{
				FText Msg = LOCTEXT("SavedPakFileMsg", "Successd to Package the patch as Pak.");
				UFlibHotPatcherEditorHelper::CreateSaveFileNotify(Msg, SavePakFilePath);
				
				FPakFileInfo CurrentPakInfo;
				if (UFlibPatchParserHelper::GetPakFileInfo(SavePakFilePath, CurrentPakInfo))
				{
					CurrentPakInfo.PakVersion = CurrentPakVersion;
					PakFilesInfoMap.Add(PlatformName,CurrentPakInfo);
				}
			}
		}

		// is save PakList?
		if (!ExportPatchSetting->IsSavePakList() && FPaths::FileExists(SavePakCommandPath))
		{
			IFileManager::Get().Delete(*SavePakCommandPath);
		}
	}

	// delete pakversion.json
	{
		FString PakVersionSavedPath = UExportPatchSettings::GetSavePakVersionPath(CurrentVersionSavePath,CurrentVersion);
		if (ExportPatchSetting->IsIncludePakVersion() && FPaths::FileExists(PakVersionSavedPath))
		{
			IFileManager::Get().Delete(*PakVersionSavedPath);
		}
	}
	
	// save difference to file
	{
		FText DiaLogMsg = FText::Format(NSLOCTEXT("ExportPatch","ExportPatchDiffFile","Generating Diff info of version {0}"),FText::FromString(CurrentVersion.VersionId));
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
