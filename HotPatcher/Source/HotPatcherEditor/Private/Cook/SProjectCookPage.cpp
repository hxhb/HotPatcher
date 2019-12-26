// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "SProjectCookPage.h"
#include "SHotPatcherCookedPlatforms.h"
#include "SHotPatcherCookSetting.h"
#include "SHotPatcherCookMaps.h"
#include "FlibPatchParserHelper.h"
#include "ThreadUtils/FProcWorkerThread.hpp"

// Engine Header
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Async.h"


#define LOCTEXT_NAMESPACE "SProjectCookPage"


/* SProjectCookPage interface
 *****************************************************************************/

void SProjectCookPage::Construct(const FArguments& InArgs, TSharedPtr<FHotPatcherCookModel> InCookModel)
{
	mCookModel = InCookModel;

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
				.Text(LOCTEXT("WhichProjectToUseText", "How would you like to cook the content?"))
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0, 8.0, 0.0, 0.0)
		[
			SNew(SExpandableArea)
			.AreaTitle(LOCTEXT("CookPlatforms", "Platforms"))
			.InitiallyCollapsed(true)
			.Padding(8.0)
			.BodyContent()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SHotPatcherCookedPlatforms,mCookModel)
				]
			]
			
		]
		+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0, 8.0, 0.0, 0.0)
			[
				SNew(SExpandableArea)
				.AreaTitle(LOCTEXT("CookMaps", "Map(s)"))
				.InitiallyCollapsed(true)
				.Padding(8.0)
				.BodyContent()
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SHotPatcherCookMaps, mCookModel)
					]
				]

			]
		+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0, 8.0, 0.0, 0.0)
			[
				SNew(SExpandableArea)
				.AreaTitle(LOCTEXT("CookSetting", "Settings"))
				.InitiallyCollapsed(true)
				.Padding(8.0)
				.BodyContent()
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SHotPatcherCookSetting, mCookModel)
					]
				]

			]
		+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0, 8.0, 0.0, 0.0)

		+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Right)
			.Padding(4, 4, 10, 4)
			[
				SNew(SButton)
				.Text(LOCTEXT("RunCook", "Cook Content"))
				.OnClicked(this,&SProjectCookPage::RunCook)
				.IsEnabled(this,&SProjectCookPage::CanExecuteCook)
			]
	];
}

bool SProjectCookPage::CanExecuteCook()const
{
	if (!(mCookModel->GetAllSelectedPlatform().Num() > 0))
	{
		return false;
	}
	if (!(mCookModel->GetAllSelectedCookMap().Num() > 0) && 
		!mCookModel->GetAllSelectedSettings().Contains(TEXT("CookAll")))
	{
		return false;
	}
	return true;
}

FReply SProjectCookPage::RunCook()const
{
	FString ProjectFilePath;
	FString FinalCookCommand;
	{
		FString ProjectPath = UKismetSystemLibrary::GetProjectDirectory();
		FString ProjectName = FString(FApp::GetProjectName()).Append(TEXT(".uproject"));
		ProjectFilePath = FPaths::Combine(ProjectPath, ProjectName);
	}
	FString EngineBin = UFlibPatchParserHelper::GetUE4CmdBinary();
	
	if (FPaths::FileExists(ProjectFilePath))
	{
		FinalCookCommand = TEXT("\"") + ProjectFilePath + TEXT("\"");
		FinalCookCommand.Append(TEXT(" -run=cook "));

		FString CookParams;
		FString ErrorMsg;
		if (mCookModel->CombineAllCookParams(CookParams, ErrorMsg))
		{
			FinalCookCommand.Append(CookParams);
		}
		RunCookProc(EngineBin, FinalCookCommand);
		
	}
	return FReply::Handled();
}


void SProjectCookPage::ReceiveOutputMsg(const FString& InMsg)
{
	UE_LOG(LogTemp, Log, TEXT("%s"), *InMsg);
}


void SProjectCookPage::SpawnRuningCookNotification()
{
	if (PendingProgressPtr.IsValid())
	{
		PendingProgressPtr.Pin()->ExpireAndFadeout();
	}
	FNotificationInfo Info(LOCTEXT("CookNotificationInProgress", "Cook Operation In Progress"));

	Info.bFireAndForget = false;

	PendingProgressPtr = mCookModel->NotificationListPtr->AddNotification(Info);
	PendingProgressPtr.Pin()->SetCompletionState(SNotificationItem::CS_Pending);

}


void SProjectCookPage::SpawnEndCookNotification()
{
	TSharedPtr<SNotificationItem> NotificationItem = PendingProgressPtr.Pin();

	if (NotificationItem.IsValid())
	{
		NotificationItem->SetText(LOCTEXT("CookNotification", "Cook Operation Finished!"));
		NotificationItem->SetCompletionState(SNotificationItem::CS_Success);
		NotificationItem->ExpireAndFadeout();

		PendingProgressPtr.Reset();
	}
}

void SProjectCookPage::RunCookProc(const FString& InBinPath, const FString& InCommand)const
{
	mCookProcWorkingThread = MakeShareable(new FProcWorkerThread(TEXT("CookThread"),InBinPath,InCommand));
	mCookProcWorkingThread->OutputMsgDelegate.AddStatic(&SProjectCookPage::ReceiveOutputMsg);
	//mCookProcWorkingThread->BeginDelegate.AddRaw(this,&SProjectCookPage::SpawnRuningCookNotification);
	//mCookProcWorkingThread->EndDelegate.AddRaw(this,&SProjectCookPage::SpawnEndCookNotification);
	mCookProcWorkingThread->Execute();
}
#undef LOCTEXT_NAMESPACE
