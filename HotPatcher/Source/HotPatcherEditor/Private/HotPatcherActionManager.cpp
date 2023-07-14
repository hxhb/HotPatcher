#include "HotPatcherActionManager.h"
#include "HotPatcherCore.h"
#include "HotPatcherEditor.h"
#include "Kismet/KismetTextLibrary.h"
#include "CreatePatch/SHotPatcherPatchWidget.h"
#include "CreatePatch/SHotPatcherReleaseWidget.h"
#include "Cooker/OriginalCooker/SOriginalCookWidget.h"
#include "SVersionUpdater/FVersionUpdaterManager.h"

#define LOCTEXT_NAMESPACE "FHotPatcherActionManager"

FHotPatcherActionManager FHotPatcherActionManager::Manager;

void FHotPatcherActionManager::Init()
{
	FVersionUpdaterManager::Get().ModCurrentVersionGetter = [this](const FString& ActionName)->float
	{
		float CurrentVersion = 0.f;
		FHotPatcherModDesc ModDesc;
		if(GetModDescByName(ActionName,ModDesc))
		{
			CurrentVersion = ModDesc.CurrentVersion;
		}
		return CurrentVersion;
	};
	
	FVersionUpdaterManager::Get().ModIsActivteCallback = [this](const FString& ModName)->bool
	{
		return IsActiviteMod(*ModName);
	};
	auto HotPatcherModDescMap2ChildModDesc = [](const TMap<FName,FHotPatcherModDesc>& HotPatcherModDescMap)->TArray<FChildModDesc>
	{
		TArray<FChildModDesc> result;
		for(const auto& HotPatcherMod:HotPatcherModDescMap)
		{
			FChildModDesc ChildModDesc;
			ChildModDesc.ModName = HotPatcherMod.Value.ModName;
			ChildModDesc.CurrentVersion = HotPatcherMod.Value.CurrentVersion;
			ChildModDesc.MinToolVersion = HotPatcherMod.Value.MinHotPatcherVersion;
			ChildModDesc.bIsBuiltInMod = HotPatcherMod.Value.bIsBuiltInMod;
			ChildModDesc.Description = HotPatcherMod.Value.Description;
			ChildModDesc.URL = HotPatcherMod.Value.URL;
			ChildModDesc.UpdateURL = HotPatcherMod.Value.UpdateURL;
			result.Add(ChildModDesc);
		}
		return result;
	};
	
	FVersionUpdaterManager::Get().RequestLocalRegistedMods = [this,HotPatcherModDescMap2ChildModDesc]()->TArray<FChildModDesc>
	{
		return HotPatcherModDescMap2ChildModDesc(ModsDesc);
	};
	FVersionUpdaterManager::Get().RequestUnsupportLocalMods = [this,HotPatcherModDescMap2ChildModDesc]()->TArray<FChildModDesc>
	{
		return HotPatcherModDescMap2ChildModDesc(UnSupportModsDesc);
	};
	
	SetupDefaultActions();
	
	FVersionUpdaterManager::Get().RequestRemoveVersion(GRemoteVersionFile);
}

void FHotPatcherActionManager::Shutdown()
{
	FVersionUpdaterManager::Get().Reset();
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

	FHotPatcherActionDesc Desc;
	Desc.Category = Category;
	Desc.ActionName = ActionName;
	Desc.ToolTip = Action.ActionToolTip.Get().ToString();
	Desc.Priority = Action.Priority;
	Desc.RequestWidgetPtr = Action.RequestWidget;
	
	ActionsDesc.Add(ActionName,Desc);
	OnHotPatcherActionRegisted.Broadcast(Category,ActionName,Action);
}

void FHotPatcherActionManager::RegisteHotPatcherAction(const FHotPatcherActionDesc& NewAction)
{
	THotPatcherTemplateHelper::AppendEnumeraters<EHotPatcherCookActionMode>(TArray<FString>{NewAction.ActionName});
	FHotPatcherAction NewPatcherAction
	(
		*NewAction.ActionName,
		*NewAction.ModName,
	UKismetTextLibrary::Conv_StringToText(NewAction.ActionName),
	UKismetTextLibrary::Conv_StringToText(NewAction.ToolTip),
			FSlateIcon(),
	nullptr,
	NewAction.RequestWidgetPtr,
	NewAction.Priority
	);
	
	FHotPatcherActionManager::Get().RegisteHotPatcherAction(NewAction.Category,NewAction.ActionName,NewPatcherAction);
}

void FHotPatcherActionManager::RegisteHotPatcherMod(const FHotPatcherModDesc& ModDesc)
{
	if(ModDesc.MinHotPatcherVersion > GetHotPatcherVersion())
	{
		UE_LOG(LogHotPatcherEdotor,Warning,TEXT("%s Min Support Version is %.1f,Current HotPatcher Version is %.1f"),*ModDesc.ModName,ModDesc.MinHotPatcherVersion,GetHotPatcherVersion());
		UnSupportModsDesc.Add(*ModDesc.ModName,ModDesc);
	}
	else
	{
		for(const auto& ActionDesc:ModDesc.ModActions)
		{
			FHotPatcherActionManager::Get().RegisteHotPatcherAction(ActionDesc);
		}
		ModsDesc.Add(*ModDesc.ModName,ModDesc);
	}
}

