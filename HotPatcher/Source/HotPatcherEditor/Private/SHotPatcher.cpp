#include "SHotPatcher.h"
#include "Cooker/OriginalCooker/SOriginalCookWidget.h"
#include "CreatePatch/SPatchersPage.h"
#include "SVersionUpdater/SVersionUpdaterWidget.h"
#include "Cooker/SCookersPage.h"
// engine header
#include "Styling/SlateTypes.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/Layout/SSeparator.h"
#include "EditorStyleSet.h"
#include "PropertyEditorModule.h"
#include "Modules/ModuleManager.h"
#include "IDetailsView.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "AssetRegistryModule.h"
#include "ARFilter.h"
#include "Modules/ModuleManager.h"
#include "ISettingsModule.h"
#include "HAL/PlatformFile.h"
#include "HAL/PlatformFilemanager.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Json.h"

#include "Misc/SecureHash.h"
#include "Misc/PackageName.h"

#define LOCTEXT_NAMESPACE "SHotPatcher"

void SHotPatcher::Construct(const FArguments& InArgs)
{
	HotPatcherModelsMap.Add(TEXT("Cooker"),MakeShareable(new FCookersModeContext));
	HotPatcherModelsMap.Add(TEXT("Patch"),MakeShareable(new FPatchersModeContext));

	ChildSlot
	[
		SNew(SOverlay)
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			.Padding(0.0f, 10.0f, 8.0f, 0.0f)
			[
				SNew(SVerticalBox)
#if ENABLE_UPDATER_CHECK
				+SVerticalBox::Slot()
				.AutoHeight()
				[
					SAssignNew(VersionUpdaterWidget,SVersionUpdaterWidget)
					.ToolName(FText::FromString(ANSI_TO_TCHAR(TOOL_NAME)))
					.DeveloperName(FText::FromString(TEXT("lipengzha")))
					.DeveloperWebsite(FText::FromString(TEXT("https://imzlp.com")))
					.UpdateWebsite(FText::FromString(TEXT("https://imzlp.com/posts/17590/")))
					.CurrentVersion(CURRENT_VERSION_ID)
					.PatchVersion(CURRENT_PATCH_ID)
				]
#endif
				+SVerticalBox::Slot()
				.Padding(0,10,0,0)
				.AutoHeight()
				[
					SNew(SGridPanel)
					.FillColumn(1, 1.0f)
					// cook section
					+ SGridPanel::Slot(0, 0)
					.Padding(8.0f, 10.0f, 0.0f, 0.0f)
					.VAlign(VAlign_Top)
					[
					SNew(STextBlock)
					.Font(FCoreStyle::GetDefaultFontStyle("Cooker", 13))
					.Text(LOCTEXT("ProjectCookSectionHeader", "Cooker"))
					]
					+ SGridPanel::Slot(1, 0)
					.Padding(32.0f, 0.0f, 8.0f, 0.0f)
					[
						// SNew(SProjectCookPage,CookModel)
						SNew(SCookersPage,*HotPatcherModelsMap.Find(TEXT("Cooker")))
					]

					+ SGridPanel::Slot(0, 1)
					.ColumnSpan(3)
					.Padding(0.0f, 16.0f)
					[
						SNew(SSeparator)
						.Orientation(Orient_Horizontal)
					]

					// Patch Version Control section
					+ SGridPanel::Slot(0, 2)
					.Padding(8.0f, 0.0f, 0.0f, 0.0f)
					.VAlign(VAlign_Top)
					[
						SNew(STextBlock)
						.Font(FCoreStyle::GetDefaultFontStyle("Patcher", 13))
					.Text(LOCTEXT("ProjectPatchSectionHeader", "Patcher"))
					]
					+ SGridPanel::Slot(1, 2)
					.Padding(32.0f, 0.0f, 8.0f, 0.0f)
					[
						SNew(SPatchersPage, *HotPatcherModelsMap.Find(TEXT("Patch")))
					]
				]
			]
		]
	];
}

TSharedPtr<SNotificationList> SHotPatcher::GetNotificationListPtr() const
{
	return NotificationListPtr;
}

#undef LOCTEXT_NAMESPACE