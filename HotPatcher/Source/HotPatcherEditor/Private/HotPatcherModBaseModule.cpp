#include "HotPatcherModBaseModule.h"

void FHotPatcherModBaseModule::StartupModule()
{
	FHotPatcherActionManager::Get().RegisteHotPatcherMod(GetModDesc());
}

void FHotPatcherModBaseModule::ShutdownModule()
{
	FHotPatcherActionManager::Get().UnRegisteHotPatcherMod(GetModDesc());
}
