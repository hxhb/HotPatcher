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

bool UFlibShaderPipelineCacheHelper::SavePipelineFileCache(EPSOSaveMode Mode)
{
	return FShaderPipelineCache::SavePipelineFileCache((FPipelineFileCache::SaveMode)Mode);
}

bool UFlibShaderPipelineCacheHelper::EnableLogPSO(bool bEnable)
{
	UE_LOG(LogHotPatcher,Display,TEXT("EnableLogPSO %s"),bEnable?TEXT("true"):TEXT("false"));
	auto Var =  IConsoleManager::Get().FindConsoleVariable(TEXT("r.ShaderPipelineCache.LogPSO"));
	if(Var)
	{
		Var->Set( bEnable ? 1 : 0);
	}
	return !!Var;
}

bool UFlibShaderPipelineCacheHelper::EnableSaveBoundPSOLog(bool bEnable)
{
	UE_LOG(LogHotPatcher,Display,TEXT("EnableSaveBoundPSOLog %s"),bEnable?TEXT("true"):TEXT("false"));
	auto Var =  IConsoleManager::Get().FindConsoleVariable(TEXT("r.ShaderPipelineCache.SaveBoundPSOLog"));
	if(Var)
	{
		Var->Set( bEnable ? 1 : 0);
	}
	return !!Var;
}

bool UFlibShaderPipelineCacheHelper::IsEnabledUsePSO()
{
	bool ret = false;
	auto Var =  IConsoleManager::Get().FindConsoleVariable(TEXT("r.ShaderPipelineCache.Enabled"));
	if(Var)
	{
		ret = Var->GetBool();
	}
	return ret;
}

bool UFlibShaderPipelineCacheHelper::IsEnabledLogPSO()
{
	bool ret = false;
	auto Var =  IConsoleManager::Get().FindConsoleVariable(TEXT("r.ShaderPipelineCache.LogPSO"));
	if(Var)
	{
		ret = Var->GetBool();
	}
	return ret;
}

bool UFlibShaderPipelineCacheHelper::IsEnabledSaveBoundPSOLog()
{
	bool ret = false;
	auto Var =  IConsoleManager::Get().FindConsoleVariable(TEXT("r.ShaderPipelineCache.SaveBoundPSOLog"));
	if(Var)
	{
		ret = Var->GetBool();
	}
	return ret;
}
