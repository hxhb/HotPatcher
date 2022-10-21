#include "Chunker/HPLProxy.h"
#include "CreatePatch/ScopedSlowTaskContext.h"
#include "FHotPatcherVersion.h"
#include "FlibPatchParserHelper.h"
#include "HotPatcherLog.h"

// engine header
#include "CommandletHelper.h"
#include "FlibHotPatcherCoreHelper.h"
#include "Misc/DateTime.h"
#include "CoreGlobals.h"
#include "HotPatcherSettings.h"
#include "Cooker/MultiCooker/FlibHotCookerHelper.h"
#include "Logging/LogMacros.h"
#include "CreatePatch/HotPatcherContext.h"
#include "Settings/ProjectPackagingSettings.h"
#include "ShaderPatch/FlibShaderCodeLibraryHelper.h"

#define LOCTEXT_NAMESPACE "UExportRelease"


void UHPLProxy::Init(FPatcherEntitySettingBase* InSetting)
{
	Super::Init(InSetting);
	UHotPatcherSettings* Settings = GetMutableDefault<UHotPatcherSettings>();
	TArray<FString> TempDirs;
	TempDirs.Add(UFlibPatchParserHelper::ReplaceMarkPath(Settings->TempStagedBuildsDir));
	TempDirs.Add(UFlibPatchParserHelper::ReplaceMarkPath(Settings->HPLPakSettings.GetSavePath()));
	for(const auto& TempDir:TempDirs)
	{
		if(FPaths::DirectoryExists(TempDir))
		{
			IFileManager::Get().Delete(*TempDir);
		}
	}
}

bool UHPLProxy::DoExport()
{
	bool bRet = true;
	OnCookAndPakHPL();
	return bRet;
}


void UHPLProxy::OnCookAndPakHPL()
{
	UE_LOG(LogHotPatcher,Log,TEXT("On CookAndPakHPL.."));
	// -TargetPlatform=IOS
	TArray<ETargetPlatform> TargetPlatforms = CommandletHelper::GetCookCommandletTargetPlatforms();
	
	UHotPatcherSettings* Settings = GetMutableDefault<UHotPatcherSettings>();

	TArray<FChunkInfo> HPLChunks = UFlibHotPatcherCoreHelper::GetHPLChunkInfos(Settings->GetHPLSearchPaths());

	FString TempDir = UFlibPatchParserHelper::ReplaceMarkPath(Settings->HPLPakSettings.SavePath.Path);
	for(const auto& LabelChunk:HPLChunks)
	{
		UFlibHotPatcherCoreHelper::CookAndPakChunk(LabelChunk,TargetPlatforms);
		if(Settings->bCopyToStagedBuilds)
		{
			for(const auto& Platform:TargetPlatforms)
			{
				FString PlatformName = THotPatcherTemplateHelper::GetEnumNameByValue(Platform);
				FHotPatcherVersion PakVersion;
				PakVersion.VersionId = LabelChunk.ChunkName;
				PakVersion.BaseVersionId = TEXT("");
				const FString ChunkPakName = UFlibHotPatcherCoreHelper::MakePakShortName(PakVersion,LabelChunk,PlatformName,Settings->HPLPakSettings.GetPakNameRegular())  + TEXT(".pak");
				FString PakPath = FPaths::Combine(TempDir,LabelChunk.ChunkName,PlatformName,ChunkPakName);
				
				ITargetPlatform* TargetPlatform = UFlibHotPatcherCoreHelper::GetPlatformByName(PlatformName);
				FString ToStageBuildPath = UFlibPatchParserHelper::ReplaceMarkPath(Settings->TempStagedBuildsDir);
				ToStageBuildPath = FPaths::Combine(ToStageBuildPath,ChunkPakName);
				
				if(FPaths::FileExists(PakPath))
				{
					if(FPaths::FileExists(ToStageBuildPath))
					{
						IFileManager::Get().Delete(*ToStageBuildPath);
					}
					IFileManager::Get().Copy(*ToStageBuildPath,*PakPath);
				}
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
