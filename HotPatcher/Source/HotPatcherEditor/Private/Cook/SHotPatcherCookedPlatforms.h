// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Interfaces/ITargetPlatformManagerModule.h"
#include "Interfaces/ITargetPlatform.h"
#include "Model/FHotPatcherCookModel.h"
#include "Templates/SharedPointer.h"
#include "Dom/JsonObject.h"

// project header
#include "ICookerItemInterface.h"

/**
 * Implements the cooked platforms panel.
 */
class SHotPatcherCookedPlatforms
	: public SCompoundWidget,public ICookerItemInterface
{
public:

	SLATE_BEGIN_ARGS(SHotPatcherCookedPlatforms) { }
	SLATE_END_ARGS()

public:

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs The Slate argument list.
	 */
	void Construct(	const FArguments& InArgs,TSharedPtr<FHotPatcherCookModel> InCookModel);

public:
	virtual TSharedPtr<FJsonObject> SerializeAsJson()const override;
	virtual void DeSerializeFromJsonObj(TSharedPtr<FJsonObject>const & InJsonObject)override;
	virtual FString GetSerializeName()const override;
	virtual void Reset() override;
protected:

	/**
	 * Builds the platform menu.
	 *
	 * @return Platform menu widget.
	 */
	void MakePlatformMenu( )
	{
		TArray<ITargetPlatform*> Platforms = GetTargetPlatformManager()->GetTargetPlatforms();

		if (Platforms.Num() > 0)
		{
			PlatformList.Reset();
			for (int32 PlatformIndex = 0; PlatformIndex < Platforms.Num(); ++PlatformIndex)
			{
				FString PlatformName = Platforms[PlatformIndex]->PlatformName();

				PlatformList.Add(MakeShareable(new FString(PlatformName)));
			}
		}
	}

private:

	// Callback for clicking the 'Select All Platforms' button.
	void HandleAllPlatformsHyperlinkNavigate( bool AllPlatforms )
	{

		if (mCookModel.IsValid())
		{
			if (AllPlatforms)
			{
				TArray<ITargetPlatform*> Platforms = GetTargetPlatformManager()->GetTargetPlatforms();

				for (int32 PlatformIndex = 0; PlatformIndex < Platforms.Num(); ++PlatformIndex)
				{
					mCookModel->AddSelectedCookPlatform(Platforms[PlatformIndex]->PlatformName());
				}
			}
			else {
				mCookModel->ClearAllPlatform();
			}

		}
	}


	// Handles generating a row widget in the map list view.
	TSharedRef<ITableRow> HandlePlatformListViewGenerateRow( TSharedPtr<FString> InItem, const TSharedRef<STableViewBase>& OwnerTable );

private:

	// Holds the platform list.
	TArray<TSharedPtr<FString> > PlatformList;

	// Holds the platform list view.
	TSharedPtr<SListView<TSharedPtr<FString> > > PlatformListView;

	TSharedPtr<FHotPatcherCookModel> mCookModel;
};

