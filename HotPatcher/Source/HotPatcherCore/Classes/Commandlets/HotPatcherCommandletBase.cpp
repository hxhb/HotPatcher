#include "HotPatcherCommandletBase.h"
#include "CreatePatch/FExportReleaseSettings.h"
#include "CreatePatch/ReleaseProxy.h"
#include "CommandletHelper.h"

// engine header
#include "CoreMinimal.h"
#include "HotPatcherCore.h"
#include "Misc/FileHelper.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"

#if PLATFORM_WINDOWS && WITH_EDITOR
#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows.h"
#include "Windows/HideWindowsPlatformTypes.h"
#endif

DEFINE_LOG_CATEGORY(LogHotPatcherCommandletBase);


static const bool bNoDDC = FParse::Param(FCommandLine::Get(), TEXT("NoDDC"));

struct FObjectTrackerTagCleaner:public FPackageTrackerBase
{
	FObjectTrackerTagCleaner(UHotPatcherCommandletBase* InCommandletIns):CommandletIns(InCommandletIns){}
	virtual void NotifyUObjectCreated(const UObjectBase* Object, int32 Index) override
	{
		auto ObjectIns = const_cast<UObject*>(static_cast<const UObject*>(Object));
		if((ObjectIns && bNoDDC) || CommandletIns->IsSkipObject(ObjectIns))
		{
			ObjectIns->ClearFlags(RF_NeedPostLoad);
			ObjectIns->ClearFlags(RF_NeedPostLoadSubobjects);
		}
	}
protected:
	UHotPatcherCommandletBase* CommandletIns;
};

FString UHotPatcherCommandletBase::GetCrashDir()
{
	return FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(),TEXT("Saved/HotPatcher/Crashs")));
}

void UHotPatcherCommandletBase::CleanCrashDir()
{
	if(FPaths::DirectoryExists(GetCrashDir()))
	{
		IFileManager::Get().DeleteDirectory(*GetCrashDir(),false,true);
	}
}

void UHotPatcherCommandletBase::OnHandleSystemError()
{
	FString CrashTagFile = FPaths::Combine(GetCrashDir(),FString::Printf(TEXT("%s.txt"),*GetCmdletName()));
	bool bSaveCrashTag = FFileHelper::SaveStringToFile(GetCmdletName(),*CrashTagFile);
	UE_LOG(LogHotPatcherCommandletBase,Display,TEXT("%s Catch SystemError,Save CrashTag to %s(%s)"),*GetCmdletName(),*CrashTagFile,bSaveCrashTag ? TEXT("TRUE"):TEXT("FALSE"));
}

void UHotPatcherCommandletBase::Update(const FString& Params)
{
	FString CommandletName;
	bool bIsCommandlet = FParse::Value(FCommandLine::Get(), TEXT("-run="), CommandletName);
	
	if(GetDefault<UHotPatcherSettings>()->bServerlessCounterInCmdlet)
	{
		Counter = MakeShareable(new FCountServerlessWrapper);
		FServerRequestInfo RequestInfo = FCountServerlessWrapper::MakeServerRequestInfo();
		auto ProjectInfo = FCountServerlessWrapper::MakeCurrentProject();
		ProjectInfo.PluginVersion = FString::Printf(TEXT("%d.%d"),GToolMainVersion,GToolPatchVersion);

		if(bIsCommandlet)
		{
			ProjectInfo.ProjectName = FString::Printf(TEXT("%s_%s"),*ProjectInfo.ProjectName,*CommandletName);
		}
		Counter->Init(RequestInfo,ProjectInfo);
		Counter->Processor();
	}
}

void UHotPatcherCommandletBase::MaybeMarkPackageAsAlreadyLoaded(UPackage* Package)
{
	if (bNoDDC || IsSkipPackage(Package))
	{
		Package->SetPackageFlags(PKG_ReloadingForCooker);
		Package->SetPackageFlags(PKG_FilterEditorOnly);
		Package->SetPackageFlags(PKG_EditorOnly);
	}
}

int32 UHotPatcherCommandletBase::Main(const FString& Params)
{
	Update(Params);
#if SUPPORT_NO_DDC
	// for Object Create Tracking,Optimize Asset searching, dont execute UObject::PostLoad
	ObjectTrackerTagCleaner = MakeShareable(new FObjectTrackerTagCleaner(this));
	FCoreUObjectDelegates::PackageCreatedForLoad.AddUObject(this,&UHotPatcherCommandletBase::MaybeMarkPackageAsAlreadyLoaded);
#endif

#if PLATFORM_WINDOWS && WITH_EDITOR
	{
		SetPriorityClass(GetCurrentProcess(),REALTIME_PRIORITY_CLASS);
		UE_LOG(LogHotPatcher,Display,TEXT("Set Commandlet Priority to REALTIME_PRIORITY_CLASS."));
	}
#endif
	
	FCoreDelegates::OnHandleSystemError.AddLambda([this](){ this->OnHandleSystemError(); });

	if(IsRunningCommandlet() && !FParse::Param(FCommandLine::Get(), TEXT("NoSearchAllAssets")))
	{
		SCOPED_NAMED_EVENT_TEXT("SearchAllAssets",FColor::Red);
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		AssetRegistryModule.Get().SearchAllAssets(true);
	}

	bool bStatus = FParse::Value(*Params, *FString(PATCHER_CONFIG_PARAM_NAME).ToLower(), CmdConfigPath);
	if (!bStatus)
	{
		UE_LOG(LogHotPatcherCommandletBase, Warning, TEXT("not -config=xxxx.json params."));
		return -1;
	}
	CmdConfigPath = UFlibPatchParserHelper::ReplaceMark(CmdConfigPath);
	if (bStatus && !FPaths::FileExists(CmdConfigPath))
	{
		UE_LOG(LogHotPatcherCommandletBase, Error, TEXT("cofnig file %s not exists."), *CmdConfigPath);
		return -1;
	}
	return 0;
}