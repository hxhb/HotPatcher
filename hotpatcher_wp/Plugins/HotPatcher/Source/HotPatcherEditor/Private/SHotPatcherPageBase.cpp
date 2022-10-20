#include "SHotPatcherPageBase.h"

FReply SHotPatcherPageBase::DoImportConfig()const
{
	if (GetActiveAction().IsValid())
	{
		GetActiveAction()->ImportConfig();
	}
	return FReply::Handled();
}

FReply SHotPatcherPageBase::DoImportProjectConfig() const
{
	if (GetActiveAction().IsValid())
	{
		GetActiveAction()->ImportProjectConfig();
	}
	return FReply::Handled();
}

FReply SHotPatcherPageBase::DoExportConfig()const
{
	if (GetActiveAction().IsValid())
	{
		GetActiveAction()->ExportConfig();
	}
	return FReply::Handled();
}
FReply SHotPatcherPageBase::DoResetConfig()const
{
	if (GetActiveAction().IsValid())
	{
		GetActiveAction()->ResetConfig();
	}
	return FReply::Handled();
}

TSharedPtr<SHotPatcherWidgetInterface> SHotPatcherPageBase::GetActiveAction()const
{
	TSharedPtr<SHotPatcherWidgetInterface> result;
	if (GetContext().IsValid())
	{
		if(ActionWidgetMap.Contains(GetContext()->GetModeName()))
		{
			result = *ActionWidgetMap.Find(GetContext()->GetModeName());
		}
	}
	return result;
}
