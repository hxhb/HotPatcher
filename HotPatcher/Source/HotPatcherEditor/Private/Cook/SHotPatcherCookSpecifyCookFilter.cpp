// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "SHotPatcherCookSpecifyCookFilter.h"
#include "FLibAssetManageHelperEx.h"
#include "SProjectCookMapListRow.h"
#include "SHyperlink.h"
#include "Widgets/Layout/SSeparator.h"
#include "FlibPatchParserHelper.h"
#include "Kismet/KismetSystemLibrary.h"

#define LOCTEXT_NAMESPACE "SHotPatcherCookSpecifyCookFilter"

void SHotPatcherCookSpecifyCookFilter::Construct(const FArguments& InArgs, TSharedPtr<FHotPatcherCookModel> InCookModel)
{

	mCookModel = InCookModel;
	mCookModel->OnRequestSpecifyCookFilter.BindRaw(this, &SHotPatcherCookSpecifyCookFilter::HandleRequestSpecifyCookFilter);
	CreateFilterListView();

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
	];
	SpecifyCookFilterSetting = USpecifyCookFilterSetting::Get();
	SettingsView->SetObject(SpecifyCookFilterSetting);
}



USpecifyCookFilterSetting* SHotPatcherCookSpecifyCookFilter::GetSpecifyCookFilterSetting() const
{
	return SpecifyCookFilterSetting;
}


TArray<FDirectoryPath> SHotPatcherCookSpecifyCookFilter::GetAlwayCookDirectory() const
{
	if (GetSpecifyCookFilterSetting())
	{
		return GetSpecifyCookFilterSetting()->GetAlwayCookFilters();
	}
	else
	{
		return TArray<FDirectoryPath>{};
	}
}


TArray<FString> SHotPatcherCookSpecifyCookFilter::GetAlwayCookAbsDirectory()
{
	TArray<FString> AlwayCookDirAbsPathList;
	for (const auto& AlwayCookRelativeDir : GetAlwayCookDirectory())
	{
		FString AbsPath;
		if (UFLibAssetManageHelperEx::ConvRelativeDirToAbsDir(AlwayCookRelativeDir.Path, AbsPath))
		{
			AlwayCookDirAbsPathList.AddUnique(AbsPath);
		}
	}
	return AlwayCookDirAbsPathList;
}

void SHotPatcherCookSpecifyCookFilter::HandleRequestSpecifyCookFilter(TArray<FDirectoryPath>& OutCookDir)
{
	OutCookDir = GetAlwayCookDirectory();
}

void SHotPatcherCookSpecifyCookFilter::CreateFilterListView()
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
