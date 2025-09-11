#pragma once

#include "HotPatcherCommandletBase.h"
#include "Commandlets/Commandlet.h"
#include "HotAssetScannerCommandlet.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogHotAssetScannerCommandlet, All, All);

UCLASS()
class UHotAssetScannerCommandlet :public UHotPatcherCommandletBase
{
	GENERATED_BODY()

public:
	virtual int32 Main(const FString& Params)override;
	virtual FString GetCmdletName()const override{ return TEXT("AssetScannerCmdlet"); }
};