void FHotPatcherActionManager::UnRegisteHotPatcherMod(const FHotPatcherModDesc& ModDesc)
{
	for(const auto& ActionDesc:ModDesc.ModActions)
	{
		FHotPatcherActionManager::Get().UnRegisteHotPatcherAction(ActionDesc.Category,ActionDesc.ActionName);
	}
	ModsDesc.Remove(*ModDesc.ModName);
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
	ActionsDesc.Remove(*ActionName);
	OnHotPatcherActionUnRegisted.Broadcast(Category,ActionName,Action);
}

float FHotPatcherActionManager::GetHotPatcherVersion() const
{
	FString Version = FString::Printf(
		TEXT("%d.%d"),
		FHotPatcherCoreModule::Get().GetMainVersion(),
		FHotPatcherCoreModule::Get().GetPatchVersion()
		);
	return UKismetStringLibrary::Conv_StringToFloat(Version);
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

bool FHotPatcherActionManager::IsContainAction(const FString& CategoryName, const FString& ActionName)
{
	bool bContain = false;
	if(HotPatcherActions.Contains(CategoryName))
	{
		bContain = HotPatcherActions.Find(CategoryName)->Contains(ActionName);
	}
	return bContain;
}

bool FHotPatcherActionManager::IsActiviteMod(const FString& ModName)
{
	return ModsDesc.Contains(*ModName);
}

bool FHotPatcherActionManager::IsSupportEditorAction(FString ActionName)
{
	FHotPatcherActionDesc Desc;
	bool bDescStatus = GetActionDescByName(ActionName,Desc);
	
	return IsActiveAction(ActionName) && (bDescStatus && Desc.RequestWidgetPtr);
}

bool FHotPatcherActionManager::IsActiveAction(FString ActionName)
{
	if(FVersionUpdaterManager::Get().IsRequestFinished())
	{
		bool bActiveInRemote = false;
		FRemteVersionDescrible* HotPatcherRemoteVersion = FVersionUpdaterManager::Get().GetRemoteVersionByName(TEXT("HotPatcher"));
		if(HotPatcherRemoteVersion)
		{
			if(HotPatcherRemoteVersion->b3rdMods)
			{
				bActiveInRemote = true;
			}
			else
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
		}
		return bActiveInRemote;
	}
	return true;
}

bool FHotPatcherActionManager::GetActionDescByName(const FString& ActionName, FHotPatcherActionDesc& Desc)
{
	bool bStatus = ActionsDesc.Contains(ActionName);
	if(bStatus)
	{
		Desc = *ActionsDesc.Find(ActionName);
	}
	return bStatus;
};

bool FHotPatcherActionManager::GetModDescByName(const FString& ModName, FHotPatcherModDesc& ModDesc)
{
	bool bStatus = ModsDesc.Contains(*ModName);
	if(bStatus)
	{
		ModDesc = *ModsDesc.Find(*ModName);
	}
	return bStatus;
}
#define HOTPATCHEER_CORE_MODENAME TEXT("HotPatcherCore")
#define CMDHANDLER_MODENAME TEXT("CmdHandler")

void FHotPatcherActionManager::SetupDefaultActions()
{
	TArray<FHotPatcherModDesc> BuiltInMods;
	
	FHotPatcherModDesc HotPatcherCoreMod;
	HotPatcherCoreMod.ModName = HOTPATCHEER_CORE_MODENAME;
	HotPatcherCoreMod.bIsBuiltInMod = true;
	HotPatcherCoreMod.CurrentVersion = 1.0;
	HotPatcherCoreMod.Description = TEXT("Unreal Engine Asset Manage and Package Solution.");
	HotPatcherCoreMod.URL = TEXT("https://imzlp.com/posts/17590/");
	HotPatcherCoreMod.UpdateURL = TEXT("https://github.com/hxhb/HotPatcher");
	
	HotPatcherCoreMod.ModActions.Emplace(
		TEXT("Patcher"),HOTPATCHEER_CORE_MODENAME,TEXT("ByRelease"), TEXT("Export Release ALL Asset Dependencies."),
		CREATE_ACTION_WIDGET_LAMBDA(SHotPatcherReleaseWidget,TEXT("ByRelease")),
		0
		);
	HotPatcherCoreMod.ModActions.Emplace(
		TEXT("Patcher"),HOTPATCHEER_CORE_MODENAME,TEXT("ByPatch"), TEXT("Create an Patch form Release version."), 
		CREATE_ACTION_WIDGET_LAMBDA(SHotPatcherPatchWidget,TEXT("ByPatch")),
		1
		);
	
#if ENABLE_ORIGINAL_COOKER
	HotPatcherCoreMod.ModActions.Emplace(
		TEXT("Cooker"),TEXT("HotPatcherCore"),HOTPATCHEER_CORE_MODENAME,TEXT("Use single-process Cook Content(UE Default)"),
		CREATE_ACTION_WIDGET_LAMBDA(SOriginalCookWidget,TEXT("ByOriginal")),
		0
		);
#endif
	BuiltInMods.Add(HotPatcherCoreMod);
	
	for(const auto& Mod:BuiltInMods)
	{
		RegisteHotPatcherMod(Mod);
	}
}

#undef LOCTEXT_NAMESPACE
