// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "SProjectCookPage.h"
#include "SHotPatcherCookedPlatforms.h"
#include "SHotPatcherCookSetting.h"
#include "SHotPatcherCookMaps.h"
#include "SProjectCookPage.h"
#include "FlibPatchParserHelper.h"
#include "ThreadUtils/FProcWorkerThread.hpp"

// Engine Header
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonReader.h"

#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Async/Async.h"


#define LOCTEXT_NAMESPACE "SProjectCookPage"

DEFINE_LOG_CATEGORY(LogCookPage);

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
			+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(8.0, 0.0, 0.0, 0.0)
				[
					SNew(SButton)
					.Text(LOCTEXT("ImportConfig", "Import"))
				.OnClicked(this, &SProjectCookPage::DoImportConfig)
				]
			+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(5.0, 0.0, 0.0, 0.0)
				[
					SNew(SButton)
					.Text(LOCTEXT("ExportConfig", "Export"))
					.OnClicked(this, &SProjectCookPage::DoExportConfig)
				]
			+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(5.0, 0.0, 0.0, 0.0)
				[
					SNew(SButton)
					.Text(LOCTEXT("ResetConfig", "Reset"))
				.OnClicked(this, &SProjectCookPage::DoResetConfig)
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
				SAssignNew(Platforms, SHotPatcherCookedPlatforms, mCookModel)
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
				SAssignNew(CookMaps, SHotPatcherCookMaps, mCookModel)
			]
			]

			]
		+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0, 8.0, 0.0, 0.0)
			[
				SNew(SExpandableArea)
				.AreaTitle(LOCTEXT("CookFilter", "Filter(s)"))
				.InitiallyCollapsed(true)
				.Padding(8.0)
				.BodyContent()
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SAssignNew(CookFilters, SHotPatcherCookSpecifyCookFilter, mCookModel)
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
				SAssignNew(CookSettings, SHotPatcherCookSetting, mCookModel)
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
			.OnClicked(this, &SProjectCookPage::RunCook)
			.IsEnabled(this, &SProjectCookPage::CanExecuteCook)
			]
	];
}
#include "DesktopPlatformModule.h"

namespace FPlatformUtils
{
	TArray<FString> OpenFileDialog()
	{
		IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
		TArray<FString> SelectedFiles;

		if (DesktopPlatform)
		{
			const bool bOpened = DesktopPlatform->OpenFileDialog(
				nullptr,
				LOCTEXT("OpenCookConfigDialog", "Open .json").ToString(),
				FString(TEXT("")),
				TEXT(""),
				TEXT("CookConfig json (*.json)|*.json"),
				EFileDialogFlags::None,
				SelectedFiles
			);
		}
		return SelectedFiles;
	}

	TArray<FString> SaveFileDialog()
	{
		IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();

		TArray<FString> SaveFilenames;
		if (DesktopPlatform)
		{

			const bool bOpened = DesktopPlatform->SaveFileDialog(
				nullptr,
				LOCTEXT("SvedCookConfig", "Save .json").ToString(),
				FString(TEXT("")),
				TEXT(""),
				TEXT("CookConfig json (*.json)|*.json"),
				EFileDialogFlags::None,
				SaveFilenames
			);
		}
		return SaveFilenames;
	}

	TSharedPtr<FJsonObject> DeserializeAsJsonObject(const FString& InContent)
	{
		TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(InContent);
		TSharedPtr<FJsonObject> JsonObject;
		FJsonSerializer::Deserialize(JsonReader, JsonObject);
		return JsonObject;
	}
}

#include "Misc/FileHelper.h"

FReply SProjectCookPage::DoImportConfig()
{
	TArray<FString> SelectedFiles = FPlatformUtils::OpenFileDialog();

	if (!!SelectedFiles.Num())
	{
		FString LoadFile = SelectedFiles[0];
		if (FPaths::FileExists(LoadFile))
		{
			FString ReadContent;
			if (FFileHelper::LoadFileToString(ReadContent, *LoadFile))
			{
				DeSerializeFromJsonObj(FPlatformUtils::DeserializeAsJsonObject(ReadContent));
			}
		}
		
	}
	return FReply::Handled();
}

