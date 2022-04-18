#pragma once

#include "Commandlets/Commandlet.h"
#include "CommandletBase/HotPatcherCommandletBase.h"
#include "Cooker/MultiCooker/FSingleCookerSettings.h"
#include "HotSingleCookerCommandlet.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogHotSingleCookerCommandlet, All, All);

UCLASS()
class UHotSingleCookerCommandlet :public UHotPatcherCommandletBase
{
	GENERATED_BODY()

public:

	virtual int32 Main(const FString& Params)override;
	virtual bool IsSkipObject(UObject* Object) override;
	virtual bool IsSkipPackage(UPackage* Package) override;
protected:
	TSharedPtr<FSingleCookerSettings> ExportSingleCookerSetting;
	TArray<FName> CurrentMissionPackagePaths;
	TArray<FName> TotalPackageNames;
};