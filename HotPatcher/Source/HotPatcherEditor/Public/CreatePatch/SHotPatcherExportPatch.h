// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Interfaces/ITargetPlatformManagerModule.h"
#include "Interfaces/ITargetPlatform.h"
#include "Model/FHotPatcherCreatePatchModel.h"
#include "ExportPatchSettings.h"
#include "SHotPatcherInformations.h"
#include "SHotPatcherPatchableBase.h"
#include "FPatchVersionDiff.h"

// engine header
#include "Interfaces/ITargetPlatform.h"
#include "Templates/SharedPointer.h"
#include "IDetailsView.h"
#include "PropertyEditorModule.h"
#include "Widgets/Text/SMultiLineEditableText.h"


/**
 * Implements the cooked platforms panel.
 */
class SHotPatcherExportPatch
	: public SHotPatcherPatchableBase
{
public:

	SLATE_BEGIN_ARGS(SHotPatcherExportPatch) { }
	SLATE_END_ARGS()

public:

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs The Slate argument list.
	 */
	void Construct(	const FArguments& InArgs,TSharedPtr<FHotPatcherCreatePatchModel> InCreateModel);

// IPatchableInterface
public:
	virtual void ImportConfig();
	virtual void ExportConfig()const;
	virtual void ResetConfig();
	virtual void DoGenerate();

protected:
	void CreateExportFilterListView();
	bool CanExportPatch()const;
	FReply DoExportPatch();

	bool CanPreviewPatch()const;
	FReply DoPreviewPatch();

	FReply DoDiff()const;
	bool CanDiff()const;
	FReply DoClearDiff()const;
	EVisibility VisibilityDiffButtons()const;

	FReply DoPreviewChunk()const;
	bool CanPreviewChunk()const;
	EVisibility VisibilityPreviewChunkButtons()const;

	bool InformationContentIsVisibility()const;
	void SetInformationContent(const FString& InContent)const;
	void SetInfomationContentVisibility(EVisibility InVisibility)const;

protected:
	bool CheckSelectedAssetsCookStatus(const TArray<FString>& PlatformNames, const FAssetDependenciesInfo& SelectedAssets,FString& OutMsg)const;

	bool CheckPatchRequire(const FPatchVersionDiff& InDiff)const;
	bool ShowMsg(const FString& InMsg)const;
	bool SavePatchVersionJson(const FHotPatcherVersion& InSaveVersion,const FString& InSavePath,FPakVersion& OutPakVersion);
	bool SavePatchDiffJson(const FHotPatcherVersion& InSaveVersion, const FPatchVersionDiff& InDiff);
	bool SavePakCommands(const FString& InPlatformName, const FPatchVersionDiff& InDiffInfo, const FString& InSavePath);


	FHotPatcherVersion MakeNewRelease(const FHotPatcherVersion& InBaseVersion, const FHotPatcherVersion& InCurrentVersion)const;
	FProcHandle DoUnrealPak(TArray<FString> UnrealPakOptions,bool block=false);
private:

	// TSharedPtr<FHotPatcherCreatePatchModel> mCreatePatchModel;

	/** Settings view ui element ptr */
	TSharedPtr<IDetailsView> SettingsView;

	UExportPatchSettings* ExportPatchSetting;

	TSharedPtr<SHotPatcherInformations> DiffWidget;
};