FReply SProjectCookPage::DoExportConfig() const
{
	TArray<FString> SaveFiles = FPlatformUtils::SaveFileDialog();
	if (!!SaveFiles.Num())
	{
		FString SaveToFile = SaveFiles[0].EndsWith(TEXT(".json")) ? SaveFiles[0] : SaveFiles[0].Append(TEXT(".json"));

		FFileHelper::SaveStringToFile(SerializeAsString(), *SaveToFile);
	}

	return FReply::Handled();
}

FReply SProjectCookPage::DoResetConfig()
{
	Reset();
	return FReply::Handled();
}
bool SProjectCookPage::CanExecuteCook()const
{

	bool bCanCook = !!mCookModel->GetAllSelectedPlatform().Num() &&
					(
						!!mCookModel->GetAllSelectedCookMap().Num() ||
						!!mCookModel->GetAlwayCookFilters().Num() ||
						mCookModel->GetAllSelectedSettings().Contains(TEXT("CookAll"))
					);
	
	return !InCooking && bCanCook;
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
		else
		{
			UE_LOG(LogCookPage, Error, TEXT("The Cook Mission faild, %s"),*ErrorMsg);
			return FReply::Handled();
		}
		for (const auto& DefaultCookParam : GetDefaultCookParams())
		{
			if (!FinalCookCommand.Contains(DefaultCookParam))
			{
				FinalCookCommand.Append(TEXT(" ") + DefaultCookParam);
			}
		}
		UE_LOG(LogCookPage, Log, TEXT("The Cook Mission is Staring..."));
		UE_LOG(LogCookPage, Log, TEXT("CookCommand:%s %s"),*EngineBin,*FinalCookCommand);
		RunCookProc(EngineBin, FinalCookCommand);
		
	}
	return FReply::Handled();
}


TArray<FString> SProjectCookPage::GetDefaultCookParams() const
{
	return TArray<FString>{"-NoLogTimes", "-UTF8Output"};
}

void SProjectCookPage::ReceiveOutputMsg(const FString& InMsg)
{
	if (InMsg.Contains(TEXT("Error:")))
	{
		UE_LOG(LogCookPage, Error, TEXT("%s"), *InMsg);
	}
	else if (InMsg.Contains(TEXT("Warning:")))
	{
		UE_LOG(LogCookPage, Warning, TEXT("%s"), *InMsg);
	}
	else
	{
		UE_LOG(LogCookPage, Log, TEXT("%s"), *InMsg);
	}
}


void SProjectCookPage::SpawnRuningCookNotification()
{
	SProjectCookPage* ProjectCookPage=this;
	AsyncTask(ENamedThreads::GameThread, [ProjectCookPage]()
	{
		if (ProjectCookPage->PendingProgressPtr.IsValid())
		{
			ProjectCookPage->PendingProgressPtr.Pin()->ExpireAndFadeout();
		}
		FNotificationInfo Info(LOCTEXT("CookNotificationInProgress", "Cook in porogress"));

		Info.bFireAndForget = false;
		Info.ButtonDetails.Add(FNotificationButtonInfo(LOCTEXT("RunningCookNotificationCancelButton", "Cancel"), FText(),
			FSimpleDelegate::CreateLambda([ProjectCookPage]() {ProjectCookPage->CancelCookMission(); }),
			SNotificationItem::CS_Pending
		));

		ProjectCookPage->PendingProgressPtr = FSlateNotificationManager::Get().AddNotification(Info);

		ProjectCookPage->PendingProgressPtr.Pin()->SetCompletionState(SNotificationItem::CS_Pending);
		ProjectCookPage->InCooking = true;
	});

}


