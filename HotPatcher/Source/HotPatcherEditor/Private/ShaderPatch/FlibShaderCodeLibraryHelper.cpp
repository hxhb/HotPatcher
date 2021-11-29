// Fill out your copyright notice in the Description page of Project Settings.


#include "ShaderPatch/FlibShaderCodeLibraryHelper.h"

#include "FlibHotPatcherEditorHelper.h"
#include "IPlatformFileSandboxWrapper.h"
#include "Interfaces/IPluginManager.h"

#define REMAPPED_PLUGINS TEXT("RemappedPlugins")

FCookShaderCollectionProxy::FCookShaderCollectionProxy(const TArray<FString>& InPlatformNames,const FString& InLibraryName,bool bInShareShader,bool InIsNative,const FString& InSaveBaseDir)
:PlatformNames(InPlatformNames),LibraryName(InLibraryName),bShareShader(bInShareShader),bIsNative(InIsNative),SaveBaseDir(InSaveBaseDir){}

FCookShaderCollectionProxy::~FCookShaderCollectionProxy(){}

void FCookShaderCollectionProxy::Init()
{
	if(bShareShader)
	{
		SHADER_COOKER_CLASS::InitForCooking(bIsNative);
		for(const auto& PlatformName:PlatformNames)
		{
			ITargetPlatform* TargetPlatform = UFlibHotPatcherEditorHelper::GetPlatformByName(PlatformName);
			TargetPlatforms.AddUnique(TargetPlatform);
			TArray<FName> ShaderFormats = UFlibShaderCodeLibraryHelper::GetShaderFormatsByTargetPlatform(TargetPlatform);
			if (ShaderFormats.Num() > 0)
			{
	#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION > 25
				TArray<SHADER_COOKER_CLASS::FShaderFormatDescriptor> ShaderFormatsWithStableKeys = UFlibShaderCodeLibraryHelper::GetShaderFormatsWithStableKeys(ShaderFormats);
	#else
				TArray<TPair<FName, bool>> ShaderFormatsWithStableKeys;
				for (FName& Format : ShaderFormats)
				{
					ShaderFormatsWithStableKeys.Push(MakeTuple(Format, true));
				}
	#endif
				SHADER_COOKER_CLASS::CookShaderFormats(ShaderFormatsWithStableKeys);
			}
		}
		FShaderCodeLibrary::OpenLibrary(LibraryName,TEXT(""));
	}
}

bool IsAppleMetalPlatform(ITargetPlatform* TargetPlatform)
{
	bool bIsMatched = false;
	TArray<FString> ApplePlatforms = {TEXT("IOS"),TEXT("Mac"),TEXT("TVOS")};
	for(const auto& Platform:ApplePlatforms)
	{
		if(TargetPlatform->PlatformName().StartsWith(Platform,ESearchCase::IgnoreCase))
		{
			bIsMatched = true;
			break;
		}
	}
	return bIsMatched;
}

void FCookShaderCollectionProxy::Shutdown()
{
	if(bShareShader)
	{
		for(const auto& TargetPlatform:TargetPlatforms)
		{
			FString PlatformName = TargetPlatform->PlatformName();
			bSuccessed = UFlibShaderCodeLibraryHelper::SaveShaderLibrary(TargetPlatform,NULL, LibraryName,SaveBaseDir);
#if ENGINE_MAJOR_VERSION < 5 && ENGINE_MINOR_VERSION <= 26
			if(bIsNative)
			{
				FString ShaderCodeDir = FPaths::Combine(SaveBaseDir,PlatformName);
				bSuccessed = bSuccessed && FShaderCodeLibrary::PackageNativeShaderLibrary(ShaderCodeDir,UFlibShaderCodeLibraryHelper::GetShaderFormatsByTargetPlatform(TargetPlatform));
			}
#endif
			// rename StarterContent_SF_METAL.0.metallib to startercontent_sf_metal.0.metallib
			if(bSuccessed && bIsNative && IsAppleMetalPlatform(TargetPlatform))
			{
				TArray<FString> FoundShaderLibs = UFlibShaderCodeLibraryHelper::FindCookedShaderLibByPlatform(PlatformName,FPaths::Combine(SaveBaseDir,TargetPlatform->PlatformName()));
				for(const auto& Shaderlib:FoundShaderLibs)
				{
					if(Shaderlib.EndsWith(TEXT("metallib"),ESearchCase::IgnoreCase) || Shaderlib.EndsWith(TEXT("metalmap"),ESearchCase::IgnoreCase))
					{
						FString Path = FPaths::GetPath(Shaderlib);
						FString Name = FPaths::GetBaseFilename(Shaderlib,true);
						FString Extersion = FPaths::GetExtension(Shaderlib,true);
						Name = FString::Printf(TEXT("%s%s"),*Name,*Extersion);
						Name.ToLowerInline();
						if(!Shaderlib.EndsWith(Name,ESearchCase::CaseSensitive))
						{
							IFileManager::Get().Move(*FPaths::Combine(Path,Name),*Shaderlib,false);
						}
					}
				}
			}
		}
		FShaderCodeLibrary::CloseLibrary(LibraryName);
		FShaderCodeLibrary::Shutdown();
	}
}

FMergeShaderCollectionProxy::FMergeShaderCollectionProxy(const TArray<FShaderCodeFormatMap>& InShaderCodeFiles):ShaderCodeFiles(InShaderCodeFiles)
{
	Init();
}
FMergeShaderCollectionProxy::~FMergeShaderCollectionProxy()
{
	Shutdown();
}

