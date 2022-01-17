#pragma once

#include "ETargetPlatform.h"
#include "FlibPatchParserHelper.h"
#include "ThreadUtils/FProcWorkerThread.hpp"
#include "HotPatcherLog.h"

#define PATCHER_CONFIG_PARAM_NAME TEXT("-config=")

DECLARE_LOG_CATEGORY_EXTERN(LogHotPatcherCommandlet, All, All);

namespace CommandletHelper
{
	namespace NSPatch
	{
		static void ReceiveMsg(const FString& InMsgType,const FString& InMsg)
		{
			UE_LOG(LogHotPatcherCommandlet,Display,TEXT("%s:%s"),*InMsgType,*InMsg);
		}

		static void ReceiveShowMsg(const FString& InMsg)
		{
			UE_LOG(LogHotPatcherCommandlet,Display,TEXT("%s"),*InMsg);
		}
	}

	static TArray<FString> ParserPatchConfigByCommandline(const FString& Commandline,const FString& Token)
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


#define ADD_PATCH_PLATFORMS TEXT("AddPatchPlatforms")
	static TArray<ETargetPlatform> ParserPatchPlatforms(const FString& Commandline)
	{
		TArray<ETargetPlatform> result;
		for(auto& PlatformName:ParserPatchConfigByCommandline(Commandline,ADD_PATCH_PLATFORMS))
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

	static TArray<FDirectoryPath> ParserPatchFilters(const FString& Commandline,const FString& FilterName)
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

	void MainTick(TFunction<bool()> IsRequestExit)
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

		while (GIsRunning && !IsEngineExitRequested() && !IsRequestExit())
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
				RequestEngineExit(TEXT("GeneralCommandlet ComWrapperShutdownEvent"));
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
}
