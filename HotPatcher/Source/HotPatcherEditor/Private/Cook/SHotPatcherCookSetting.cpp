// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

// #include "HotPatcherPrivatePCH.h"
#include "SHotPatcherCookSetting.h"
#include "SHotPatcherPlatformListRow.h"
#include "SHyperlink.h"
#include "Widgets/Layout/SSeparator.h"

#define LOCTEXT_NAMESPACE "SHotPatcherCookSetting"

void SHotPatcherCookSetting::Construct(const FArguments& InArgs, TSharedPtr<FHotPatcherCookModel> InCookModel)
{

	mCookModel = InCookModel;

	ChildSlot
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.FillHeight(1.0f)

			// + SVerticalBox::Slot()
			// 	.AutoHeight()
			// 	.Padding(0.0f, 6.0f, 0.0f, 4.0f)
			// 	[
			// 		SNew(SSeparator)
			// 		.Orientation(Orient_Horizontal)
			// 	]

		];
}


#undef LOCTEXT_NAMESPACE
