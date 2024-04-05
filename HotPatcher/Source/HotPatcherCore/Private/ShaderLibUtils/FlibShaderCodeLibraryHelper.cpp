// Fill out your copyright notice in the Description page of Project Settings.

#include "ShaderLibUtils//FlibShaderCodeLibraryHelper.h"
#include "HotPatcherLog.h"
#include "FlibHotPatcherCoreHelper.h"
#include "HotPatcherCore.h"

#include "ShaderCompiler.h"
#include "IPlatformFileSandboxWrapper.h"
#include "Interfaces/IPluginManager.h"
#include "Interfaces/ITargetPlatform.h"
#include "Misc/EngineVersionComparison.h"

#define REMAPPED_PLUGINS TEXT("RemappedPlugins")

FString UFlibShaderCodeLibraryHelper::ShaderExtension = TEXT(".ushaderbytecode");
FString UFlibShaderCodeLibraryHelper::ShaderAssetInfoExtension = TEXT(".assetinfo.json");
FString UFlibShaderCodeLibraryHelper::StableExtension = TEXT(".scl.csv");

// FMergeShaderCollectionProxy::FMergeShaderCollectionProxy(const TArray<FShaderCodeFormatMap>& InShaderCodeFiles):ShaderCodeFiles(InShaderCodeFiles)
// {
// 	Init();
// }
// FMergeShaderCollectionProxy::~FMergeShaderCollectionProxy()
// {
// 	Shutdown();
// }
//
// void FMergeShaderCollectionProxy::Init()
// {
// 	for(const auto& ShaderCodeFoemat:ShaderCodeFiles)
// 	{
// 		TArray<FName> ShaderFormatNames = UFlibShaderCodeLibraryHelper::GetShaderFormatsByTargetPlatform(ShaderCodeFoemat.Platform);
// 		
// 		for(const auto& ShaderFormatName:ShaderFormatNames)
// 		{
// 			EShaderPlatform ShaderPlatform= ::ShaderFormatToLegacyShaderPlatform(ShaderFormatName);
// 			
// 			if(ShaderCodeFoemat.ShaderCodeTypeFilesMap.Contains(ShaderFormatName.ToString()))
// 			{
// 				SHADER_COOKER_CLASS::InitForCooking(ShaderCodeFoemat.bIsNative);
// 				SHADER_COOKER_CLASS::InitForRuntime(ShaderPlatform);
// 				TArray<FName> CurrentPlatforomShaderTypeNames;
// 				{
// 					TArray<FString> PlatforomShaderTypeNames;
// 					ShaderCodeFoemat.ShaderCodeTypeFilesMap.GetKeys(PlatforomShaderTypeNames);
// 					for(const auto& Name:PlatforomShaderTypeNames)
// 					{
// 						CurrentPlatforomShaderTypeNames.AddUnique(*Name);
// 					}
// 				}
//
// #if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION > 25
// 				TArray<SHADER_COOKER_CLASS::FShaderFormatDescriptor> ShaderFormatsWithStableKeys = UFlibShaderCodeLibraryHelper::GetShaderFormatsWithStableKeys(CurrentPlatforomShaderTypeNames);
// #else
// 				TArray<TPair<FName, bool>> ShaderFormatsWithStableKeys;
// 				for (FName& Format : ShaderFormatNames)
// 				{
// 					ShaderFormatsWithStableKeys.Push(MakeTuple(Format, true));
// 				}
// #endif
// 				
// 				SHADER_COOKER_CLASS::CookShaderFormats(ShaderFormatsWithStableKeys);
// 				FShaderCodeFormatMap::FShaderFormatNameFiles ShaderFormatNameFiles =  *ShaderCodeFoemat.ShaderCodeTypeFilesMap.Find(ShaderFormatName.ToString());
// 				for(const auto& File:ShaderFormatNameFiles.Files)
// 				{
// 					FString Path = FPaths::GetPath(File);
// 					FShaderCodeLibrary::OpenLibrary(ShaderFormatNameFiles.ShaderName,Path);
// 				}
// 				if(!UFlibShaderCodeLibraryHelper::SaveShaderLibrary(ShaderCodeFoemat.Platform, NULL, FApp::GetProjectName(),ShaderCodeFoemat.SaveBaseDir,true))
// 				{
// 					UE_LOG(LogHotPatcherCoreHelper,Display,TEXT("SaveShaderLibrary %s for %s Failed!"),*FApp::GetProjectName(),*ShaderCodeFoemat.Platform->PlatformName() );
// 				}
// 				SHADER_COOKER_CLASS::Shutdown();
// 			}
// 		}
// 	}
// }
//
// void FMergeShaderCollectionProxy::Shutdown()
// {
// 	FShaderCodeLibrary::Shutdown();
// }

