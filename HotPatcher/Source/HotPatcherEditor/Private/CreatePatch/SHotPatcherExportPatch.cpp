// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

// #include "HotPatcherPrivatePCH.h"
#include "CreatePatch/SHotPatcherExportPatch.h"
#include "CreatePatch/FExportPatchSettings.h"
#include "CreatePatch/PatcherProxy.h"
#include "CreatePatch/ScopedSlowTaskContext.h"

#include "FlibHotPatcherEditorHelper.h"
#include "FlibPatchParserHelper.h"
#include "FHotPatcherVersion.h"
#include "FLibAssetManageHelperEx.h"
#include "FPakFileInfo.h"
#include "ThreadUtils/FThreadUtils.hpp"
#include "HotPatcherLog.h"
#include "HotPatcherEditor.h"

// engine header
#include "Misc/FileHelper.h"
#include "Widgets/Input/SHyperlink.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Text/SMultiLineEditableText.h"
#include "Kismet/KismetStringLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/SecureHash.h"
#include "HAL/FileManager.h"
#include "PakFileUtilities.h"


#define LOCTEXT_NAMESPACE "SHotPatcherCreatePatch"

void SHotPatcherExportPatch::Construct(const FArguments& InArgs, TSharedPtr<FHotPatcherCreatePatchModel> InCreatePatchModel)
{
	ExportPatchSetting = MakeShareable(new FExportPatchSettings);
	GPatchSettings = ExportPatchSetting.Get();
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
		UFlibPatchParserHelper::TDeserializeJsonStringAsStruct(JsonContent,*ExportPatchSetting);
		SettingsView->GetDetailsView()->ForceRefresh();
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
			UFlibPatchParserHelper::TSerializeStructAsJsonString(*ExportPatchSetting,SerializedJsonStr);
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
	FString DefaultSettingJson;
	UFlibPatchParserHelper::TSerializeStructAsJsonString(*FExportPatchSettings::Get(),DefaultSettingJson);
	UFlibPatchParserHelper::TDeserializeJsonStringAsStruct(DefaultSettingJson,*ExportPatchSetting);
	SettingsView->GetDetailsView()->ForceRefresh();

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
			bDeserializeStatus = UFlibPatchParserHelper::TDeserializeJsonStringAsStruct(BaseVersionContent, BaseVersion);
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
		ExportPatchSetting->GetAddExternAssetsToPlatform(),
		ExportPatchSetting->IsIncludeHasRefAssetsOnly()
	);

	FPatchVersionDiff VersionDiffInfo = UFlibPatchParserHelper::DiffPatchVersionWithPatchSetting(*ExportPatchSetting, BaseVersion, CurrentVersion);
	
	bool bShowDeleteAsset = false;
	FString SerializeDiffInfo;
	UFlibPatchParserHelper::TSerializeStructAsJsonString(VersionDiffInfo,SerializeDiffInfo);
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
			BaseVersion.PlatformAssets.Empty();
		}
	}

	UFLibAssetManageHelperEx::UpdateAssetMangerDatabase(true);
	FChunkInfo NewVersionChunk = UFlibHotPatcherEditorHelper::MakeChunkFromPatchSettings(ExportPatchSetting.Get());

	FHotPatcherVersion CurrentVersion = UFlibPatchParserHelper::ExportReleaseVersionInfoByChunk(
		ExportPatchSetting->GetVersionId(),
		BaseVersion.VersionId,
		FDateTime::UtcNow().ToString(),
		NewVersionChunk,
		ExportPatchSetting->IsIncludeHasRefAssetsOnly()
	);

	FString CurrentVersionSavePath = ExportPatchSetting->GetCurrentVersionSavePath();
	FPatchVersionDiff VersionDiffInfo = UFlibPatchParserHelper::DiffPatchVersionWithPatchSetting(*ExportPatchSetting, BaseVersion, CurrentVersion);

	FString ShowMsg;
	for (const auto& Chunk : ExportPatchSetting->GetChunkInfos())
	{	
		FChunkAssetDescribe ChunkAssetsDescrible = UFlibPatchParserHelper::CollectFChunkAssetsDescribeByChunk(VersionDiffInfo, Chunk,ExportPatchSetting->GetPakTargetPlatforms());
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
		
		for(auto Platform:ExportPatchSetting->GetPakTargetPlatforms())
		{
			TArray<FString> PlatformExFiles;
			FString PlatformName = UFlibPatchParserHelper::GetEnumNameByValue(Platform,false);
			PlatformExFiles.Append(ChunkAssetsDescrible.GetExFileStrings(Platform));
			AppendFilesToMsg(PlatformName, PlatformExFiles);
		}
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
	PatcherProxy->AddToRoot();
	PatcherProxy->SetProxySettings(ExportPatchSetting.Get());
	PatcherProxy->OnShowMsg.AddRaw(this,&SHotPatcherExportPatch::ShowMsg);
	PatcherProxy->DoExport();

	return FReply::Handled();
}

bool SHotPatcherExportPatch::CanPreviewPatch() const
{
	bool bHasFilter = !!ExportPatchSetting->GetAssetIncludeFiltersPaths().Num();
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
	bool bHasExternFiles = HasExFilesLambda();
	
	bool bHasAnyPakFiles = (
		bHasFilter || bHasSpecifyAssets || bHasExternFiles ||
		ExportPatchSetting->IsIncludeEngineIni() ||
		ExportPatchSetting->IsIncludePluginIni() ||
		ExportPatchSetting->IsIncludeProjectIni()
		);

	return bHasFilter || bHasSpecifyAssets || bHasExternFiles || bHasAnyPakFiles;
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
			BaseVersion.PlatformAssets.Empty();
		}
	}

	FChunkInfo NewVersionChunk = UFlibHotPatcherEditorHelper::MakeChunkFromPatchSettings(ExportPatchSetting.Get());
	
	FChunkAssetDescribe ChunkAssetsDescrible = UFlibPatchParserHelper::DiffChunkByBaseVersionWithPatchSetting(*ExportPatchSetting.Get(),NewVersionChunk, DefaultChunk, BaseVersion);

	TArray<FString> AllUnselectedAssets = ChunkAssetsDescrible.GetAssetsStrings();
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
	ChunkCheckerMsg(TEXT("External Files"), TArray<FString>{});
	for(auto Platform:ExportPatchSetting->GetPakTargetPlatforms())
	{
		TArray<FString> PlatformExFiles;
		FString PlatformName = UFlibPatchParserHelper::GetEnumNameByValue(Platform,false);
		PlatformExFiles.Append(ChunkAssetsDescrible.GetExFileStrings(Platform));
		PlatformExFiles.Append(ChunkAssetsDescrible.GetExFileStrings(ETargetPlatform::AllPlatforms));
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

void SHotPatcherExportPatch::CreateExportFilterListView()
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
