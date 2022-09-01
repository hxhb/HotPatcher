#pragma once

#include "FCountServerlessWrapper.h"
#include "Commandlets/Commandlet.h"
#include "HotPatcherCommandletBase.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogHotPatcherCommandletBase, All, All);

UCLASS()
class HOTPATCHERCORE_API UHotPatcherCommandletBase :public UCommandlet
{
	GENERATED_BODY()

public:
	virtual int32 Main(const FString& Params)override;
	virtual bool IsSkipObject(UObject* Object){ return false; }
	virtual bool IsSkipPackage(UPackage* Package){ return false; }
protected:
	void Update(const FString& Params);
	void MaybeMarkPackageAsAlreadyLoaded(UPackage* Package);
	TSharedPtr<struct FObjectTrackerTagCleaner> ObjectTrackerTagCleaner;
	FString CmdConfigPath;
	TSharedPtr<FCountServerlessWrapper> Counter;
};