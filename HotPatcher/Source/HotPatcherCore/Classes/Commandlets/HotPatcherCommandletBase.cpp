#include "HotPatcherCommandletBase.h"
#include "CreatePatch/FExportReleaseSettings.h"
#include "CreatePatch/ReleaseProxy.h"
#include "CommandletHelper.h"

// engine header
#include "CoreMinimal.h"
#include "Misc/FileHelper.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY(LogHotPatcherCommandletBase);


static const bool bNoDDC = FParse::Param(FCommandLine::Get(), TEXT("NoDDC"));

struct FObjectTrackerTagCleaner:public FPackageTrackerBase
{
	virtual void NotifyUObjectCreated(const UObjectBase* Object, int32 Index) override
	{
		auto ObjectIns = const_cast<UObject*>(static_cast<const UObject*>(Object));
		if(ObjectIns && bNoDDC)
		{
			ObjectIns->ClearFlags(RF_NeedPostLoad);
			ObjectIns->ClearFlags(RF_NeedPostLoadSubobjects);
		}
	}
};

void UHotPatcherCommandletBase::MaybeMarkPackageAsAlreadyLoaded(UPackage* Package)
{
	Package->SetPackageFlags(PKG_ReloadingForCooker);
	Package->SetPackageFlags(PKG_FilterEditorOnly);
}

int32 UHotPatcherCommandletBase::Main(const FString& Params)
{
#if SUPPORT_NO_DDC
	// for Object Create Tracking,Optimize Asset searching, dont execute UObject::PostLoad
	if (bNoDDC)
	{
		ObjectTrackerTagCleaner = MakeShareable(new FObjectTrackerTagCleaner);
		FCoreUObjectDelegates::PackageCreatedForLoad.AddUObject(this,&UHotPatcherCommandletBase::MaybeMarkPackageAsAlreadyLoaded);
	}
#endif
	return 0;
}