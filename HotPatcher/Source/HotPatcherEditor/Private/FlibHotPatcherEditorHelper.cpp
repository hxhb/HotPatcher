// Fill out your copyright notice in the Description page of Project Settings.


#include "FlibHotPatcherEditorHelper.h"
#include "Misc/SecureHash.h"

TArray<FString> UFlibHotPatcherEditorHelper::GetAllCookOption()
{
	TArray<FString> result
	{
		"Iterate",
		"UnVersioned",
		"CookAll",
		"Compressed"
	};
	return result;
}

void UFlibHotPatcherEditorHelper::CreateSaveFileNotify(const FText& InMsg, const FString& InSavedFile)
{
	auto Message = InMsg;
	FNotificationInfo Info(Message);
	Info.bFireAndForget = true;
	Info.ExpireDuration = 5.0f;
	Info.bUseSuccessFailIcons = false;
	Info.bUseLargeFont = false;

	const FString HyperLinkText = InSavedFile;
	Info.Hyperlink = FSimpleDelegate::CreateStatic(
		[](FString SourceFilePath)
		{
			FPlatformProcess::ExploreFolder(*SourceFilePath);
		},
		HyperLinkText
		);
	Info.HyperlinkText = FText::FromString(HyperLinkText);

	FSlateNotificationManager::Get().AddNotification(Info)->SetCompletionState(SNotificationItem::CS_Success);
}

bool UFlibHotPatcherEditorHelper::SerializeExAssetFileInfoToJsonObject(const FExternAssetFileInfo& InExFileInfo, TSharedPtr<FJsonObject>& OutJsonObject)
{
	bool bRunStatus = false;

	if (!OutJsonObject.IsValid())
	{
		OutJsonObject = MakeShareable(new FJsonObject);
	}
	FString FileAbsPath = FPaths::ConvertRelativePathToFull(InExFileInfo.FilePath.FilePath);
	FMD5Hash FileMD5Hash = FMD5Hash::HashFile(*FileAbsPath);
	OutJsonObject->SetStringField(TEXT("FilePath"), FileAbsPath);
	OutJsonObject->SetStringField(TEXT("MD5Hash"), LexToString(FileMD5Hash));
	OutJsonObject->SetStringField(TEXT("MountPath"), InExFileInfo.MountPath);

	bRunStatus = true;

	return bRunStatus;
}
