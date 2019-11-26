// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "SProjectVersionControlPage.h"

#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SExpandableArea.h"


#define LOCTEXT_NAMESPACE "SProjectVersionControlPage"


/* SProjectVersionControlPage interface
 *****************************************************************************/

void SProjectVersionControlPage::Construct(const FArguments& InArgs, TSharedPtr<FHotPatcherVersionControlModel> InVersionControlModel)
{
	mVersionControlModel = InVersionControlModel;

	ChildSlot
	[
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.FillWidth(1.0)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("WhichProjectToUseText", "How would you like to VersionControl the content?"))
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0, 8.0, 0.0, 0.0)
		[
			SNew(SExpandableArea)
			.AreaTitle(LOCTEXT("SelectVersionControlPlatform", "Patch Setting"))
			.InitiallyCollapsed(true)
			.Padding(8.0)
			.BodyContent()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
			]
			
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0, 8.0, 0.0, 0.0)
	];
}


#undef LOCTEXT_NAMESPACE
