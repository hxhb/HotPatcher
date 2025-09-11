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
	// UE_LOG(LogHotPatcher,Display,TEXT("Load Shader pipeline cache %s for platform %d"),*Name,*ShaderPlatformToShaderFormatName(GMaxRHIShaderPlatform).ToString());
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

FString UFlibShaderPipelineCacheHelper::GetGamePathStable(const FString& FileName,EShaderPlatform ShaderPlatform)
{
	FName PlatformName = LegacyShaderPlatformToShaderFormat(ShaderPlatform);
	FString GamePathStable = FPaths::ProjectContentDir() / TEXT("PipelineCaches") / ANSI_TO_TCHAR(FPlatformProperties::IniPlatformName()) / FString::Printf(TEXT("%s_%s.stable.upipelinecache"), *FileName, *PlatformName.ToString());
	return GamePathStable;
}

struct FPipelineCacheFileFormatHeaderCopy
{
	uint64 Magic;			// Sanity check
	uint32 Version; 		// File version must match engine version, otherwise we ignore
	uint32 GameVersion; 	// Same as above but game specific code can invalidate
	TEnumAsByte<EShaderPlatform> Platform; // The shader platform for all referenced PSOs.
	FGuid Guid;				// Guid to identify the file uniquely
	uint64 TableOffset;		// absolute file offset to TOC
	
	friend FArchive& operator<<(FArchive& Ar, FPipelineCacheFileFormatHeaderCopy& Info)
	{
		Ar << Info.Magic;
		Ar << Info.Version;
		Ar << Info.GameVersion;
		Ar << Info.Platform;
		Ar << Info.Guid;
		Ar << Info.TableOffset;
		
		return Ar;
	}
};

FGuid UFlibShaderPipelineCacheHelper::GetStablePipelineCacheGUID(const FString& FilePath)
{
	FGuid GUID;
	if(FPaths::FileExists(FilePath))
	{
		TUniquePtr<FArchive> FileReader(IFileManager::Get().CreateFileReader(*FilePath));
		if (FileReader)
		{
			FPipelineCacheFileFormatHeaderCopy Header;
			*FileReader << Header;
			GUID = Header.Guid;
			FileReader->Close();
		};
	}
	return GUID;
}
#if PLATFORM_ANDROID
#include "Misc/SecureHash.h"
#include "RHI.h"
FString UFlibShaderPipelineCacheHelper::GetOpenGLProgramBinaryName()
{
	FString HashString;
	HashString.Append(GRHIAdapterInternalDriverVersion);
	HashString.Append(GRHIAdapterName);
	FSHAHash VersionHash;
	FSHA1::HashBuffer(TCHAR_TO_ANSI(*HashString), HashString.Len(), VersionHash.Hash);

	UE_LOG(LogHotPatcher,Display,TEXT("GL_VERSION: %s ,GL_RENDERER %s, VersionHash: %s"),*GRHIAdapterInternalDriverVersion,*GRHIAdapterName,*VersionHash.ToString());
	FString CacheFilename = LegacyShaderPlatformToShaderFormat(GMaxRHIShaderPlatform).ToString() + TEXT("_") + VersionHash.ToString();
	return CacheFilename;
}

FString UFlibShaderPipelineCacheHelper::GetOpenGLProgramBinaryFile()
{
	FString CacheFolderPath;
#if PLATFORM_ANDROID && USE_ANDROID_FILE
	// @todo Lumin: Use that GetPathForExternalWrite or something?
	extern FString GExternalFilePath;
	CacheFolderPath = GExternalFilePath / TEXT("ProgramBinaryCache");
			
#else
	CacheFolderPath = FPaths::ProjectSavedDir() / TEXT("ProgramBinaryCache");
#endif
	FString CacheFileName = GetOpenGLProgramBinaryName();
	FString ProgramBinaryPath = FPaths::Combine(CacheFolderPath,CacheFileName);
	if(!FPaths::FileExists(ProgramBinaryPath) && FPaths::FileExists(ProgramBinaryPath+TEXT(".scan")))
	{
		ProgramBinaryPath+=TEXT(".scan");
	}
	return ProgramBinaryPath;
}

