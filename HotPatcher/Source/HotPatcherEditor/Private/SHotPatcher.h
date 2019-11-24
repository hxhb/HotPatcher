#pragma once

#include "HotPatcherEditor.h"
#include "CoreMinimal.h"
#include "IDetailsView.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Input/Reply.h"
#include "Widgets/SCompoundWidget.h"

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

private:

	FReply OnHotPatcherButtonClicked();
	bool CanHotPatcherExecuted() const;
	void CreateTargetAssetListView();

private:
	/** Inline content area for different tool modes */
	TSharedPtr<SBox> InlineContentHolder;

	/** Settings view ui element ptr */
	TSharedPtr<IDetailsView> SettingsView;
};