#pragma once

// engine header
#include "Templates/SharedPointer.h"
#include "Dom/JsonObject.h"
#include "CoreMinimal.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "FlibHotPatcherEditorHelper.generated.h"

UCLASS()
class HOTPATCHEREDITOR_API UFlibHotPatcherEditorHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	static void CreateSaveFileNotify(const FText& InMsg,const FString& InSavedFile,SNotificationItem::ECompletionState NotifyType = SNotificationItem::CS_Success);

};


