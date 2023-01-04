// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CreatePatch/HotPatcherProxyBase.h"
#include "ShaderPatch/FExportShaderPatchSettings.h"
#include "ShaderPatchProxy.generated.h"

/**
 * 
 */
UCLASS()
class SHADERPATCHEREDITOR_API UShaderPatchProxy : public UHotPatcherProxyBase
{
	GENERATED_BODY()
public:
	virtual bool DoExport() override;
	FORCEINLINE bool IsRunningCommandlet()const{return ::IsRunningCommandlet();}
	FORCEINLINE virtual FExportShaderPatchSettings* GetSettingObject()override
	{
		return (FExportShaderPatchSettings*)Setting;
	}
};
