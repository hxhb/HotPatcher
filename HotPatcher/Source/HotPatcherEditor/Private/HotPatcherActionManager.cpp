#include "HotPatcherActionManager.h"
#include "CreatePatch/SHotPatcherPatchWidget.h"
#include "CreatePatch/SHotPatcherReleaseWidget.h"
#include "ShaderPatch/SShaderPatchWidget.h"
#include "GameFeature/SGameFeaturePackageWidget.h"
#include "Cooker/OriginalCooker/SOriginalCookWidget.h"

#define LOCTEXT_NAMESPACE "FHotPatcherActionManager"

void FHotPatcherActionManager::Init()
{
	SetupDefaultActions();
}

void FHotPatcherActionManager::Shutdown()
{
	
}

void FHotPatcherActionManager::RegisteHotPatcherAction(const FString& Category, const FString& ActionName,
                                                        const FHotPatcherAction& Action)
{
	TMap<FString,FHotPatcherAction>& CategoryIns = HotPatcherActions.FindOrAdd(Category);
	CategoryIns.Add(ActionName,Action);
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

void FHotPatcherActionManager::SetupDefaultActions()
{
	TArray<FHotPatcherAction> DefaultActions;
	DefaultActions.Emplace(
		TEXT("Patcher"),LOCTEXT("ByExportRelease", "ByRelease"), LOCTEXT("ExportReleaseActionHint", "Export Release ALL Asset Dependencies."), FSlateIcon(),nullptr,
		CREATE_ACTION_WIDGET_LAMBDA(SHotPatcherReleaseWidget,TEXT("ByRelease"))
		);
	DefaultActions.Emplace(
		TEXT("Patcher"),LOCTEXT("ByCreatePatch", "ByPatch"), LOCTEXT("CreatePatchActionHint", "Create an Patch form Release version."), FSlateIcon(),nullptr,
		CREATE_ACTION_WIDGET_LAMBDA(SHotPatcherPatchWidget,TEXT("ByPatch"))
		);
	DefaultActions.Emplace(
		TEXT("Patcher"),LOCTEXT("ByShaderPatch", "ByShaderPatch"), LOCTEXT("CreateShaderPatchActionHint", "Create an Shader code Patch form Metadata."), FSlateIcon(),nullptr,
		CREATE_ACTION_WIDGET_LAMBDA(SShaderPatchWidget,TEXT("ByShaderPatch"))
		);
#if ENGINE_GAME_FEATURE
	DefaultActions.Emplace(
		TEXT("Patcher"),LOCTEXT("ByGameFeature", "ByGameFeature"), LOCTEXT("CreateGameFeatureActionHint", "Create an Game Feature Package."), FSlateIcon(),nullptr,
		CREATE_ACTION_WIDGET_LAMBDA(SGameFeaturePackageWidget,TEXT("ByGameFeature"))
		);
#endif

	DefaultActions.Emplace(
		TEXT("Cooker"),LOCTEXT("ByOriginal", "ByOriginal"),LOCTEXT("OriginalCookerActionHint", "Use single-process Cook Content(UE Default)"),FSlateIcon(),nullptr,
		CREATE_ACTION_WIDGET_LAMBDA(SOriginalCookWidget,TEXT("ByOriginal"))
		);
	for(const auto& Action:DefaultActions)
	{
		RegisteHotPatcherAction(Action.Category.ToString(),Action.ActionName.Get().ToString(),Action);
	}
}

#undef LOCTEXT_NAMESPACE