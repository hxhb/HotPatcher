#include "SHotPatcher.h"
#include "Styling/SlateTypes.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SCheckBox.h"
#include "EditorStyleSet.h"
#include "PropertyEditorModule.h"
#include "Modules/ModuleManager.h"
#include "IDetailsView.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "AssetRegistryModule.h"
#include "ARFilter.h"
#include "ModuleManager.h"
#include "ISettingsModule.h"
#include "PlatformFile.h"
#include "PlatformFilemanager.h"
#include "FileHelper.h"
#include "json.h"
#include "Misc/SecureHash.h"
#include "FileManager.h"
#include "PackageName.h"

#define LOCTEXT_NAMESPACE "FExportPakModule"

void SHotPatcher::Construct(const FArguments& InArgs)
{
	CreateTargetAssetListView();

	ChildSlot
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			.Padding(2, 0, 0, 0)
			[
				SNew(SBorder)
				.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
				.Padding(0.f)
				[
					SNew(SVerticalBox)

					+ SVerticalBox::Slot()
					.FillHeight(1.0f)
					.Padding(4, 4, 4, 4)
					[
						SNew(SScrollBox)
						+ SScrollBox::Slot()
						[
							SNew(SVerticalBox)

							+ SVerticalBox::Slot()
							.Padding(0, 10, 0, 0)
							[
								SNew(SBorder)
								.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
								[
									SNew(SVerticalBox)
									// Static mesh component selection
									+ SVerticalBox::Slot()
									.Padding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
									[
										SNew(SHorizontalBox)
										+ SHorizontalBox::Slot()
										.VAlign(VAlign_Center)
										[
											SettingsView->AsShared()
										]
									]
								]
							]
						]
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Right)
					.Padding(4, 4, 10, 4)
					[
						SNew(SButton)
						.Text(LOCTEXT("HotPatcher", "Export Pak file(s)"))
						.OnClicked(this, &SHotPatcher::OnHotPatcherButtonClicked)
						.IsEnabled(this, &SHotPatcher::CanHotPatcherExecuted)
					]
				]
			]
		];

}


void SHotPatcher::CreateTargetAssetListView()
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

	SettingsView = EditModule.CreateDetailView(DetailsViewArgs);;
}

FReply SHotPatcher::OnHotPatcherButtonClicked()
{
	return FReply::Handled();
}
bool SHotPatcher::CanHotPatcherExecuted() const
{
	return false;
}


#undef LOCTEXT_NAMESPACE