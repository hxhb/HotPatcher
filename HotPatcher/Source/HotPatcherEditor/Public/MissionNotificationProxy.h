// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "MissionNotificationProxy.generated.h"


DECLARE_MULTICAST_DELEGATE(FMissionCanceled);
/**
 * 
 */
UCLASS()
class HOTPATCHEREDITOR_API UMissionNotificationProxy : public UObject
{
	GENERATED_UCLASS_BODY()
public:

	virtual void ReceiveOutputMsg(const FString& InMsg);
	virtual void SpawnRuningMissionNotification();
	virtual void SpawnMissionSuccessedNotification();
	virtual void SpawnMissionFaildNotification();
	virtual void CancelMission();

	virtual void SetMissionName(FName NewMissionName);
	virtual void SetMissionNotifyText(const FText& RunningText,const FText& CancelText,const FText& SuccessedText,const FText& FaildText);
	FMissionCanceled MissionCanceled;
protected:
	TWeakPtr<SNotificationItem> PendingProgressPtr;
	bool bRunning = false;
	FText RunningNotifyText;
	FText RunningNofityCancelText;
	FText MissionSuccessedNotifyText;
	FText MissionFailedNotifyText;
	FName MissionName;
};