#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION > 25
TArray<SHADER_COOKER_CLASS::FShaderFormatDescriptor> UFlibShaderCodeLibraryHelper::GetShaderFormatsWithStableKeys(
	const TArray<FName>& ShaderFormats,bool bNeedShaderStableKeys/* = true*/,bool bNeedsDeterministicOrder/* = true*/)
{
	TArray<SHADER_COOKER_CLASS::FShaderFormatDescriptor> ShaderFormatsWithStableKeys;
	for (const FName& Format : ShaderFormats)
	{
		SHADER_COOKER_CLASS::FShaderFormatDescriptor NewDesc;
		NewDesc.ShaderFormat = Format;
		NewDesc.bNeedsStableKeys = bNeedShaderStableKeys;
		NewDesc.bNeedsDeterministicOrder = bNeedsDeterministicOrder;
		ShaderFormatsWithStableKeys.Push(NewDesc);
	}
	return ShaderFormatsWithStableKeys;
}
#endif

TArray<FName> UFlibShaderCodeLibraryHelper::GetShaderFormatsByTargetPlatform(ITargetPlatform* TargetPlatform)
{
	TArray<FName> ShaderFormats;
	TargetPlatform->GetAllTargetedShaderFormats(ShaderFormats);
	return ShaderFormats;
}

FString UFlibShaderCodeLibraryHelper::GenerateShaderCodeLibraryName(FString const& Name, bool bIsIterateSharedBuild)
{
	FString ActualName = (!bIsIterateSharedBuild) ? Name : Name + TEXT("_SC");
	return ActualName;
}

bool UFlibShaderCodeLibraryHelper::SaveShaderLibrary(const ITargetPlatform* TargetPlatform,TArray<FName> ShaderFormats,FString const& Name, const FString& ShaderCodeDir,const FString& RootMetaDataPath, bool bMaster)
{
	bool bSaved = false;
	// TargetPlatform->GetAllTargetedShaderFormats(ShaderFormats);
	if (ShaderFormats.Num() > 0)
	{
		FString TargetPlatformName = TargetPlatform->PlatformName();
		TArray<FString> PlatformSCLCSVPaths;// = OutSCLCSVPaths.FindOrAdd(FName(TargetPlatformName));
		
		#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION > 25
#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION > 26
		FString ErrorString;
#if !UE_VERSION_OLDER_THAN(5,1,0)
		bool bOutHasData = false;
#endif
		bSaved = SHADER_COOKER_CLASS::SaveShaderLibraryWithoutChunking(TargetPlatform, Name, ShaderCodeDir, RootMetaDataPath, PlatformSCLCSVPaths, ErrorString
#if !UE_VERSION_OLDER_THAN(5,1,0)
		,bOutHasData
#endif
		);
#else
		TArray<TSet<FName>> ChunkAssignments;
		bSaved = FShaderCodeLibrary::SaveShaderCode(ShaderCodeDir, RootMetaDataPath, ShaderFormats, PlatformSCLCSVPaths, &ChunkAssignments);
#endif
#else
		if(bMaster)
		{
			bSaved = FShaderCodeLibrary::SaveShaderCodeMaster(ShaderCodeDir, RootMetaDataPath, ShaderFormats, PlatformSCLCSVPaths);
		}
		else
		{
			bSaved = FShaderCodeLibrary::SaveShaderCodeChild(ShaderCodeDir, RootMetaDataPath, ShaderFormats);
		}
#endif	
	}
	return bSaved;
}

bool UFlibShaderCodeLibraryHelper::SaveShaderLibrary(const ITargetPlatform* TargetPlatform, const TArray<TSet<FName>>* ChunkAssignments, FString const& Name, const FString&
                                                     SaveBaseDir, bool bMaster)
{
	bool bSaved = false;
	FString ActualName = GenerateShaderCodeLibraryName(Name, false);
	FString BasePath = FPaths::ProjectContentDir();
	
	FString ShaderCodeDir = FPaths::Combine(SaveBaseDir,TargetPlatform->PlatformName());

	const FString RootMetaDataPath = ShaderCodeDir / TEXT("Metadata") / TEXT("PipelineCaches");

	// note that shader formats can be shared across the target platforms
	TArray<FName> ShaderFormats;
	TargetPlatform->GetAllTargetedShaderFormats(ShaderFormats);
	if (ShaderFormats.Num() > 0)
	{
		bSaved = UFlibShaderCodeLibraryHelper::SaveShaderLibrary(TargetPlatform,ShaderFormats,Name,ShaderCodeDir,RootMetaDataPath,bMaster);
	}
	return bSaved;
}