void SProjectCookPage::SpawnCookSuccessedNotification()
{
	SProjectCookPage* ProjectCookPage = this;
	AsyncTask(ENamedThreads::GameThread, [ProjectCookPage]() {
		TSharedPtr<SNotificationItem> NotificationItem = ProjectCookPage->PendingProgressPtr.Pin();

		if (NotificationItem.IsValid())
		{
			NotificationItem->SetText(LOCTEXT("CookSuccessedNotification", "Cook Finished!"));
			NotificationItem->SetCompletionState(SNotificationItem::CS_Success);
			NotificationItem->ExpireAndFadeout();

			ProjectCookPage->PendingProgressPtr.Reset();
		}

		ProjectCookPage->InCooking = false;
		UE_LOG(LogCookPage, Log, TEXT("The Cook Mission is Successfuly."))
	});
}


void SProjectCookPage::SpawnCookFaildNotification()
{
	SProjectCookPage* ProjectCookPage = this;
	AsyncTask(ENamedThreads::GameThread, [ProjectCookPage]() {
		TSharedPtr<SNotificationItem> NotificationItem = ProjectCookPage->PendingProgressPtr.Pin();

		if (NotificationItem.IsValid())
		{
			NotificationItem->SetText(LOCTEXT("CookFaildNotification", "Cook Faild!"));
			NotificationItem->SetCompletionState(SNotificationItem::CS_Fail);
			NotificationItem->ExpireAndFadeout();

			ProjectCookPage->PendingProgressPtr.Reset();
			ProjectCookPage->InCooking = false;
		}
		UE_LOG(LogCookPage, Error, TEXT("The Cook Mission is faild."))
	});
}

void SProjectCookPage::CancelCookMission()
{
	if (mCookProcWorkingThread.IsValid() && mCookProcWorkingThread->GetThreadStatus() == EThreadStatus::Busy)
	{
		mCookProcWorkingThread->Cancel();
	}
	UE_LOG(LogCookPage, Error, TEXT("The Cook Mission is canceled."));
}
void SProjectCookPage::RunCookProc(const FString& InBinPath, const FString& InCommand)const
{
	if (mCookProcWorkingThread.IsValid() && mCookProcWorkingThread->GetThreadStatus()==EThreadStatus::Busy)
	{
		mCookProcWorkingThread->Cancel();
	}
	else
	{
		mCookProcWorkingThread = MakeShareable(new FProcWorkerThread(TEXT("CookThread"), InBinPath, InCommand));
		mCookProcWorkingThread->ProcOutputMsgDelegate.AddStatic(&SProjectCookPage::ReceiveOutputMsg);
		mCookProcWorkingThread->ProcBeginDelegate.AddRaw(this, &SProjectCookPage::SpawnRuningCookNotification);
		mCookProcWorkingThread->ProcSuccessedDelegate.AddRaw(this, &SProjectCookPage::SpawnCookSuccessedNotification);
		mCookProcWorkingThread->ProcFaildDelegate.AddRaw(this, &SProjectCookPage::SpawnCookFaildNotification);
		mCookProcWorkingThread->Execute();
	}
}



TSharedPtr<FJsonObject> SProjectCookPage::SerializeAsJson() const
{
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	
	for (const auto& SerializableItem : GetSerializableItems())
	{
		JsonObject->SetObjectField(SerializableItem->GetSerializeName(), SerializableItem->SerializeAsJson());
	}
	return JsonObject;
}

void SProjectCookPage::DeSerializeFromJsonObj(TSharedPtr<FJsonObject>const & InJsonObject)
{
	for (const auto& SerializableItem : GetSerializableItems())
	{
		TSharedPtr<FJsonObject> ItemJsonObject = InJsonObject->GetObjectField(SerializableItem->GetSerializeName());
		SerializableItem->DeSerializeFromJsonObj(ItemJsonObject);
	}
}

FString SProjectCookPage::GetSerializeName()const
{
	return TEXT("CookPage");
}



FString SProjectCookPage::SerializeAsString()const
{
	FString result;
	auto JsonWriter = TJsonWriterFactory<>::Create(&result);
	FJsonSerializer::Serialize(SerializeAsJson().ToSharedRef(), JsonWriter);
	return result;
}

void SProjectCookPage::Reset()
{
	for (const auto& SerializableItem : GetSerializableItems())
	{
		SerializableItem->Reset();
	}
}
#undef LOCTEXT_NAMESPACE
