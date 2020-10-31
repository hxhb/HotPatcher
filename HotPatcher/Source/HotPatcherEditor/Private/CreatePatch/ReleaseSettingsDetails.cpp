#include "CreatePatch/ReleaseSettingsDetails.h"
#include "CreatePatch/FExportReleaseSettings.h"

// engine header
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "Widgets/Input/SButton.h"

#define LOCTEXT_NAMESPACE "ReleaseSettingsDetails"

TSharedRef<IDetailCustomization> FReleaseSettingsDetails::MakeInstance()
{
    return MakeShareable(new FReleaseSettingsDetails());
}

void FReleaseSettingsDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
    TArray< TSharedPtr<FStructOnScope> > StructBeingCustomized;
    DetailBuilder.GetStructsBeingCustomized(StructBeingCustomized);
    check(StructBeingCustomized.Num() == 1);
    
    FExportReleaseSettings* ReleaseSettingsIns = (FExportReleaseSettings*)StructBeingCustomized[0].Get()->GetStructMemory();
    
    IDetailCategoryBuilder& VersionCategory = DetailBuilder.EditCategory("Version",FText::GetEmpty(),ECategoryPriority::Default);
    VersionCategory.SetShowAdvanced(true);
    
    VersionCategory.AddCustomRow(LOCTEXT("ImportPakLists", "Import Pak Lists"),true)
        .ValueContent()
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .Padding(0)
            .AutoWidth()
            [
                SNew(SButton)
                .Text(LOCTEXT("Import", "Import"))
                .ToolTipText(LOCTEXT("ImportPakLists_Tooltip", "Import Pak Lists"))
                .IsEnabled_Lambda([this,ReleaseSettingsIns]()->bool
                {
                    return ReleaseSettingsIns->IsByPakList();
                })
                .OnClicked_Lambda([this, ReleaseSettingsIns]()
                {
                    if (ReleaseSettingsIns)
                    {
                        ReleaseSettingsIns->ImportPakLists();
                    }
                    return(FReply::Handled());
                })
            ]
            + SHorizontalBox::Slot()
            .Padding(5,0,0,0)
            .AutoWidth()
            [
                SNew(SButton)
                .Text(LOCTEXT("Clear", "Clear"))
                .ToolTipText(LOCTEXT("ClearPakLists_Tooltip", "Clear Pak Lists"))
                .IsEnabled_Lambda([this,ReleaseSettingsIns]()->bool
                {
                    return ReleaseSettingsIns->IsByPakList();
                })
                .OnClicked_Lambda([this, ReleaseSettingsIns]()
                {
                    if (ReleaseSettingsIns)
                    {
                        ReleaseSettingsIns->ClearImportedPakList();
                    }
                    return(FReply::Handled());
                })
            ]
        ];
}
#undef LOCTEXT_NAMESPACE