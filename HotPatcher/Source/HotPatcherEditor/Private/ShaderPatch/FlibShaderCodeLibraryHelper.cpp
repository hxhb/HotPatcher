// Fill out your copyright notice in the Description page of Project Settings.


#include "ShaderPatch/FlibShaderCodeLibraryHelper.h"

#include "FlibHotPatcherEditorHelper.h"
#include "IPlatformFileSandboxWrapper.h"
#include "Interfaces/IPluginManager.h"

#define REMAPPED_PLUGINS TEXT("RemappedPlugins")

FCookShaderCollectionProxy::FCookShaderCollectionProxy(const FString& InPlatformName,const FString& InLibraryName,bool InIsNative,const FString& InSaveBaseDir)
:PlatformName(InPlatformName),LibraryName(InLibraryName),bIsNative(InIsNative),SaveBaseDir(InSaveBaseDir){}

FCookShaderCollectionProxy::~FCookShaderCollectionProxy(){}

void FCookShaderCollectionProxy::Init()
{
	SHADER_COOKER_CLASS::InitForCooking(bIsNative);
	TargetPlatform =  UFlibHotPatcherEditorHelper::GetPlatformByName(PlatformName);
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
	FShaderCodeLibrary::OpenLibrary(LibraryName,TEXT(""));
}

void FCookShaderCollectionProxy::Shutdown()
{
	bSuccessed = UFlibShaderCodeLibraryHelper::SaveShaderLibrary(TargetPlatform,NULL, LibraryName,SaveBaseDir);
#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION <= 26
	// UE 4.27 and later,FShaderCodeLibrary::SaveShaderCode will call PackageNativeShaderLibrary
	if(bIsNative)
	{
		FString ShaderCodeDir = FPaths::Combine(SaveBaseDir,PlatformName);
		bSuccessed = bSuccessed && FShaderCodeLibrary::PackageNativeShaderLibrary(ShaderCodeDir,UFlibShaderCodeLibraryHelper::GetShaderFormatsByTargetPlatform(TargetPlatform));
	}
#endif
	FShaderCodeLibrary::CloseLibrary(LibraryName);
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

TArray<FString> UFlibShaderCodeLibraryHelper::FindCookedShaderLibByPlatform(const FString& PlatfomName,const FString& Directory)
{
	TArray<FString> FoundFiles;
	
	if(PlatfomName.StartsWith(TEXT("IOS"),ESearchCase::IgnoreCase) || PlatfomName.StartsWith(TEXT("Mac"),ESearchCase::IgnoreCase))
	{
		auto GetMetalShaderFormatLambda = [](const FString& Directory,const FString& Extersion)
		{
			TArray<FString> FoundMetallibFiles;
			IFileManager::Get().FindFiles(FoundMetallibFiles,*Directory,*Extersion);
			return FoundMetallibFiles;
		};
		FoundFiles.Append(GetMetalShaderFormatLambda(Directory,TEXT("metallib")));
		FoundFiles.Append(GetMetalShaderFormatLambda(Directory,TEXT("metalmap")));
	}
	
	if(!FoundFiles.Num())
	{
		IFileManager::Get().FindFiles(FoundFiles,*Directory,TEXT("ushaderbytecode"));
	}
	for(auto& File:FoundFiles)
	{
		File = FPaths::Combine(Directory,File);
	}
	return FoundFiles;
}

bool UFlibShaderCodeLibraryHelper::PackageNativeShaderLibrary(const FString& ShaderCodeDir,
	const TArray<FName>& ShaderFormats)
{
	return FShaderCodeLibrary::PackageNativeShaderLibrary(ShaderCodeDir,ShaderFormats);
}

TArray<FName> UFlibShaderCodeLibraryHelper::GetShaderFormatsByPlatformName(const FString& PlatfornName)
{
	ITargetPlatform* TargetPlatform =  UFlibHotPatcherEditorHelper::GetPlatformByName(PlatfornName);
	TArray<FName> ShaderFormats = UFlibShaderCodeLibraryHelper::GetShaderFormatsByTargetPlatform(TargetPlatform);
	return ShaderFormats;
}


