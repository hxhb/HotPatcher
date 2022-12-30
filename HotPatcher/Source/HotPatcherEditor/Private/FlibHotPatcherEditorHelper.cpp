#include "FlibHotPatcherEditorHelper.h"

#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"
#include "Async/Async.h"

void UFlibHotPatcherEditorHelper::CreateSaveFileNotify(const FText& InMsg, const FString& InSavedFile,
                                                       SNotificationItem::ECompletionState NotifyType)
{
	AsyncTask(ENamedThreads::GameThread,[InMsg,InSavedFile,NotifyType]()
	{
		auto Message = InMsg;
		FNotificationInfo Info(Message);
		Info.bFireAndForget = true;
		Info.ExpireDuration = 5.0f;
		Info.bUseSuccessFailIcons = false;
		Info.bUseLargeFont = false;

		const FString HyperLinkText = InSavedFile;
		Info.Hyperlink = FSimpleDelegate::CreateLambda(
			[](FString SourceFilePath)
			{
				FPlatformProcess::ExploreFolder(*SourceFilePath);
			},
			HyperLinkText
		);
		Info.HyperlinkText = FText::FromString(HyperLinkText);
		FSlateNotificationManager::Get().AddNotification(Info)->SetCompletionState(NotifyType);
	});
}

TArray<FString> UFlibHotPatcherEditorHelper::SaveFileDialog(const FString& DialogTitle, const FString& DefaultPath,
	const FString& DefaultFile, const FString& FileTypes, uint32 Flags)
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();

	TArray<FString> SaveFilenames;
	if (DesktopPlatform)
	{
		const bool bOpened = DesktopPlatform->SaveFileDialog(
			nullptr,
			DialogTitle,
			DefaultPath,
			DefaultFile,
			FileTypes,
			Flags,
			SaveFilenames
		);
	}
	return SaveFilenames;
}

TArray<FString> UFlibHotPatcherEditorHelper::OpenFileDialog(const FString& DialogTitle, const FString& DefaultPath,
	const FString& DefaultFile, const FString& FileTypes, uint32 Flags)
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	TArray<FString> SelectedFiles;
	
	if (DesktopPlatform)
	{
		const bool bOpened = DesktopPlatform->OpenFileDialog(
			nullptr,
			DialogTitle,
			DefaultPath,
			DefaultFile,
			FileTypes,
			Flags,
			SelectedFiles
		);
	}
	return SelectedFiles;
}
