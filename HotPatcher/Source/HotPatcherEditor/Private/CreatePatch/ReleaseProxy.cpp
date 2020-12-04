#include "CreatePatch/ReleaseProxy.h"
#include "CreatePatch/ScopedSlowTaskContext.h"
#include "FHotPatcherVersion.h"
#include "FlibPatchParserHelper.h"
#include "HotPatcherLog.h"

// engine header
#include "FlibHotPatcherEditorHelper.h"
#include "Misc/DateTime.h"
#include "CoreGlobals.h"
#include "Logging/LogMacros.h"


#define LOCTEXT_NAMESPACE "UExportRelease"


bool UReleaseProxy::DoExport()
{
    bool bRetStatus = false;

    float AmountOfWorkProgress = 4.0f;
	// FScopedSlowTask UnrealPakSlowTask(AmountOfWorkProgress);
	// UnrealPakSlowTask.MakeDialog();
	UScopedSlowTaskContext* UnrealPakSlowTask = NewObject<UScopedSlowTaskContext>();
	UnrealPakSlowTask->init(AmountOfWorkProgress);
	
	FHotPatcherVersion ExportVersion;
	{
    	FText DiaLogMsg = FText::Format(NSLOCTEXT("AnalysisRelease", "AnalysisReleaseVersionInfo", "Analysis Release {0} Assets info."), FText::FromString(GetSettingObject()->GetVersionId()));
    	UnrealPakSlowTask->EnterProgressFrame(1.0, DiaLogMsg);
    	ExportVersion = UFlibPatchParserHelper::ExportReleaseVersionInfo(
            GetSettingObject()->GetVersionId(),
            TEXT(""),
            FDateTime::UtcNow().ToString(),
            GetSettingObject()->GetAssetIncludeFiltersPaths(),
            GetSettingObject()->GetAssetIgnoreFiltersPaths(),
            GetSettingObject()->GetAssetRegistryDependencyTypes(),
            GetSettingObject()->GetSpecifyAssets(),
            // GetSettingObject()->GetAllExternFiles(true),
            GetSettingObject()->GetAddExternAssetsToPlatform(),
            GetSettingObject()->IsIncludeHasRefAssetsOnly(),
            GetSettingObject()->IsAnalysisFilterDependencies()
        );
	}

	FString SaveVersionDir = FPaths::Combine(GetSettingObject()->GetSavePath(), GetSettingObject()->GetVersionId());

	// save release asset info
	{
		FText DiaLogMsg = FText::Format(NSLOCTEXT("ExportReleaseJson", "ExportReleaseVersionInfoJson", "Export Release {0} Assets info to file."), FText::FromString(GetSettingObject()->GetVersionId()));
		UnrealPakSlowTask->EnterProgressFrame(1.0, DiaLogMsg);
		FString SaveToJson;
		if (UFlibPatchParserHelper::TSerializeStructAsJsonString(ExportVersion, SaveToJson))
		{

			FString SaveToFile = FPaths::Combine(
				SaveVersionDir,
				FString::Printf(TEXT("%s_Release.json"), *GetSettingObject()->GetVersionId())
			);
			bool runState = UFLibAssetManageHelperEx::SaveStringToFile(SaveToFile, SaveToJson);
			if (runState)
			{
				auto Message = LOCTEXT("ExportReleaseSuccessNotification", "Succeed to export HotPatcher Release Version.");
				if(IsRunningCommandlet())
				{
					OnShowMsg.Broadcast(Message.ToString());
				}
				else
				{
					UFlibHotPatcherEditorHelper::CreateSaveFileNotify(Message, SaveToFile);
				}
				
			}
			UE_LOG(LogHotPatcher, Log, TEXT("HotPatcher Export RELEASE is %s."), runState ? TEXT("Success") : TEXT("FAILD"));
		}
	}

	{
		FText DiaLogMsg = FText::Format(NSLOCTEXT("ExportReleaseConfig", "ExportReleaseConfigJson", "Export Release {0} Configuration to file."), FText::FromString(GetSettingObject()->GetVersionId()));
		UnrealPakSlowTask->EnterProgressFrame(1.0, DiaLogMsg);
		FString ConfigJson;
		if (UFlibPatchParserHelper::TSerializeStructAsJsonString(*GetSettingObject(),ConfigJson))
		{
			FString SaveToFile = FPaths::Combine(
				SaveVersionDir,
				FString::Printf(TEXT("%s_ReleaseConfig.json"), *GetSettingObject()->GetVersionId())
			);
			bool runState = UFLibAssetManageHelperEx::SaveStringToFile(SaveToFile, ConfigJson);
			if (runState)
			{
				auto Message = LOCTEXT("ExportReleaseConfigSuccessNotification", "Succeed to export HotPatcher Release Config.");
				if(IsRunningCommandlet())
				{
					OnShowMsg.Broadcast(Message.ToString());
				}
				else
				{
					UFlibHotPatcherEditorHelper::CreateSaveFileNotify(Message, SaveToFile);
				}
			}
			UE_LOG(LogHotPatcher, Log, TEXT("HotPatcher Export RELEASE CONFIG is %s."), runState ? TEXT("Success") : TEXT("FAILD"));
		}
	}

	// save asset dependency
	{
		FText DiaLogMsg = FText::Format(NSLOCTEXT("ExportReleaseAssetDependency", "ExportReleaseAssetDependencyJson", "Export Release {0} Asset Dependency to file."), FText::FromString(GetSettingObject()->GetVersionId()));
		UnrealPakSlowTask->EnterProgressFrame(1.0, DiaLogMsg);
		if (GetSettingObject()->IsSaveAssetRelatedInfo())
		{
			TArray<FHotPatcherAssetDependency> AssetsDependency = UFlibPatchParserHelper::GetAssetsRelatedInfoByFAssetDependencies(ExportVersion.AssetInfo,GetSettingObject()->GetAssetRegistryDependencyTypes());

			FString AssetsDependencyString = UFlibPatchParserHelper::SerializeAssetsDependencyAsJsonString(AssetsDependency);
			
			FString SaveAssetRelatedInfoToFile = FPaths::Combine(
				SaveVersionDir,
				FString::Printf(TEXT("%s_AssetRelatedInfos.json"), *ExportVersion.VersionId)
			);
			if (UFLibAssetManageHelperEx::SaveStringToFile(SaveAssetRelatedInfoToFile, AssetsDependencyString))
			{
				auto Message = LOCTEXT("SaveAssetRelatedInfoInfo", "Succeed to export Asset Related info.");
				if(IsRunningCommandlet())
				{
					OnShowMsg.Broadcast(Message.ToString());
				}
				else
				{
					UFlibHotPatcherEditorHelper::CreateSaveFileNotify(Message, SaveAssetRelatedInfoToFile);
				}
			}
		}
	}
	UnrealPakSlowTask->Final();
	return bRetStatus;
}

#undef LOCTEXT_NAMESPACE