void FMergeShaderCollectionProxy::Init()
{
	for(const auto& ShaderCodeFoemat:ShaderCodeFiles)
	{
		TArray<FName> ShaderFormatNames = UFlibShaderCodeLibraryHelper::GetShaderFormatsByTargetPlatform(ShaderCodeFoemat.Platform);
		
		for(const auto& ShaderFormatName:ShaderFormatNames)
		{
			EShaderPlatform ShaderPlatform= ::ShaderFormatToLegacyShaderPlatform(ShaderFormatName);
			
			if(ShaderCodeFoemat.ShaderCodeTypeFilesMap.Contains(ShaderFormatName.ToString()))
			{
				SHADER_COOKER_CLASS::InitForCooking(ShaderCodeFoemat.bIsNative);
				SHADER_COOKER_CLASS::InitForRuntime(ShaderPlatform);
				TArray<FName> CurrentPlatforomShaderTypeNames;
				{
					TArray<FString> PlatforomShaderTypeNames;
					ShaderCodeFoemat.ShaderCodeTypeFilesMap.GetKeys(PlatforomShaderTypeNames);
					for(const auto& Name:PlatforomShaderTypeNames)
					{
						CurrentPlatforomShaderTypeNames.AddUnique(*Name);
					}
				}

#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION > 25
				TArray<SHADER_COOKER_CLASS::FShaderFormatDescriptor> ShaderFormatsWithStableKeys = UFlibShaderCodeLibraryHelper::GetShaderFormatsWithStableKeys(CurrentPlatforomShaderTypeNames);
#else
				TArray<TPair<FName, bool>> ShaderFormatsWithStableKeys;
				for (FName& Format : ShaderFormatNames)
				{
					ShaderFormatsWithStableKeys.Push(MakeTuple(Format, true));
				}
#endif
				
				SHADER_COOKER_CLASS::CookShaderFormats(ShaderFormatsWithStableKeys);
				FShaderCodeFormatMap::FShaderFormatNameFiles ShaderFormatNameFiles =  *ShaderCodeFoemat.ShaderCodeTypeFilesMap.Find(ShaderFormatName.ToString());
				for(const auto& File:ShaderFormatNameFiles.Files)
				{
					FString Path = FPaths::GetPath(File);
					FShaderCodeLibrary::OpenLibrary(ShaderFormatNameFiles.ShaderName,Path);
				}
				if(!UFlibShaderCodeLibraryHelper::SaveShaderLibrary(ShaderCodeFoemat.Platform,NULL, FApp::GetProjectName(),ShaderCodeFoemat.SaveBaseDir))
				{
					UE_LOG(LogHotPatcherEditorHelper,Display,TEXT("SaveShaderLibrary %s for %s Failed!"),*FApp::GetProjectName(),*ShaderCodeFoemat.Platform->PlatformName() );
				}
				SHADER_COOKER_CLASS::Shutdown();
			}
		}
	}
}

void FMergeShaderCollectionProxy::Shutdown()
{
	FShaderCodeLibrary::Shutdown();
}

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

bool UFlibShaderCodeLibraryHelper::SaveShaderLibrary(const ITargetPlatform* TargetPlatform, const TArray<TSet<FName>>* ChunkAssignments, FString const& Name, const FString&
                                                     SaveBaseDir)
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
		FString TargetPlatformName = TargetPlatform->PlatformName();
		TArray<FString> PlatformSCLCSVPaths;// = OutSCLCSVPaths.FindOrAdd(FName(TargetPlatformName));
		
#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION > 25
	#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION > 26
		FString ErrorString;
		bSaved = SHADER_COOKER_CLASS::SaveShaderLibraryWithoutChunking(TargetPlatform, FApp::GetProjectName(), ShaderCodeDir, RootMetaDataPath, PlatformSCLCSVPaths, ErrorString);
	#else
		bSaved = FShaderCodeLibrary::SaveShaderCode(ShaderCodeDir, RootMetaDataPath, ShaderFormats, PlatformSCLCSVPaths, ChunkAssignments);
	#endif
#else
		bSaved = FShaderCodeLibrary::SaveShaderCodeMaster(ShaderCodeDir, RootMetaDataPath, ShaderFormats, PlatformSCLCSVPaths);
#endif	
	}
	return bSaved;
}

TArray<FString> UFlibShaderCodeLibraryHelper::FindCookedShaderLibByShaderFrmat(const FString& ShaderFormatName,const FString& Directory)
{
	TArray<FString> result;
	TArray<FString > FoundShaderFiles;
	IFileManager::Get().FindFiles(FoundShaderFiles,*Directory,TEXT("ushaderbytecode"));

	FString FormatExtersion = FString::Printf(TEXT("%s.ushaderbytecode"),*ShaderFormatName);
	for(const auto& ShaderFile:FoundShaderFiles)
	{
		if(ShaderFile.EndsWith(FormatExtersion))
		{
			result.Add(ShaderFile);
		}
	}
	return result;
}
TArray<FString> UFlibShaderCodeLibraryHelper::FindCookedShaderLibByPlatform(const FString& PlatfomName,const FString& Directory, bool bRecursive)
{
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
		FoundFiles.Append(GetMetalShaderFormatLambda(Directory,TEXT("metallib"),bRecursive));
		FoundFiles.Append(GetMetalShaderFormatLambda(Directory,TEXT("metalmap"),bRecursive));
	}
	
	if(!FoundFiles.Num())
	{
		FoundFiles.Append(GetMetalShaderFormatLambda(Directory,TEXT("ushaderbytecode"),bRecursive));
	}
	for(auto& File:FoundFiles)
	{
		File = FPaths::Combine(Directory,File);
	}
	return FoundFiles;
}


