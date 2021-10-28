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
	TArray<SHADER_COOKER_CLASS::FShaderFormatDescriptor> ShaderFormatsWithStableKeys = UFlibShaderCodeLibraryHelper::GetShaderFormatsWithStableKeys(ShaderFormats);
	if (ShaderFormats.Num() > 0)
	{
		SHADER_COOKER_CLASS::CookShaderFormats(ShaderFormatsWithStableKeys);
	}
	FShaderCodeLibrary::OpenLibrary(LibraryName,TEXT(""));
}

void FCookShaderCollectionProxy::Shutdown()
{
	UFlibShaderCodeLibraryHelper::SaveShaderLibrary(TargetPlatform,NULL, LibraryName,SaveBaseDir);

	FShaderCodeLibrary::CloseLibrary(LibraryName);
	FShaderCodeLibrary::Shutdown();
}


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

void UFlibShaderCodeLibraryHelper::SaveShaderLibrary(const ITargetPlatform* TargetPlatform, const TArray<TSet<FName>>* ChunkAssignments, FString const& Name, const FString&
                                                     SaveBaseDir)
{
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
		bool bSaved = FShaderCodeLibrary::SaveShaderCode(ShaderCodeDir, RootMetaDataPath, ShaderFormats, PlatformSCLCSVPaths, ChunkAssignments);
	}
}

TArray<FString> UFlibShaderCodeLibraryHelper::FindCookedShaderLibByPlatform(const FString& PlatfomName,const FString& Directory)
{
	TArray<FString> FoundFiles;
	
	if(PlatfomName.StartsWith(TEXT("IOS"),ESearchCase::IgnoreCase) || PlatfomName.StartsWith(TEXT("Mac"),ESearchCase::IgnoreCase))
	{
		IFileManager::Get().FindFiles(FoundFiles,*Directory,TEXT("metallib"));
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


