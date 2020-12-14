// Fill out your copyright notice in the Description page of Project Settings.


#include "ShaderPatch/FlibShaderPatchHelper.h"
#include "ShaderCodeLibrary.h"



bool UFlibShaderPatchHelper::CreateShaderCodePatch(TArray<FString> const& OldMetaDataDirs, FString const& NewMetaDataDir, FString const& OutDir, bool bNativeFormat)
{
#if ENGINE_MINOR_VERSION > 25
	return FShaderCodeLibrary::CreatePatchLibrary(OldMetaDataDirs,NewMetaDataDir,OutDir,bNativeFormat,true);
#else
	#if ENGINE_MINOR_VERSION > 23
		return FShaderCodeLibrary::CreatePatchLibrary(OldMetaDataDirs,NewMetaDataDir,OutDir,bNativeFormat);
	#else
		return false;
	#endif
#endif
}

TArray<FString> UFlibShaderPatchHelper::ConvDirectoryPathToStr(const TArray<FDirectoryPath>& Dirs)
{
	TArray<FString> result;
	for(const auto& Dir:Dirs)
	{
		FString Path = FPaths::ConvertRelativePathToFull(Dir.Path);
		if(FPaths::DirectoryExists(Path))
		{
			result.AddUnique(Path);
		}
	}
	return result;
}