// Copyright 2019 Lipeng Zha, Inc. All Rights Reserved.

#include "CmdHandler.h"

#include "Misc/CommandLine.h"
#include "Misc/ConfigCacheIni.h"

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

#if WITH_EDITOR
static bool bDDCUrl = false;
static FString MultiCookerDDCBackendName = TEXT("MultiCookerDDC");
void AddMultiCookerBackendToConfig(const FString& DDCAddr)
{
	if(DDCAddr.IsEmpty())
	{
		UE_LOG(LogCmdHandler, Warning, TEXT("not use MultiCookerDDC"));
		return;
	}
	auto EngineIniIns = GConfig->FindConfigFile(GEngineIni);
	auto MultiCookerDDCBackendSection = EngineIniIns->FindOrAddSection(MultiCookerDDCBackendName);

	auto UpdateKeyLambda = [](FConfigSection* Section,const FString& Key,const FString& Value)
	{
		if(Section->Find(*Key))
		{
			Section->Remove(*Key);
		}
		Section->Add(*Key,FConfigValue(*Value));
	};
	
	UpdateKeyLambda(MultiCookerDDCBackendSection,TEXT("MinimumDaysToKeepFile"),TEXT("7"));
	UpdateKeyLambda(MultiCookerDDCBackendSection,TEXT("Root"),TEXT("(Type=KeyLength, Length=120, Inner=AsyncPut)"));
	UpdateKeyLambda(MultiCookerDDCBackendSection,TEXT("AsyncPut"),TEXT("(Type=AsyncPut, Inner=Hierarchy)"));
	UpdateKeyLambda(MultiCookerDDCBackendSection,TEXT("Hierarchy"),TEXT("(Type=Hierarchical, Inner=Boot, Inner=PakWrite, Inner=PakRead, Inner=Local, Inner=Shared)"));
	UpdateKeyLambda(MultiCookerDDCBackendSection,TEXT("Boot"),TEXT("(Type=Boot, Filename=\"%GAMEDIR%DerivedDataCache/Boot.ddc\", MaxCacheSize=512)"));

	FString DDC = FString::Printf(
		TEXT("(Type=FileSystem, ReadOnly=false, Clean=true, Flush=false, DeleteUnused=true, UnusedFileAge=10, FoldersToClean=10, MaxFileChecksPerSec=1, ConsiderSlowAt=70, PromptIfMissing=false, Path=%s, EnvPathOverride=UE-SharedDataCachePath, EditorOverrideSetting=SharedDerivedDataCache)"),
		*DDCAddr
	);
	UpdateKeyLambda(MultiCookerDDCBackendSection,TEXT("Shared"),DDC);

	FString DDCBackendName;
	if(!FParse::Value(FCommandLine::Get(),TEXT("-ddc="),DDCBackendName))
	{
		DDCBackendName = MultiCookerDDCBackendName;
		FCommandLine::Append(*FString::Printf(TEXT(" -ddc=%s"),*DDCBackendName));
		UE_LOG(LogCmdHandler, Warning, TEXT("Append cmd: %s"),FCommandLine::Get());
	}
	UE_LOG(LogCmdHandler, Warning, TEXT("use MultiCookerDDC: %s"),*DDCBackendName);
}

void OverrideEditorEnv()
{
	int32 OverrideNumUnusedShaderCompilingThreads = 3;
	if(FParse::Value(FCommandLine::Get(), TEXT("-MaxShaderWorker="), OverrideNumUnusedShaderCompilingThreads))
	{
		OverrideNumUnusedShaderCompilingThreads = FMath::Max(OverrideNumUnusedShaderCompilingThreads,3);
		const int32 NumVirtualCores = FPlatformMisc::NumberOfCoresIncludingHyperthreads();
		OverrideNumUnusedShaderCompilingThreads = NumVirtualCores - OverrideNumUnusedShaderCompilingThreads;
	}
	GConfig->SetInt( TEXT("DevOptions.Shaders"), TEXT("NumUnusedShaderCompilingThreads"), OverrideNumUnusedShaderCompilingThreads, GEngineIni);
	OverrideConfigValue(GEngineIni,TEXT("DevOptions.Shaders"),TEXT("NumUnusedShaderCompilingThreads"), OverrideNumUnusedShaderCompilingThreads);
	
	int32 OverrideMaxShaderJobBatchSize = 10;
	FParse::Value(FCommandLine::Get(), TEXT("-MaxShaderJobBatchSize="), OverrideMaxShaderJobBatchSize);
	OverrideConfigValue( GEngineIni,TEXT("DevOptions.Shaders"), TEXT("MaxShaderJobBatchSize"), OverrideMaxShaderJobBatchSize);
	
	int32 OverrideWorkerProcessPriority;
	if(FParse::Value(FCommandLine::Get(), TEXT("-SCWPriority="), OverrideWorkerProcessPriority))
	{
		OverrideConfigValue( GEngineIni,TEXT("DevOptions.Shaders"), TEXT("WorkerProcessPriority"), OverrideWorkerProcessPriority);
	}
	
	FString DDCURL;
	if(FParse::Value(FCommandLine::Get(),TEXT("-ddcurl="),DDCURL) && !DDCURL.IsEmpty())
	{
		AddMultiCookerBackendToConfig(DDCURL);
		bDDCUrl = true;
	}
}

void UnOverrideEditorEnv()
{
	if(bDDCUrl)
	{
		auto EngineIniIns = GConfig->FindConfigFile(GEngineIni);
		EngineIniIns->Remove(MultiCookerDDCBackendName);
	}
}

#else

void OverrideRuntimeEnv()
{
	
}

void UnOverrideRuntimeEnv()
{
	
}
#endif

void FCmdHandlerModule::StartupModule()
{
#if WITH_EDITOR
	OverrideEditorEnv();
#else
	OverrideRuntimeEnv();
#endif
}

void FCmdHandlerModule::ShutdownModule()
{
#if WITH_EDITOR
	UnOverrideEditorEnv();
#else
	UnOverrideRuntimeEnv();
#endif
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FCmdHandlerModule, CmdHandler)