#include "HotPatcherActionManager.h"
#include "CreatePatch/SHotPatcherPatchWidget.h"
#include "CreatePatch/SHotPatcherReleaseWidget.h"
#include "ShaderPatch/SShaderPatchWidget.h"
#include "GameFeature/SGameFeaturePackageWidget.h"
#include "Cooker/OriginalCooker/SOriginalCookWidget.h"
#include "SVersionUpdater/FVersionUpdaterManager.h"

#define LOCTEXT_NAMESPACE "FHotPatcherActionManager"

void FHotPatcherActionManager::Init()
{
	FVersionUpdaterManager::Get().RequestRemoveVersion(REMOTE_VERSION_FILE);
	SetupDefaultActions();
}

void FHotPatcherActionManager::Shutdown()
{
	
}

void FHotPatcherActionManager::RegisterCategory(const FHotPatcherCategory& Category)
{
	HotPatcherCategorys.Emplace(Category.CategoryName,Category);
}

void FHotPatcherActionManager::RegisteHotPatcherAction(const FString& Category, const FString& ActionName,
                                                       const FHotPatcherAction& Action)
{
	TMap<FString,FHotPatcherAction>& CategoryIns = HotPatcherActions.FindOrAdd(Category);
	CategoryIns.Add(ActionName,Action);
	CategoryIns.ValueSort([](const FHotPatcherAction& R,const FHotPatcherAction& L)->bool
			{
				return R.Priority > L.Priority;
			});
	
	OnHotPatcherActionRegisted.Broadcast(Category,ActionName,Action);
}
void FHotPatcherActionManager::UnRegisteHotPatcherAction(const FString& Category, const FString& ActionName)
{
	TMap<FString,FHotPatcherAction>& CategoryIns = HotPatcherActions.FindOrAdd(Category);
	FHotPatcherAction Action;
	if(CategoryIns.Contains(ActionName))
	{
		Action = *CategoryIns.Find(ActionName);
		CategoryIns.Remove(ActionName);
	}
	OnHotPatcherActionUnRegisted.Broadcast(Category,ActionName,Action);
}

FHotPatcherAction* FHotPatcherActionManager::GetTopActionByCategory(const FString CategoryName)
{
	FHotPatcherAction* result = nullptr;
	TMap<FString,FHotPatcherAction>* CategoryIns = HotPatcherActions.Find(CategoryName);
	if(CategoryIns)
	{
		for(auto& Pair:*CategoryIns)
		{
			if(result)
			{
				break;
			}
			result = &Pair.Value;
		}
	}
	return result;
}

bool FHotPatcherActionManager::IsActiveAction(FString ActionName)
{
	if(FVersionUpdaterManager::Get().IsRequestFinished())
	{
		bool bActiveInRemote = false;
		auto HotPatcherRemoteVersion = FVersionUpdaterManager::Get().GetRemoteVersionByName(TEXT("HotPatcher"));
		if(HotPatcherRemoteVersion)
		{
			for(auto ActionCategory:HotPatcherRemoteVersion->ActiveActions)
			{
				if(ActionCategory.Value.Contains(*ActionName))
				{
					bActiveInRemote = true;
					break;
				}
			}
		}
		return bActiveInRemote;
	}
	return false;
}

void FHotPatcherActionManager::SetupDefaultActions()
{
	TArray<FHotPatcherAction> DefaultActions;
	DefaultActions.Emplace(
		TEXT("Patcher"),LOCTEXT("ByExportRelease", "ByRelease"), LOCTEXT("ExportReleaseActionHint", "Export Release ALL Asset Dependencies."), FSlateIcon(),nullptr,
		CREATE_ACTION_WIDGET_LAMBDA(SHotPatcherReleaseWidget,TEXT("ByRelease")),
		0
		);
	DefaultActions.Emplace(
		TEXT("Patcher"),LOCTEXT("ByCreatePatch", "ByPatch"), LOCTEXT("CreatePatchActionHint", "Create an Patch form Release version."), FSlateIcon(),nullptr,
		CREATE_ACTION_WIDGET_LAMBDA(SHotPatcherPatchWidget,TEXT("ByPatch")),
		1
		);
	DefaultActions.Emplace(
		TEXT("Patcher"),LOCTEXT("ByShaderPatch", "ByShaderPatch"), LOCTEXT("CreateShaderPatchActionHint", "Create an Shader code Patch form Metadata."), FSlateIcon(),nullptr,
		CREATE_ACTION_WIDGET_LAMBDA(SShaderPatchWidget,TEXT("ByShaderPatch")),
		-1
		);
#if ENGINE_GAME_FEATURE
	DefaultActions.Emplace(
		TEXT("Patcher"),LOCTEXT("ByGameFeature", "ByGameFeature"), LOCTEXT("CreateGameFeatureActionHint", "Create an Game Feature Package."), FSlateIcon(),nullptr,
		CREATE_ACTION_WIDGET_LAMBDA(SGameFeaturePackageWidget,TEXT("ByGameFeature")),
		0
		);
#endif

	DefaultActions.Emplace(
		TEXT("Cooker"),LOCTEXT("ByOriginal", "ByOriginal"),LOCTEXT("OriginalCookerActionHint", "Use single-process Cook Content(UE Default)"),FSlateIcon(),nullptr,
		CREATE_ACTION_WIDGET_LAMBDA(SOriginalCookWidget,TEXT("ByOriginal")),
		0
		);
	for(const auto& Action:DefaultActions)
	{
		RegisteHotPatcherAction(Action.Category.ToString(),Action.ActionName.Get().ToString(),Action);
	}
}



#undef LOCTEXT_NAMESPACE
