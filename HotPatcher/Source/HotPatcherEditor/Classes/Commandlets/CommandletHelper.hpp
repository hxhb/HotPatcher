#pragma once


#include "ETargetPlatform.h"
#include "FlibPatchParserHelper.h"

#define PATCHER_CONFIG_PARAM_NAME TEXT("-config=")

namespace CommandletHelper
{
	namespace NSPatch
	{
		void ReceiveMsg(const FString& InMsgType,const FString& InMsg)
		{
			UE_LOG(LogHotPatcherCommandlet,Display,TEXT("%s:%s"),*InMsgType,*InMsg);
		}

		void ReceiveShowMsg(const FString& InMsg)
		{
			UE_LOG(LogHotPatcherCommandlet,Display,TEXT("%s"),*InMsg);
		}
	}

	TArray<FString> ParserPatchConfigByCommandline(const FString& Commandline,const FString& Token)
	{
		TArray<FString> result;
		TMap<FString, FString> KeyValues = UFlibPatchParserHelper::GetCommandLineParamsMap(Commandline);
		if(KeyValues.Find(Token))
		{
			FString AddPakListInfo = *KeyValues.Find(Token);
			AddPakListInfo.ParseIntoArray(result,TEXT(","));
		}
		return result;
	}


#define ADD_PATCH_PLATFORMS TEXT("AddPatchPlatforms")
	TArray<ETargetPlatform> ParserPatchPlatforms(const FString& Commandline)
	{
		TArray<ETargetPlatform> result;
		for(auto& PlatformName:ParserPatchConfigByCommandline(Commandline,ADD_PATCH_PLATFORMS))
		{
			ETargetPlatform Platform = ETargetPlatform::None;
			UFlibPatchParserHelper::GetEnumValueByName(PlatformName,Platform);
			if(Platform != ETargetPlatform::None)
			{
				result.AddUnique(Platform);
			}
		}
		return result;
	}

	TArray<FDirectoryPath> ParserPatchFilters(const FString& Commandline,const FString& FilterName)
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
}
