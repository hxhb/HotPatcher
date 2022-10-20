#pragma once 

#include "CoreMinimal.h"

#include "Templates/SharedPointer.h"
#include "IDetailsView.h"
#include "PropertyEditorModule.h"
#include "Widgets/Text/SMultiLineEditableText.h"


class SHotPatcherInformations
	: public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SHotPatcherInformations) { }
	SLATE_END_ARGS()

public:

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs The Slate argument list.
	 */
	void Construct(	const FArguments& InArgs);

public:

	void SetExpanded(bool InExpand);

	void SetContent(const FString& InContent);

private:

	TSharedPtr<SExpandableArea> DiffAreaWidget;
	TSharedPtr<SMultiLineEditableText> MulitText;
};