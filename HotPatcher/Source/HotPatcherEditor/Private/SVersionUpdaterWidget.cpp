#include "SVersionUpdaterWidget.h"

// engine header
#include "Widgets/Input/SHyperlink.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "EditorStyleSet.h"
#include "Misc/FileHelper.h"
#include "Json.h"

#define LOCTEXT_NAMESPACE "VersionUpdaterWidget"

void SVersionUpdaterWidget::Construct(const FArguments& InArgs)
{
	SetToolUpdateInfo(
		InArgs._ToolName.Get().ToString(),
		InArgs._DeveloperName.Get().ToString(),
		InArgs._DeveloperWebsite.Get().ToString(),
		InArgs._UpdateWebsite.Get().ToString()
		);
	CurrentVersion = InArgs._CurrentVersion.Get();
	ChildSlot
	[
		SNew(SBorder)
		.Padding(2)
		.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			.Padding(4.0f, 0.0f, 4.0f, 0.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SBox)
					.WidthOverride(40)
					.HeightOverride(40)
					[
						SNew(SImage)
						.Image(FEditorStyle::GetBrush("LauncherCommand.QuickLaunch"))
					]
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(10,0,10,0)
				.VAlign(VAlign_Center)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(2, 4, 2, 4)
					[
						SNew(STextBlock)
						.Text_Raw(this,&SVersionUpdaterWidget::GetToolName)
					]
					+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(2, 4, 2, 4)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.Padding(0,0,4,0)
							[
								SNew(STextBlock)
								.Text_Raw(this,&SVersionUpdaterWidget::GetCurrentVersionText)
							]
							+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								SNew(SHyperlink)
								.Text(LOCTEXT("ToolsUpdaterInfo", "A new version is avaliable"))
								.OnNavigate(this, &SVersionUpdaterWidget::HyLinkClickEventOpenUpdateWebsite)
							]
							
						]
				]
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			[
				SNew(SOverlay)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(10,0,10,0)
			.VAlign(VAlign_Center)
			[
				SNew(SHyperlink)
				.Text_Raw(this,&SVersionUpdaterWidget::GetDeveloperDescrible)
				.OnNavigate(this, &SVersionUpdaterWidget::HyLinkClickEventOpenDeveloperWebsite)
			]
		]
	];
}

void SVersionUpdaterWidget::HyLinkClickEventOpenUpdateWebsite()
{
	FPlatformProcess::LaunchURL(*UpdateWebsite, NULL, NULL);
}

void SVersionUpdaterWidget::HyLinkClickEventOpenDeveloperWebsite()
{
	FPlatformProcess::LaunchURL(*DeveloperWebsite, NULL, NULL);
}

void SVersionUpdaterWidget::SetToolUpdateInfo(const FString& InToolName, const FString& InDeveloperName,
	const FString& InDeveloperWebsite, const FString& InUpdateWebsite)
{
	ToolName = InToolName;
	DeveloperName = InDeveloperName;
	DeveloperWebsite = InDeveloperWebsite;
	UpdateWebsite = InUpdateWebsite;
}

#undef LOCTEXT_NAMESPACE