TArray<FString> UFlibShaderCodeLibraryHelper::FindCookedShaderLibByShaderFrmat(const FString& ShaderFormatName,const FString& Directory)
{
	TArray<FString> result;
	TArray<FString > FoundShaderFiles;
	IFileManager::Get().FindFiles(FoundShaderFiles,*Directory,TEXT("ushaderbytecode"));

	FString FormatExtersion = FString::Printf(TEXT("*%s*.ushaderbytecode"),*ShaderFormatName);
	for(const auto& ShaderFile:FoundShaderFiles)
	{
		if(ShaderFile.MatchesWildcard(FormatExtersion))
		{
			result.Add(ShaderFile);
		}
	}
	return result;
}

TArray<FString> UFlibShaderCodeLibraryHelper::FindCookedShaderLibByPlatform(const FString& PlatfomName,const FString& Directory, bool bRecursive)
{
	SCOPED_NAMED_EVENT_TEXT("UFlibShaderCodeLibraryHelper::FindCookedShaderLibByPlatform",FColor::Red);
	TArray<FString> FoundFiles;
	auto GetMetalShaderFormatLambda = [](const FString& Directory,const FString& Extersion, bool bRecursive)
	{
		TArray<FString> FoundMetallibFiles;
		if(bRecursive)
		{
			IFileManager::Get().FindFilesRecursive(FoundMetallibFiles,*Directory,*Extersion,true,false, false);
		}
		else
		{
			IFileManager::Get().FindFiles(FoundMetallibFiles,*Directory,*Extersion);
		}
		return FoundMetallibFiles;
	};
	
	if(PlatfomName.StartsWith(TEXT("IOS"),ESearchCase::IgnoreCase) || PlatfomName.StartsWith(TEXT("Mac"),ESearchCase::IgnoreCase))
	{
		FoundFiles.Append(GetMetalShaderFormatLambda(Directory,TEXT("*.metallib"),bRecursive));
		FoundFiles.Append(GetMetalShaderFormatLambda(Directory,TEXT("*.metalmap"),bRecursive));
	}
	
	if(!FoundFiles.Num())
	{
		FoundFiles.Append(GetMetalShaderFormatLambda(Directory,TEXT("*.ushaderbytecode"),bRecursive));
	}
	if(!bRecursive)
	{
		for(auto& File:FoundFiles)
		{
			File = FPaths::Combine(Directory,File);
		}
	}
	return FoundFiles;
}

void UFlibShaderCodeLibraryHelper::WaitShaderCompilingComplete()
{
	// Wait for all shaders to finish compiling
	if (GShaderCompilingManager)
	{
		SCOPED_NAMED_EVENT_TEXT("WaitShaderCompileComplete",FColor::Red);
		while(GShaderCompilingManager->IsCompiling())
		{
			GShaderCompilingManager->ProcessAsyncResults(false, false);
			// int32 CurrentNumRemaingingJobs = GShaderCompilingManager->GetNumRemainingJobs();
			// if(GCookLog && LastRemainingJob != CurrentNumRemaingingJobs)
			// {
			// 	UE_LOG(LogHotPatcher,Display,TEXT("Remaining Shader %d"),CurrentNumRemaingingJobs);
			// 	LastRemainingJob = CurrentNumRemaingingJobs;
			// }
			// GShaderCompilingManager->FinishAllCompilation();
			FPlatformProcess::Sleep(0.5f);
			GLog->Flush();
		}

		// One last process to get the shaders that were compiled at the very end
		GShaderCompilingManager->ProcessAsyncResults(false, false);
		GShaderCompilingManager->FinishAllCompilation();
	}
}

void UFlibShaderCodeLibraryHelper::CleanShaderWorkerDir()
{
	if (FPaths::DirectoryExists(FPaths::ShaderWorkingDir()) && !IFileManager::Get().DeleteDirectory(*FPaths::ShaderWorkingDir(), false, true))
	{
		UE_LOG(LogHotPatcher, Warning, TEXT("Could not delete the shader compiler working directory '%s'."), *FPaths::ShaderWorkingDir());
	}
}

#if !UE_VERSION_OLDER_THAN(5,2,0)
#include "DataDrivenShaderPlatformInfo.h"
#endif
bool UFlibShaderCodeLibraryHelper::RHISupportsNativeShaderLibraries(const FStaticShaderPlatform Platform)
{
	return ::RHISupportsNativeShaderLibraries(Platform);;
}

void UFlibShaderCodeLibraryHelper::CancelMaterialShaderCompile(UMaterialInterface* MaterialInterface)
{
	if(MaterialInterface)
	{
		UMaterial* Material = MaterialInterface->GetMaterial();
		for (int32 FeatureLevel = 0; FeatureLevel < ERHIFeatureLevel::Num; ++FeatureLevel)
		{
			if (FMaterialResource* Res = Material->GetMaterialResource((ERHIFeatureLevel::Type)FeatureLevel))
			{
				Res->CancelCompilation();
			}
		}
	}
}
