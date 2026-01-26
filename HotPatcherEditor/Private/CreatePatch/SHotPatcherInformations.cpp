#include "CreatePatch/SHotPatcherInformations.h"

// engine header
#include "Widgets/Input/SHyperlink.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/Layout/SHeader.h"
#include "Internationalization/Internationalization.h"
#include "Widgets/Layout/SExpandableArea.h"

void SHotPatcherInformations::Construct(const FArguments& InArgs)
{
	ChildSlot
	[

		SAssignNew(DiffAreaWidget, SExpandableArea)
		.AreaTitle(FText::FromString(TEXT("Informations")))
		.InitiallyCollapsed(false)
		.Padding(8.0)
		.BodyContent()
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.VAlign(VAlign_Fill)
				.FillWidth(1.0)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.HAlign(HAlign_Left)
					.Padding(4, 4, 10, 4)
					[
						SAssignNew(MulitText, SMultiLineEditableText)
						.IsReadOnly(true)
					]
				]
			]
		]
	];
}

void SHotPatcherInformations::SetExpanded(bool InExpand)
{
	DiffAreaWidget->SetExpanded_Animated(InExpand);
}

void SHotPatcherInformations::SetContent(const FString& InContent)
{
	MulitText->SetText(FText::FromString(InContent));
}