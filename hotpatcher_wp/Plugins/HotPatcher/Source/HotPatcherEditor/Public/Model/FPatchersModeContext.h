#pragma once
#include "Templates/HotPatcherTemplateHelper.hpp"
#include "CoreMinimal.h"
#include "FHotPatcherContextBase.h"
#include "FPatchersModeContext.generated.h"

UENUM(BlueprintType)
enum class EHotPatcherActionModes:uint8
{
	ByPatch,
	ByRelease,
	ByShaderPatch,
	ByGameFeature,
	Count UMETA(Hidden)
};
ENUM_RANGE_BY_COUNT(EHotPatcherActionModes, EHotPatcherActionModes::Count);


struct HOTPATCHEREDITOR_API FPatchersModeContext: public FHotPatcherContextBase
{
public:
	virtual ~FPatchersModeContext(){};
	virtual FName GetContextName()const override{ return TEXT("Patch"); }
};

#if ENGINE_MAJOR_VERSION <= 4 && ENGINE_MINOR_VERSION <= 21
namespace THotPatcherTemplateHelper
{
	template<>
	std::string GetCPPTypeName<EHotPatcherActionModes>()
	{
		return std::string("EHotPatcherActionModes");	
	};
}
#endif