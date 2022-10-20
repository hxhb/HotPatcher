// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Interfaces/ITargetPlatformManagerModule.h"
#include "Interfaces/ITargetPlatform.h"
#include "Model/FOriginalCookerContext.h"
#include "Templates/SharedPointer.h"
#include "Widgets/Input/SEditableTextBox.h"
// project header
#include "IOriginalCookerChildWidget.h"

/**
 * Implements the cooked platforms panel.
 */
class SHotPatcherCookSetting
	: public SCompoundWidget,public IOriginalCookerChildWidget
{
public:

	SLATE_BEGIN_ARGS(SHotPatcherCookSetting) { }
	SLATE_END_ARGS()

public:

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs The Slate argument list.
	 */
	void Construct(	const FArguments& InArgs,TSharedPtr<FHotPatcherContextBase> InContext);
public:
	virtual TSharedPtr<FJsonObject> SerializeAsJson()const override;
	virtual void DeSerializeFromJsonObj(TSharedPtr<FJsonObject>const & InJsonObject)override;
	virtual FString GetSerializeName()const override;
	virtual void Reset() override;

protected:
	TSharedRef<ITableRow> HandleCookSettingListViewGenerateRow(TSharedPtr<FString> InItem, const TSharedRef<STableViewBase>& OwnerTable);
	void RefreshSettingsList();

	void HandleRequestExSettings(TArray<FString>& OutExSettings);
	// FReply ConfirmExCookSetting();


	TArray<FString> GetDefaultCookParams() const
	{
		return TArray<FString>{"-NoLogTimes", "-UTF8Output"};
	}

private:
	/** Holds the map list. */
	TArray<TSharedPtr<FString> > SettingList;

	/** Holds the map list view. */
	TSharedPtr<SListView<TSharedPtr<FString> > > SettingListView;
	
	// Extern Cook Setting Param
	TSharedPtr<SEditableTextBox> ExternSettingTextBox;
};

