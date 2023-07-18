// Fill out your copyright notice in the Description page of Project Settings.


#include "FlibShaderPipelineCacheHelper.h"
#include "FlibPatchParserHelper.h"
#include "HotPatcherLog.h"
#include "ShaderPipelineCache.h"
#include "RHIShaderFormatDefinitions.inl"
#include "HAL/IConsoleManager.h"
#include "Misc/EngineVersionComparison.h"

bool UFlibShaderPipelineCacheHelper::LoadShaderPipelineCache(const FString& Name)
{
#if UE_VERSION_OLDER_THAN(5,1,0)
	return FShaderPipelineCache::OpenPipelineFileCache(Name,GMaxRHIShaderPlatform);
#else
	return FShaderPipelineCache::OpenPipelineFileCache(GMaxRHIShaderPlatform);
#endif
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
#if UE_VERSION_OLDER_THAN(5,1,0)
	return FShaderPipelineCache::SavePipelineFileCache((FPipelineFileCache::SaveMode)Mode);
#else
	return FShaderPipelineCache::SavePipelineFileCache((FPipelineFileCacheManager::SaveMode)Mode);
#endif
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
		ret = !!Var->GetInt();
	}
	return ret;
}

bool UFlibShaderPipelineCacheHelper::IsEnabledLogPSO()
{
	bool ret = false;
	auto Var =  IConsoleManager::Get().FindConsoleVariable(TEXT("r.ShaderPipelineCache.LogPSO"));
	if(Var)
	{
		ret = !!Var->GetInt();
	}
	return ret;
}

bool UFlibShaderPipelineCacheHelper::IsEnabledSaveBoundPSOLog()
{
	bool ret = false;
	auto Var =  IConsoleManager::Get().FindConsoleVariable(TEXT("r.ShaderPipelineCache.SaveBoundPSOLog"));
	if(Var)
	{
		ret = !!Var->GetInt();
	}
	return ret;
}
