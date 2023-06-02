#pragma once
#include "CommandletHelper.h"

#include "ETargetPlatform.h"
#include "FlibPatchParserHelper.h"
#include "ThreadUtils/FProcWorkerThread.hpp"
#include "HotPatcherLog.h"
#include "Engine/Engine.h"

void CommandletHelper::ReceiveMsg(const FString& InMsgType,const FString& InMsg)
{
	UE_LOG(LogHotPatcherCommandlet,Display,TEXT("%s:%s"),*InMsgType,*InMsg);
}

void CommandletHelper::ReceiveShowMsg(const FString& InMsg)
{
	UE_LOG(LogHotPatcherCommandlet,Display,TEXT("%s"),*InMsg);
}
	
TArray<FString> CommandletHelper::ParserPatchConfigByCommandline(const FString& Commandline,const FString& Token)
{
	TArray<FString> result;
	TMap<FString, FString> KeyValues = THotPatcherTemplateHelper::GetCommandLineParamsMap(Commandline);
	if(KeyValues.Find(Token))
	{
		FString AddPakListInfo = *KeyValues.Find(Token);
		AddPakListInfo.ParseIntoArray(result,TEXT(","));
	}
	return result;
}
	
TArray<ETargetPlatform> CommandletHelper::ParserPlatforms(const FString& Commandline, const FString& Token)
{
	TArray<ETargetPlatform> result;
	for(auto& PlatformName:ParserPatchConfigByCommandline(Commandline,Token))
	{
		ETargetPlatform Platform = ETargetPlatform::None;
		THotPatcherTemplateHelper::GetEnumValueByName(PlatformName,Platform);
		if(Platform != ETargetPlatform::None)
		{
			result.AddUnique(Platform);
		}
	}
	return result;
}

TArray<FDirectoryPath> CommandletHelper::ParserPatchFilters(const FString& Commandline,const FString& FilterName)
{
	TArray<FDirectoryPath> Result;
	for(auto& FilterPath:ParserPatchConfigByCommandline(Commandline,FString::Printf(TEXT("Add%s"),*FilterName)))
	{
		FDirectoryPath Path;
		Path.Path = FilterPath;
		Result.Add(Path);
	}
	return Result;
}

static bool IsRequestingExit()
{
#if ENGINE_MAJOR_VERSION > 4 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION > 23)
	return IsEngineExitRequested();
#else
	return GIsRequestingExit;
#endif
}

static void RequestEngineExit()
{
#if ENGINE_MAJOR_VERSION > 4 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION > 23)
	RequestEngineExit(TEXT("GeneralCommandlet ComWrapperShutdownEvent"));
#else
	GIsRequestingExit  = true;
#endif
}

void CommandletHelper::MainTick(TFunction<bool()> IsRequestExit)
{
	GIsRunning = true;

	//@todo abstract properly or delete
	#if PLATFORM_WINDOWS// Windows only
	// Used by the .com wrapper to notify that the Ctrl-C handler was triggered.
	// This shared event is checked each tick so that the log file can be cleanly flushed.
	FEvent* ComWrapperShutdownEvent = FPlatformProcess::GetSynchEventFromPool(true);
#endif
		
	// main loop
	FDateTime LastConnectionTime = FDateTime::UtcNow();

	while (GIsRunning &&
		// !IsRequestingExit() &&
		!IsRequestExit())
	{
		GEngine->UpdateTimeAndHandleMaxTickRate();
		GEngine->Tick(FApp::GetDeltaTime(), false);

		// update task graph
		FTaskGraphInterface::Get().ProcessThreadUntilIdle(ENamedThreads::GameThread);

		// execute deferred commands
		for (int32 DeferredCommandsIndex=0; DeferredCommandsIndex<GEngine->DeferredCommands.Num(); DeferredCommandsIndex++)
		{
			GEngine->Exec( GWorld, *GEngine->DeferredCommands[DeferredCommandsIndex], *GLog);
		}

		GEngine->DeferredCommands.Empty();

		// flush log
		GLog->FlushThreadedLogs();

#if PLATFORM_WINDOWS
		if (ComWrapperShutdownEvent->Wait(0))
		{
			RequestEngineExit();
		}
#endif
	}
		
	//@todo abstract properly or delete 
	#if PLATFORM_WINDOWS
	FPlatformProcess::ReturnSynchEventToPool(ComWrapperShutdownEvent);
	ComWrapperShutdownEvent = nullptr;
#endif

	GIsRunning = false;
}

bool CommandletHelper::GetCommandletArg(const FString& Token,FString& OutValue)
{
	OutValue.Empty();
	FString Value;
	bool bHasToken = FParse::Value(FCommandLine::Get(), *Token, Value);
	if(bHasToken && !Value.IsEmpty())
	{
		OutValue = Value;
	}
	return bHasToken && !OutValue.IsEmpty();
}

bool CommandletHelper::IsCookCommandlet()
{
	bool bIsCookCommandlet = false;

	if(::IsRunningCommandlet())
	{
		FString CommandletName;
		bool bIsCommandlet = CommandletHelper::GetCommandletArg(TEXT("-run="),CommandletName); //FParse::Value(FCommandLine::Get(), TEXT("-run="), CommandletName);
	
		if(bIsCommandlet && !CommandletName.IsEmpty())
		{
			bIsCookCommandlet = CommandletName.Equals(TEXT("cook"),ESearchCase::IgnoreCase);
		}
	}
	return bIsCookCommandlet;
}

TArray<ETargetPlatform> CommandletHelper::GetCookCommandletTargetPlatforms()
{
	TArray<ETargetPlatform> TargetPlatforms;
	{
		FString PlatformName;
		if(CommandletHelper::GetCommandletArg(TEXT("-TargetPlatform="),PlatformName))
		{
			ETargetPlatform TargetPlatform;
			THotPatcherTemplateHelper::GetEnumValueByName(PlatformName,TargetPlatform);
			TargetPlatforms.AddUnique(TargetPlatform);
		}
	}
	return TargetPlatforms;
}

TArray<FString> CommandletHelper::GetCookCommandletTargetPlatformName()
{
	TArray<FString> result;
	TArray<ETargetPlatform> Platforms = CommandletHelper::GetCookCommandletTargetPlatforms();

	for(const auto& Platform:Platforms)
	{
		result.AddUnique(THotPatcherTemplateHelper::GetEnumNameByValue(Platform));
	}

	return result;
}


void CommandletHelper::ModifyTargetPlatforms(const FString& InParams,const FString& InToken,TArray<ETargetPlatform>& OutTargetPlatforms,bool Replace)
{
	TArray<ETargetPlatform> TargetPlatforms = CommandletHelper::ParserPlatforms(InParams,InToken);
	if(TargetPlatforms.Num())
	{
		if(Replace){
			OutTargetPlatforms = TargetPlatforms;
		}else{
			for(ETargetPlatform Platform:TargetPlatforms){ OutTargetPlatforms.AddUnique(Platform); }
		}
	}
};