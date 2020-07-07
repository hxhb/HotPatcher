#pragma once

#include "Interfaces/ITargetPlatformManagerModule.h"
#include "Interfaces/ITargetPlatform.h"
#include "Model/FHotPatcherCreatePatchModel.h"
#include "ExportPatchSettings.h"
#include "SHotPatcherInformations.h"
#include "SHotPatcherPatchableBase.h"
#include "FPatchVersionDiff.h"

// engine header
#include "Interfaces/ITargetPlatform.h"
#include "Templates/SharedPointer.h"
#include "IDetailsView.h"
#include "PropertyEditorModule.h"
#include "Widgets/Text/SMultiLineEditableText.h"

#include "PatcherProxy.generated.h"

DECLARE_MULTICAST_DELEGATE_TwoParams(FExportPakProcess,const FString&,const FString&);


UCLASS()
class UPatcherProxy:public UObject
{
    GENERATED_BODY()
public:

    bool CheckSelectedAssetsCookStatus(const TArray<FString>& PlatformNames, const FAssetDependenciesInfo& SelectedAssets, FString& OutMsg) const;
    bool CheckPatchRequire(const FPatchVersionDiff& InDiff, FString& OutMsg) const;
    bool SavePatchVersionJson(const FHotPatcherVersion& InSaveVersion, const FString& InSavePath, FPakVersion& OutPakVersion);
    bool SavePatchDiffJson(const FHotPatcherVersion& InSaveVersion, const FPatchVersionDiff& InDiff);
    FProcHandle DoUnrealPak(TArray<FString> UnrealPakOptions, bool block);
    bool SavePakCommands(const FString& InPlatformName, const FPatchVersionDiff& InDiffInfo, const FString& InSavePath);
    FHotPatcherVersion MakeNewRelease(const FHotPatcherVersion& InBaseVersion, const FHotPatcherVersion& InCurrentVersion) const;
    bool CanExportPatch() const;
    bool DoExportPatch();

	FORCEINLINE void SetExportPatchSetting(UExportPatchSettings* InExportPatchSetting)
	{
		ExportPatchSetting = InExportPatchSetting;
	}
public:
    FExportPakProcess OnPaking;
private:
    UExportPatchSettings* ExportPatchSetting;
};