FGuid UFlibShaderPipelineCacheHelper::GetOpenGLProgramBinaryGUID(const FString& FilePath)
{
	FGuid BinaryCacheGuid;
	if(FPaths::FileExists(FilePath))
	{
		TUniquePtr<FArchive> FileReader(IFileManager::Get().CreateFileReader(*FilePath));
		if (FileReader)
		{
			FArchive& Ar = *FileReader;
			uint32 Version = 0;
			Ar << Version;
			Ar << BinaryCacheGuid;
			FileReader->Close();
		}
	}
	return BinaryCacheGuid;
}
#endif

void UFlibShaderPipelineCacheHelper::SetPsoBatchMode(FShaderPipelineCache::BatchMode BatchMode,bool bOverrideVar,int32 BatchSize,float BatchTime)
{
	if(bOverrideVar)
	{
		IConsoleVariable* BatchSizeVar = nullptr;
		IConsoleVariable* BatchTimeVar = nullptr;
		switch (BatchMode)
		{
			case FShaderPipelineCache::BatchMode::Precompile:{
					BatchSizeVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.ShaderPipelineCache.PrecompileBatchSize"));
					BatchTimeVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.ShaderPipelineCache.PrecompileBatchTime"));
					break;
			}
			case FShaderPipelineCache::BatchMode::Fast:{
					BatchSizeVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.ShaderPipelineCache.BatchSize"));
					BatchTimeVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.ShaderPipelineCache.BatchTime"));
					break;
			}
			case FShaderPipelineCache::BatchMode::Background:{
					BatchSizeVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.ShaderPipelineCache.BackgroundBatchSize"));
					BatchTimeVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.ShaderPipelineCache.BackgroundBatchTime"));
					break;
			}
		}
		if(BatchSizeVar) { BatchSizeVar->Set(BatchSize); }
		if(BatchTimeVar){ BatchTimeVar->Set(BatchTime); }
	}
	FShaderPipelineCache::SetBatchMode(BatchMode);
};

#if PLATFORM_IOS
bool UFlibShaderPipelineCacheHelper::HasValidMetalSystemCache()
{
	bool bReady = false;
	struct stat FileInfo1,FileInfo2;
	static FString PrivateWritePathBase = FString([NSSearchPathForDirectoriesInDomains(NSLibraryDirectory, NSUserDomainMask, YES) objectAtIndex:0]) + TEXT("/");
	FString Result = PrivateWritePathBase + FString([NSString stringWithFormat:@"/Caches/%@/com.apple.metal/functions.data", [NSBundle mainBundle].bundleIdentifier]);
	FString Result2 = PrivateWritePathBase + FString([NSString stringWithFormat:@"/Caches/%@/com.apple.metal/usecache.txt", [NSBundle mainBundle].bundleIdentifier]);
	
	auto Var = IConsoleManager::Get().FindConsoleVariable(TEXT("r.ShaderPipelineCache.MetalCacheMinSizeInMB"));
	int32 MetalCacheMinSize = Var->GetInt();
	
	bool bFileStat1 = (stat(TCHAR_TO_UTF8(*Result), &FileInfo1) != -1);
	bool bFileStat2 = (stat(TCHAR_TO_UTF8(*Result2), &FileInfo2) != -1);
	UE_LOG(LogHotPatcher,Display,TEXT("Metal functions.data: %s, size: %d"),*Result,bFileStat1 ? FileInfo1.st_size : 0);
	UE_LOG(LogHotPatcher,Display,TEXT("Metal usecache.txt: %s, size: %d"),*Result2,bFileStat2 ? FileInfo2.st_size : 0);

	if (bFileStat1 && ((FileInfo1.st_size / 1024 / 1024) > MetalCacheMinSize) && bFileStat2)
	{
		bReady = true;
	}
	return bReady;
}
#endif