#pragma once

#include "HotPatcherEditor.h"
#include "CoreMinimal.h"
#include "IDetailsView.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Input/Reply.h"
#include "Widgets/SCompoundWidget.h"
#include "Model/FHotPatcherCookModel.h"
#include "Model/FHotPatcherCreatePatchModel.h"
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
	void Construct(const FArguments& InArgs);
	TSharedPtr<SNotificationList> GetNotificationListPtr()const;


private:
	/** The list of active system messages */
	TSharedPtr<SNotificationList> NotificationListPtr;

	TSharedPtr<FHotPatcherCookModel> CookModel;
	TSharedPtr<FHotPatcherCreatePatchModel> CreatePatchModel;
};