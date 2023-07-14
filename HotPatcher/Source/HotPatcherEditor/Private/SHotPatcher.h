#pragma once

#include "HotPatcherEditor.h"
#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "SVersionUpdater/SVersionUpdaterWidget.h"
#include "Widgets/Layout/SGridPanel.h"

class SHotPatcher : public SCompoundWidget
{

public:
	SLATE_BEGIN_ARGS(SHotPatcher)
	{}
	SLATE_END_ARGS()

public:
	/**
	* Construct the widget
	*
	* @param	InArgs			A declaration from which to construct the widget
	*/
	void Construct(const FArguments& InArgs,const FSHotPatcherContext& Context);
	TSharedPtr<SNotificationList> GetNotificationListPtr()const;


private:
	/** The list of active system messages */
	TSharedPtr<SNotificationList> NotificationListPtr;
	// TArray<FHotPatcherCategory> CategoryPages;
	TSharedPtr<SVersionUpdaterWidget> VersionUpdaterWidget;
	TSharedPtr<SGridPanel> ActionPageGridPanel;
};

