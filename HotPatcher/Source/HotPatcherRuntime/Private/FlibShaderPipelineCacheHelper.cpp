// Fill out your copyright notice in the Description page of Project Settings.


#include "FlibShaderPipelineCacheHelper.h"
#include "FlibPatchParserHelper.h"
#include "HotPatcherLog.h"
#include "ShaderPipelineCache.h"
#include "RHIShaderFormatDefinitions.inl"
#include "HAL/IConsoleManager.h"

bool UFlibShaderPipelineCacheHelper::LoadShaderPipelineCache(const FString& Name)
{
	UE_LOG(LogHotPatcher,Display,TEXT("Load Shader pipeline cache %s for platform %d"),*Name,*ShaderPlatformToShaderFormatName(GMaxRHIShaderPlatform).ToString());
	return FShaderPipelineCache::OpenPipelineFileCache(Name,GMaxRHIShaderPlatform);
}

bool UFlibShaderPipelineCacheHelper::EnableShaderPipelineCache(bool bEnable)
{
	UE_LOG(LogHotPatcher,Display,TEXT("EnableShaderPipelineCache %s"),bEnable?TEXT("true"):TEXT("false"));
	auto Var =  IConsoleManager::Get().FindConsoleVariable(TEXT("r.ShaderPipelineCache.Enabled"));
	if(Var)
	{
		Var->Set( bEnable ? 1 : 0);
	}
	return !!Var;
}
