// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Model/FOriginalCookerContext.h"
#include "Templates/SharedPointer.h"
#include "FlibPatchParserHelper.h"
#include "Kismet/KismetSystemLibrary.h"

// project header
#include "IOriginalCookerChildWidget.h"

/**
 * Implements the cooked Maps panel.
 */
class SHotPatcherCookMaps
	: public SCompoundWidget,public IOriginalCookerChildWidget
{
public:

	SLATE_BEGIN_ARGS(SHotPatcherCookMaps) { }
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
	bool IsCookAllMap()const { return MapList.Num() == GetCookerContextPtr()->GetAllSelectedCookMap().Num(); }
protected:

	
	// Callback for clicking the 'Select All Maps' button.
	void HandleAllMapHyperlinkNavigate(bool AllMap)
	{
		if (mContext.IsValid())
		{
			if (AllMap)
			{
				TArray<FString> Maps = UFlibPatchParserHelper::GetAvailableMaps(UKismetSystemLibrary::GetProjectDirectory(),false,false,true);

				for (int32 MapIndex = 0; MapIndex < Maps.Num(); ++MapIndex)
				{
					GetCookerContextPtr()->AddSelectedCookMap(Maps[MapIndex]);
				}
			}
			else {
				GetCookerContextPtr()->ClearAllMap();
			}

		}
	}

	TSharedRef<ITableRow> HandleMapListViewGenerateRow(TSharedPtr<FString> InItem, const TSharedRef<STableViewBase>& OwnerTable);
	void RefreshMapList();
private:

	/** Holds the map list. */
	TArray<TSharedPtr<FString> > MapList;
	/** Holds the map list view. */
	TSharedPtr<SListView<TSharedPtr<FString> > > MapListView;

};

