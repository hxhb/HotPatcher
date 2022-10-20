// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "SOriginalCookWidget.h"
#include "SHotPatcherCookedPlatforms.h"
#include "SHotPatcherCookSetting.h"
#include "SHotPatcherCookMaps.h"
#include "SOriginalCookWidget.h"
#include "FlibPatchParserHelper.h"
#include "ThreadUtils/FProcWorkerThread.hpp"
#include "FlibHotPatcherCoreHelper.h"

// Engine Header
#include "FlibHotPatcherEditorHelper.h"
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

void SOriginalCookWidget::Construct(const FArguments& InArgs, TSharedPtr<FHotPatcherContextBase> InContext)
{
	OriginalCookerContext = MakeShareable(new FOriginalCookerContext);
	SetContext(OriginalCookerContext);
	
	MissionNotifyProay = NewObject<UMissionNotificationProxy>();
	MissionNotifyProay->AddToRoot();
	MissionNotifyProay->SetMissionName(TEXT("Cook"));
	MissionNotifyProay->SetMissionNotifyText(
		LOCTEXT("CookNotificationInProgress", "Cook in progress"),
		LOCTEXT("RunningCookNotificationCancelButton", "Cancel"),
		LOCTEXT("CookSuccessedNotification", "Cook Finished!"),
		LOCTEXT("CookFaildNotification", "Cook Faild!")
	);
	
	ChildSlot
	[
		SNew(SVerticalBox)
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
					SAssignNew(Platforms, SHotPatcherCookedPlatforms, OriginalCookerContext)
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
						SAssignNew(CookMaps, SHotPatcherCookMaps, OriginalCookerContext)
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
						SAssignNew(CookFilters, SHotPatcherCookSpecifyCookFilter, OriginalCookerContext)
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
				SAssignNew(CookSettings, SHotPatcherCookSetting, OriginalCookerContext)
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
			.OnClicked(this, &SOriginalCookWidget::RunCook)
			.IsEnabled(this, &SOriginalCookWidget::CanExecuteCook)
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

FReply SOriginalCookWidget::DoImportConfig()
{
	ImportConfig();
	return FReply::Handled();
}

FReply SOriginalCookWidget::DoExportConfig() const
{
	ExportConfig();


	return FReply::Handled();
}

FReply SOriginalCookWidget::DoResetConfig()
{
	ResetConfig();
	return FReply::Handled();
}
bool SOriginalCookWidget::CanExecuteCook()const
{
	bool bCanCook = !!GetCookerContextPtr()->GetAllSelectedPlatform().Num() &&
					(
						!!GetCookerContextPtr()->GetAllSelectedCookMap().Num() ||
						!!GetCookerContextPtr()->GetAlwayCookFilters().Num() ||
						GetCookerContextPtr()->GetAllSelectedSettings().Contains(TEXT("CookAll"))
					);
	
	return !InCooking && bCanCook;
}

FReply SOriginalCookWidget::RunCook()const
{
	FCookerConfig CookConfig = GetCookerContextPtr()->GetCookConfig();

	if (FPaths::FileExists(CookConfig.EngineBin) && FPaths::FileExists(CookConfig.ProjectPath))
	{
		FString CookCommand;
		UFlibPatchParserHelper::GetCookProcCommandParams(CookConfig, CookCommand);
		UE_LOG(LogCookPage, Log, TEXT("The Cook Mission is Staring..."));
		UE_LOG(LogCookPage, Log, TEXT("CookCommand:%s %s"),*CookConfig.EngineBin,*CookCommand);
		RunCookProc(CookConfig.EngineBin, CookCommand);	
	}
	return FReply::Handled();
}

void SOriginalCookWidget::CancelCookMission()
{
	if (mCookProcWorkingThread.IsValid() && mCookProcWorkingThread->GetThreadStatus() == EThreadStatus::Busy)
	{
		mCookProcWorkingThread->Cancel();
	}
	UE_LOG(LogCookPage, Error, TEXT("The Cook Mission is canceled."));
}
void SOriginalCookWidget::RunCookProc(const FString& InBinPath, const FString& InCommand)const
{
	if (mCookProcWorkingThread.IsValid() && mCookProcWorkingThread->GetThreadStatus()==EThreadStatus::Busy)
	{
		mCookProcWorkingThread->Cancel();
	}
	else
	{
		mCookProcWorkingThread = MakeShareable(new FProcWorkerThread(TEXT("CookThread"), InBinPath, InCommand));
		mCookProcWorkingThread->ProcOutputMsgDelegate.BindUObject(MissionNotifyProay,&UMissionNotificationProxy::ReceiveOutputMsg);
		mCookProcWorkingThread->ProcBeginDelegate.AddUObject(MissionNotifyProay,&UMissionNotificationProxy::SpawnRuningMissionNotification);
		mCookProcWorkingThread->ProcSuccessedDelegate.AddUObject(MissionNotifyProay,&UMissionNotificationProxy::SpawnMissionSuccessedNotification);
		mCookProcWorkingThread->ProcFaildDelegate.AddUObject(MissionNotifyProay,&UMissionNotificationProxy::SpawnMissionFaildNotification);
		MissionNotifyProay->MissionCanceled.AddRaw(const_cast<SOriginalCookWidget*>(this),&SOriginalCookWidget::CancelCookMission);
		mCookProcWorkingThread->Execute();
	}
}

TSharedPtr<FJsonObject> SOriginalCookWidget::SerializeAsJson() const
{
	FCookerConfig CookConfig = GetCookerContextPtr()->GetCookConfig();
	TSharedPtr<FJsonObject> CookConfigJsonObj;
	THotPatcherTemplateHelper::TSerializeStructAsJsonObject(CookConfig,CookConfigJsonObj);
	return CookConfigJsonObj;
}

void SOriginalCookWidget::DeSerializeFromJsonObj(TSharedPtr<FJsonObject>const & InJsonObject)
{
	for (const auto& SerializableItem : GetSerializableItems())
	{
		// TSharedPtr<FJsonObject> ItemJsonObject = InJsonObject->GetObjectField(SerializableItem->GetSerializeName());
		SerializableItem->DeSerializeFromJsonObj(InJsonObject);
	}
}

FString SOriginalCookWidget::GetSerializeName()const
{
	return TEXT("CookPage");
}

FString SOriginalCookWidget::SerializeAsString()const
{
	FString result;
	auto JsonWriter = TJsonWriterFactory<>::Create(&result);
	FJsonSerializer::Serialize(SerializeAsJson().ToSharedRef(), JsonWriter);
	return result;
}

void SOriginalCookWidget::Reset()
{
	for (const auto& SerializableItem : GetSerializableItems())
	{
		SerializableItem->Reset();
	}
}

void SOriginalCookWidget::ImportConfig()
{
	SHotPatcherCookerBase::ImportConfig();
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
}

void SOriginalCookWidget::ExportConfig() const
{
	SHotPatcherCookerBase::ExportConfig();
	TArray<FString> SaveFiles = FPlatformUtils::SaveFileDialog();
	if (!!SaveFiles.Num())
	{
		FString SaveToFile = SaveFiles[0].EndsWith(TEXT(".json")) ? SaveFiles[0] : SaveFiles[0].Append(TEXT(".json"));

		if (FFileHelper::SaveStringToFile(SerializeAsString(), *SaveToFile))
		{
			FText Msg = LOCTEXT("SavedCookerConfig", "Successd to Export the Cooker Config.");
			UFlibHotPatcherEditorHelper::CreateSaveFileNotify(Msg, SaveToFile);
		}
	}
}

void SOriginalCookWidget::ResetConfig()
{
	for (const auto& SerializableItem : GetSerializableItems())
	{
		SerializableItem->Reset();
	}
	SHotPatcherCookerBase::ResetConfig();
}
#undef LOCTEXT_NAMESPACE
