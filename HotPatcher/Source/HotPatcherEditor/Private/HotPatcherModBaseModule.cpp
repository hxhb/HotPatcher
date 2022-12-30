#include "HotPatcherModBaseModule.h"

void FHotPatcherModBaseModule::StartupModule()
{
	for(const auto& ActionDesc:GetModActions())
	{
		RegistedActions.Add(ActionDesc);
		FHotPatcherActionManager::Get().RegisteHotPatcherAction(ActionDesc);
	}
}

void FHotPatcherModBaseModule::ShutdownModule()
{
	for(const auto& ActionDesc:RegistedActions)
	{
		FHotPatcherActionManager::Get().UnRegisteHotPatcherAction(ActionDesc.Category,ActionDesc.ActionName);
	}
}
