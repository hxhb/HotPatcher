// Fill out your copyright notice in the Description page of Project Settings.


#include "ShaderPatch/FlibShaderCodeLibraryHelper.h"
#include "IPlatformFileSandboxWrapper.h"
#include "Interfaces/IPluginManager.h"

#define REMAPPED_PLUGINS TEXT("RemappedPlugins")

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

FString UFlibShaderCodeLibraryHelper::ConvertToFullSandboxPath(const FString& FileName, bool bForWrite)
{
	TUniquePtr<FSandboxPlatformFile> SandboxFileIns = FSandboxPlatformFile::Create(false);

	FString Result;
	if (bForWrite)
	{
		TArray<TSharedRef<IPlugin>> PluginsToRemap = IPluginManager::Get().GetEnabledPlugins();
		// Ideally this would be in the Sandbox File but it can't access the project or plugin
		if (PluginsToRemap.Num() > 0)
		{
			// Handle remapping of plugins
			for (TSharedRef<IPlugin> Plugin : PluginsToRemap)
			{
				// If these match, then this content is part of plugin that gets remapped when packaged/staged
				if (FileName.StartsWith(Plugin->GetContentDir()))
				{
					FString SearchFor;
					SearchFor /= Plugin->GetName() / TEXT("Content");
					int32 FoundAt = FileName.Find(SearchFor, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
					check(FoundAt != -1);
					// Strip off everything but <PluginName/Content/<remaing path to file>
					FString SnippedOffPath = FileName.RightChop(FoundAt);
					// Put this is in <sandbox path>/RemappedPlugins/<PluginName>/Content/<remaing path to file>
					FString RemappedPath = SandboxFileIns->GetSandboxDirectory();
					RemappedPath /= REMAPPED_PLUGINS;
					Result = RemappedPath / SnippedOffPath;
					return Result;
				}
			}
		}
		Result = SandboxFileIns->ConvertToAbsolutePathForExternalAppForWrite(*FileName);
	}
	else
	{
		Result = SandboxFileIns->ConvertToAbsolutePathForExternalAppForRead(*FileName);
	}

	/*if ( IsCookingDLC() )
	{
		check( IsCookByTheBookRunning() );
		Result.ReplaceInline(TEXT("/Cooked/"), *FString::Printf(TEXT("/CookedDLC_%s/"), *CookByTheBookOptions->DlcName) );
	}*/
	return Result;
}

FString UFlibShaderCodeLibraryHelper::ConvertToFullSandboxPath( const FString &FileName, bool bForWrite, const FString& PlatformName )
{
	FString Result = ConvertToFullSandboxPath( FileName, bForWrite );
	Result.ReplaceInline(TEXT("[Platform]"), *PlatformName);
	return Result;
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


