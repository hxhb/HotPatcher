// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Model/FHotPatcherCookModel.h"
#include "ThreadUtils/FProcWorkerThread.hpp"

DECLARE_LOG_CATEGORY_EXTERN(LogCookPage, Log, All);

/**
 * Implements the profile page for the session launcher wizard.
 */
class SProjectCookPage
	: public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SProjectCookPage) { }
	SLATE_END_ARGS()

public:

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs The Slate argument list.
	 */
	void Construct(	const FArguments& InArgs,TSharedPtr<FHotPatcherCookModel> InCookModel);
	

protected:
	bool CanExecuteCook()const;
	void RunCookProc(const FString& InBinPath, const FString& InCommand)const;
	FReply RunCook()const;

protected:
	TArray<FString> GetDefaultCookParams()const;

	static void ReceiveOutputMsg(const FString& InMsg);
	void SpawnRuningCookNotification();
	void SpawnCookSuccessedNotification();
	void SpawnCookFaildNotification();
private:
	bool InCooking=false;
	/** The pending progress message */
	TWeakPtr<SNotificationItem> PendingProgressPtr;

	TSharedPtr<FHotPatcherCookModel> mCookModel;
	mutable TSharedPtr<FProcWorkerThread> mCookProcWorkingThread;
};
