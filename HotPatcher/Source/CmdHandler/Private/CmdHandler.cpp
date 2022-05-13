// Copyright 2019 Lipeng Zha, Inc. All Rights Reserved.

#include "CmdHandler.h"

DEFINE_LOG_CATEGORY(LogCmdHandler);

bool OverrideConfigValue(const FString& FileName,const FString& Section,const FString& Key,int32 NewValue)
{
	bool bRet = false;
	int32 DefaultValue;
	if(GConfig->GetInt( *Section, *Key, DefaultValue, FileName ))
	{
		GConfig->SetInt(  *Section, *Key, NewValue, FileName );
		UE_LOG(LogCmdHandler, Display, TEXT("Override section: %s key: %s from %d to %d."), *Section,*Key,DefaultValue,NewValue);
		bRet = true;
	}
	else
	{
		UE_LOG(LogCmdHandler, Warning, TEXT("section: %s key: %s is not found!"), *Section,*Key);
	}
	return bRet;
}

void FCmdHandlerModule::StartupModule()
{
	int32 OverrideNumUnusedShaderCompilingThreads;
	if(FParse::Value(FCommandLine::Get(), TEXT("-MaxShaderWorker="), OverrideNumUnusedShaderCompilingThreads))
	{
		OverrideNumUnusedShaderCompilingThreads = FMath::Max(OverrideNumUnusedShaderCompilingThreads,3);
		const int32 NumVirtualCores = FPlatformMisc::NumberOfCoresIncludingHyperthreads();
		OverrideNumUnusedShaderCompilingThreads = NumVirtualCores - OverrideNumUnusedShaderCompilingThreads;
		GConfig->SetInt( TEXT("DevOptions.Shaders"), TEXT("NumUnusedShaderCompilingThreads"), OverrideNumUnusedShaderCompilingThreads, GEngineIni);
		OverrideConfigValue(GEngineIni,TEXT("DevOptions.Shaders"),TEXT("NumUnusedShaderCompilingThreads"), OverrideNumUnusedShaderCompilingThreads);
	}
	
	int32 OverrideMaxShaderJobBatchSize;
	if(FParse::Value(FCommandLine::Get(), TEXT("-MaxShaderJobBatchSize="), OverrideMaxShaderJobBatchSize))
	{
		OverrideConfigValue( GEngineIni,TEXT("DevOptions.Shaders"), TEXT("MaxShaderJobBatchSize"), OverrideMaxShaderJobBatchSize);
	}

	int32 OverrideWorkerProcessPriority;
	if(FParse::Value(FCommandLine::Get(), TEXT("-SCWPriority="), OverrideWorkerProcessPriority))
	{
		OverrideConfigValue( GEngineIni,TEXT("DevOptions.Shaders"), TEXT("WorkerProcessPriority"), OverrideWorkerProcessPriority);
	}
}

void FCmdHandlerModule::ShutdownModule()
{

}
#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FCmdHandlerModule, CmdHandler)