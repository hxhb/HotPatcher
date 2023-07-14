#include "SHotPatcher.h"
#include "CreatePatch/SPatchersPage.h"
#include "Cooker/SCookersPage.h"
#include "SVersionUpdater/SVersionUpdaterWidget.h"
// engine header
#include "HotPatcherCore.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "HAL/PlatformFile.h"
#include "Json.h"
#include "Model/FCookersModeContext.h"

#define LOCTEXT_NAMESPACE "SHotPatcher"

void SHotPatcher::Construct(const FArguments& InArgs,const FSHotPatcherContext& Context)
{
	TMap<FString,FHotPatcherCategory> HotPatcherCategorys;
	
	HotPatcherCategorys.Add(TEXT("Cooker"),FHotPatcherCategory(TEXT("Cooker"),SNew(SCookersPage, MakeShareable(new FCookersModeContext))));
	HotPatcherCategorys.Add(TEXT("Patcher"),FHotPatcherCategory(TEXT("Patcher"),SNew(SPatchersPage, MakeShareable(new FPatchersModeContext))));
	HotPatcherCategorys.Append(FHotPatcherActionManager::Get().GetHotPatcherCategorys());
	float HotPatcherVersion = FHotPatcherActionManager::Get().GetHotPatcherVersion();
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
					.ToolName(FText::FromString(GToolName))
					.ToolVersion(HotPatcherVersion)
					.DeveloperName(FText::FromString(TEXT("lipengzha")))
					.DeveloperWebsite(FText::FromString(TEXT("https://imzlp.com")))
					.UpdateWebsite(FText::FromString(TEXT("https://imzlp.com/posts/17590/")))
					.CurrentVersion(GToolMainVersion)
					.PatchVersion(GToolPatchVersion)
				]
#endif
				+SVerticalBox::Slot()
				.Padding(0,10,0,0)
				.AutoHeight()
				[
					SAssignNew(ActionPageGridPanel,SGridPanel)
					.FillColumn(1, 1.0f)
				]
			]
		]
	];

	int32 RowNum = 0;
	for(auto ActionPage:HotPatcherCategorys)
	{
		FString ActionCategoryName = ActionPage.Key;
		if(!Context.Category.IsEmpty() && !Context.Category.Equals(ActionCategoryName))
		{
			continue;
		}
		if(TMap<FString,FHotPatcherAction>* ActionCategory = FHotPatcherActionManager::Get().GetHotPatcherActions().Find(ActionCategoryName))
		{
			bool bHasActiveActions = false;
			for(auto Action:*ActionCategory)
			{
				bHasActiveActions = FHotPatcherActionManager::Get().IsActiveAction(Action.Key);
				if(bHasActiveActions)
				{
					break;
				}
			}
			if(bHasActiveActions)
			{
				ActionPageGridPanel->AddSlot(0,RowNum)
				.Padding(8.0f, 10.0f, 0.0f, 0.0f)
				.VAlign(VAlign_Top)
				[
					SNew(STextBlock)
					.Font(FCoreStyle::GetDefaultFontStyle(*ActionCategoryName, 13))
					.Text(UKismetTextLibrary::Conv_StringToText(ActionCategoryName))
				];
		
				ActionPageGridPanel->AddSlot(1, RowNum)
				.Padding(25.0f, 0.0f, 8.0f, 0.0f)
				[
					ActionPage.Value.Widget
				];
				++RowNum;
				if(RowNum < (HotPatcherCategorys.Num()*2-1))
				{
					ActionPageGridPanel->AddSlot(0, RowNum)
					.ColumnSpan(3)
					.Padding(0.0f, 16.0f)
					[
						SNew(SSeparator)
						.Orientation(Orient_Horizontal)
					];
					++RowNum;
				}
			}
		}
	}
	if(!Context.Category.IsEmpty() && !Context.ActionName.IsEmpty())
	{
		HotPatcherCategorys.Find(Context.Category)->Widget->SelectToAction(Context.ActionName);
	}
}

TSharedPtr<SNotificationList> SHotPatcher::GetNotificationListPtr() const
{
	return NotificationListPtr;
}

#undef LOCTEXT_NAMESPACE