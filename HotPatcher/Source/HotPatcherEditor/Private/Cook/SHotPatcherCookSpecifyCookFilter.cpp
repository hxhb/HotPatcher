// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "SHotPatcherCookSpecifyCookFilter.h"
#include "FLibAssetManageHelperEx.h"
#include "SProjectCookMapListRow.h"
#include "Widgets/Input/SHyperlink.h"
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



TSharedPtr<FJsonObject> SHotPatcherCookSpecifyCookFilter::SerializeAsJson() const
{
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	TArray<TSharedPtr<FJsonValue>> CookFiltersJsonValueList;

	for (const auto& Filter : GetSpecifyCookFilterSetting()->GetAlwayCookFilters())
	{
		FString FilterPath = Filter.Path;
		CookFiltersJsonValueList.Add(MakeShareable(new FJsonValueString(FilterPath)));
	}
	JsonObject->SetArrayField(TEXT("CookFilter"), CookFiltersJsonValueList);
	return JsonObject;
}

void SHotPatcherCookSpecifyCookFilter::DeSerializeFromJsonObj(TSharedPtr<FJsonObject>const & InJsonObject)
{
	TArray<TSharedPtr<FJsonValue>> CookFiltersJsonValueList = InJsonObject->GetArrayField(TEXT("CookFilter"));

	for (const auto& FilterJsonValue : CookFiltersJsonValueList)
	{
		FDirectoryPath FilterDirPath;
		FilterDirPath.Path = FilterJsonValue->AsString();
		GetSpecifyCookFilterSetting()->GetAlwayCookFilters().Add(FilterDirPath);
	}

}

FString SHotPatcherCookSpecifyCookFilter::GetSerializeName()const
{
	return TEXT("Filters");
}


void SHotPatcherCookSpecifyCookFilter::Reset()
{
	GetSpecifyCookFilterSetting()->GetAlwayCookFilters().Reset();
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
