#pragma once

#include "CoreMinimal.h"
#include "GameDelegates.h"

DECLARE_MULTICAST_DELEGATE_TwoParams(FNotificationEvent,FText,const FString&)

class HOTPATCHERCORE_API FHotPatcherDelegates
{
public:
	/** Return a single FGameDelegates object */
	static FHotPatcherDelegates& Get();
	
	DEFINE_GAME_DELEGATE_TYPED(NotifyFileGenerated,FNotificationEvent);
};