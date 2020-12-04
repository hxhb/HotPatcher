#include "CreatePatch/PatcherProxy.h"
#include "HotPatcherLog.h"
#include "ThreadUtils/FThreadUtils.hpp"

// engine header

#include "CoreGlobals.h"
#include "FlibHotPatcherEditorHelper.h"
#include "Dom/JsonValue.h"
#include "HAL/PlatformFilemanager.h"
#include "Kismet/KismetStringLibrary.h"
#include "PakFileUtilities.h"
#include "CreatePatch/ScopedSlowTaskContext.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/FileHelper.h"
#include <Templates/Function.h>
#define LOCTEXT_NAMESPACE "HotPatcherProxy"

UPatcherProxy::UPatcherProxy(const FObjectInitializer& ObjectInitializer):Super(ObjectInitializer)
{
	bCommandlet = ::IsRunningCommandlet();
}

bool UPatcherProxy::CheckSelectedAssetsCookStatus(const TArray<FString>& PlatformNames, const FAssetDependenciesInfo& SelectedAssets, FString& OutMsg)const
{
	OutMsg.Empty();
	// 检查所修改的资源是否被Cook过
	for (const auto& PlatformName : PlatformNames)
	{
			TArray<FAssetDetail> ValidCookAssets;
			TArray<FAssetDetail> InvalidCookAssets;

			UFlibHotPatcherEditorHelper::CheckInvalidCookFilesByAssetDependenciesInfo(UKismetSystemLibrary::GetProjectDirectory(), PlatformName, SelectedAssets, ValidCookAssets, InvalidCookAssets);

			if (InvalidCookAssets.Num() > 0)
			{
				OutMsg.Append(FString::Printf(TEXT("%s UnCooked Assets:\n"), *PlatformName));

				for (const auto& Asset : InvalidCookAssets)
				{
					FString AssetLongPackageName;
					UFLibAssetManageHelperEx::ConvPackagePathToLongPackageName(Asset.mPackagePath, AssetLongPackageName);
					OutMsg.Append(FString::Printf(TEXT("\t%s\n"), *AssetLongPackageName));
				}
			}
	}

	return OutMsg.IsEmpty();
}



bool UPatcherProxy::CheckPatchRequire(const FPatchVersionDiff& InDiff,FString& OutMsg)const
{
	bool Status = false;
	// 错误处理
	{
		FString GenErrorMsg;
		FAssetDependenciesInfo AllChangedAssetInfo = UFLibAssetManageHelperEx::CombineAssetDependencies(InDiff.AssetDiffInfo.AddAssetDependInfo, InDiff.AssetDiffInfo.ModifyAssetDependInfo);
		bool bSelectedCookStatus = CheckSelectedAssetsCookStatus(const_cast<UPatcherProxy*>(this)->GetSettingObject()->GetPakTargetPlatformNames(), AllChangedAssetInfo, GenErrorMsg);

		// 如果有错误信息 则输出后退出
		if (!bSelectedCookStatus)
		{
			OutMsg = GenErrorMsg;
			Status = false;
		}
		else
		{
			OutMsg = TEXT("");
			Status = true;
		}
	}
	return Status;
}

bool UPatcherProxy::SavePatchVersionJson(const FHotPatcherVersion& InSaveVersion, const FString& InSavePath, FPakVersion& OutPakVersion)
{
	bool bStatus = false;
	OutPakVersion = FExportPatchSettings::GetPakVersion(InSaveVersion, FDateTime::UtcNow().ToString());
	{
		if (GetSettingObject()->IsIncludePakVersion())
		{
			FString SavePakVersionFilePath = FExportPatchSettings::GetSavePakVersionPath(InSavePath, InSaveVersion);

			FString OutString;
			if (UFlibPakHelper::SerializePakVersionToString(OutPakVersion, OutString))
			{
				bStatus = UFLibAssetManageHelperEx::SaveStringToFile(SavePakVersionFilePath, OutString);
			}
		}
	}
	return bStatus;
}

bool UPatcherProxy::SavePatchDiffJson(const FHotPatcherVersion& InSaveVersion, const FPatchVersionDiff& InDiff)
{
	bool bStatus = false;
	if (GetSettingObject()->IsSaveDiffAnalysis())
	{
		

		auto SerializeChangedAssetInfo = [](const FPatchVersionDiff& InAssetInfo)->FString
		{
			FString AddAssets;
			UFlibPatchParserHelper::TSerializeStructAsJsonString(InAssetInfo,AddAssets);
			return AddAssets;
		};
		
		FString SerializeDiffInfo = SerializeChangedAssetInfo(InDiff);
		
			FString::Printf(TEXT("%s"),*SerializeDiffInfo
			);


		FString SaveDiffToFile = FPaths::Combine(
			GetSettingObject()->GetCurrentVersionSavePath(),
			FString::Printf(TEXT("%s_%s_Diff.json"), *InSaveVersion.BaseVersionId, *InSaveVersion.VersionId)
		);
		if (UFLibAssetManageHelperEx::SaveStringToFile(SaveDiffToFile, SerializeDiffInfo))
		{
			bStatus = true;

			FString Msg = FString::Printf(TEXT("Succeed to export New Patch Diff Info."),*SaveDiffToFile);
			OnPaking.Broadcast(TEXT("SavePatchDiffInfo"),Msg);
		}
	}
	return bStatus;
}


FProcHandle UPatcherProxy::DoUnrealPak(TArray<FString> UnrealPakOptions, bool block)
{
	FString UnrealPakBinary = UFlibPatchParserHelper::GetUnrealPakBinary();

	FString CommandLine;
	for (const auto& Option : UnrealPakOptions)
	{
		CommandLine.Append(FString::Printf(TEXT(" %s"), *Option));
	}

	// create UnrealPak process

	uint32 *ProcessID = NULL;
	FProcHandle ProcHandle = FPlatformProcess::CreateProc(*UnrealPakBinary, *CommandLine, true, false, false, ProcessID, 0, NULL, NULL, NULL);

	if (ProcHandle.IsValid())
	{
		if (block)
		{
			FPlatformProcess::WaitForProc(ProcHandle);
		}
	}
	return ProcHandle;
}


bool UPatcherProxy::SavePakCommands(const FString& InPlatformName, const FPatchVersionDiff& InDiffInfo, const FString& InSavePath)
{
	bool bStatus = false;
	// combine all pak commands
	{
		FString ProjectDir = UKismetSystemLibrary::GetProjectDirectory();

		// generated cook command form asset list
		TArray<FString> OutPakCommand = GetSettingObject()->MakeAllPakCommandsByTheSetting(InPlatformName, InDiffInfo, GetSettingObject()->IsEnableExternFilesDiff());

		// save paklist to file
		if (FFileHelper::SaveStringArrayToFile(OutPakCommand, *InSavePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
		{
			if (GetSettingObject()->IsSavePakList())
			{
				// auto Msg = LOCTEXT("SavePatchPakCommand", "Succeed to export the Patch Packaghe Pak Command.");
				// UFlibHotPatcherEditorHelper::CreateSaveFileNotify(Msg, InSavePath);
				FString Msg = FString::Printf(TEXT("Succeed to export the Patch Packaghe Pak Command to %s."),*InSavePath);
				OnPaking.Broadcast(TEXT("SavePatchDiffInfo"),Msg);
				
				bStatus = true;
			}
		}
	}
	return bStatus;
}


FHotPatcherVersion UPatcherProxy::MakeNewRelease(const FHotPatcherVersion& InBaseVersion, const FHotPatcherVersion& InCurrentVersion, FExportPatchSettings* InPatchSettings)const
{
	FHotPatcherVersion BaseVersion = InBaseVersion;
	FHotPatcherVersion NewRelease = InCurrentVersion;
	FPatchVersionDiff DiffInfo = UFlibPatchParserHelper::DiffPatchVersionWithPatchSetting(*GetSettingObject(),BaseVersion, InCurrentVersion);

	FAssetDependenciesInfo& BaseAssetInfoRef = BaseVersion.AssetInfo;
	// TMap<FString, FExternFileInfo>& BaseExternalFilesRef = BaseVersion.ExternalFiles;
	TMap<ETargetPlatform,FPlatformExternAssets>& BasePlatformAssetsRef = BaseVersion.PlatformAssets;

	// Modify Asset
	auto DeleteOldAssetLambda = [&BaseAssetInfoRef](const FAssetDependenciesInfo& InAssetDependenciesInfo)
	{
		for (const auto& AssetsModulePair : InAssetDependenciesInfo.AssetsDependenciesMap)
		{
			FAssetDependenciesDetail* NewReleaseModuleAssets = BaseAssetInfoRef.AssetsDependenciesMap.Find(AssetsModulePair.Key);

			for (const auto& NeedDeleteAsset : AssetsModulePair.Value.AssetDependencyDetails)
			{
				if (NewReleaseModuleAssets && NewReleaseModuleAssets->AssetDependencyDetails.Contains(NeedDeleteAsset.Key))
				{
					NewReleaseModuleAssets->AssetDependencyDetails.Remove(NeedDeleteAsset.Key);
				}
			}
		}
	};
	
	DeleteOldAssetLambda(DiffInfo.AssetDiffInfo.ModifyAssetDependInfo);
	if(InPatchSettings && !InPatchSettings->IsSaveDeletedAssetsToNewReleaseJson())
	{
		DeleteOldAssetLambda(DiffInfo.AssetDiffInfo.DeleteAssetDependInfo);
	}

	// Add Asset
	BaseAssetInfoRef = UFLibAssetManageHelperEx::CombineAssetDependencies(BaseAssetInfoRef, DiffInfo.AssetDiffInfo.AddAssetDependInfo);
	// modify Asset
	BaseAssetInfoRef = UFLibAssetManageHelperEx::CombineAssetDependencies(BaseAssetInfoRef, DiffInfo.AssetDiffInfo.ModifyAssetDependInfo);
	NewRelease.AssetInfo = BaseAssetInfoRef;

	// // external files
	// auto RemoveOldExternalFilesLambda = [&BaseExternalFilesRef](const TArray<FExternFileInfo>& InFiles)
	// {
	// 	for (const auto& File : InFiles)
	// 	{
	// 		if (BaseExternalFilesRef.Contains(File.FilePath.FilePath))
	// 		{
	// 			BaseExternalFilesRef.Remove(File.FilePath.FilePath);
	// 		}
	// 	}
	// };

	TArray<ETargetPlatform> DiffPlatforms;
	DiffInfo.PlatformExternDiffInfo.GetKeys(DiffPlatforms);

	for(auto Platform:DiffPlatforms)
	{
		FPlatformExternAssets AddPlatformFiles;
		AddPlatformFiles.TargetPlatform = Platform;
		AddPlatformFiles.AddExternFileToPak = DiffInfo.PlatformExternDiffInfo[Platform].AddExternalFiles;
		AddPlatformFiles.AddExternFileToPak.Append(DiffInfo.PlatformExternDiffInfo[Platform].ModifyExternalFiles);
		if(BasePlatformAssetsRef.Contains(Platform))
		{
			for(const auto& File:AddPlatformFiles.AddExternFileToPak)
			{
				if(BasePlatformAssetsRef[Platform].AddExternFileToPak.Contains(File))
				{
					BasePlatformAssetsRef[Platform].AddExternFileToPak.Remove(File);
				}
			}
		}else
		{
			BasePlatformAssetsRef.Add(Platform,AddPlatformFiles);
		}
	}
	// RemoveOldExternalFilesLambda(DiffInfo.ExternDiffInfo.ModifyExternalFiles);
	// DeleteOldExternalFilesLambda(DiffInfo.DeleteExternalFiles);

	// auto AddExternalFilesLambda = [&BaseExternalFilesRef](const TArray<FExternFileInfo>& InFiles)
	// {
	// 	for (const auto& File : InFiles)
	// 	{
	// 		if (!BaseExternalFilesRef.Contains(File.FilePath.FilePath))
	// 		{
	// 			BaseExternalFilesRef.Add(File.FilePath.FilePath,File);
	// 		}
	// 	}
	// };
	// AddExternalFilesLambda(DiffInfo.ExternDiffInfo.AddExternalFiles);
	// AddExternalFilesLambda(DiffInfo.ExternDiffInfo.ModifyExternalFiles);
	// NewRelease.ExternalFiles = BaseExternalFilesRef;

	return NewRelease;
}

bool UPatcherProxy::CanExportPatch()const
{
	bool bCanExport = false;
	FExportPatchSettings* NoConstSettingObject = const_cast<UPatcherProxy*>(this)->GetSettingObject();
	if (NoConstSettingObject)
	{
		bool bHasBase = false;
		if (NoConstSettingObject->IsByBaseVersion())
			bHasBase = !NoConstSettingObject->GetBaseVersion().IsEmpty() && FPaths::FileExists(NoConstSettingObject->GetBaseVersion());
		else
			bHasBase = true;
		bool bHasVersionId = !NoConstSettingObject->GetVersionId().IsEmpty();
		bool bHasFilter = !!NoConstSettingObject->GetAssetIncludeFiltersPaths().Num();
		bool bHasSpecifyAssets = !!NoConstSettingObject->GetIncludeSpecifyAssets().Num();
		bool bHasExternFiles = !!NoConstSettingObject->GetAddExternFiles().Num();
		bool bHasExDirs = !!NoConstSettingObject->GetAddExternDirectory().Num();
		bool bHasSavePath = !NoConstSettingObject->GetSaveAbsPath().IsEmpty();
		bool bHasPakPlatfotm = !!NoConstSettingObject->GetPakTargetPlatforms().Num();

		bool bHasAnyPakFiles = (
			bHasFilter || bHasSpecifyAssets || bHasExternFiles || bHasExDirs ||
			NoConstSettingObject->IsIncludeEngineIni() ||
			NoConstSettingObject->IsIncludePluginIni() ||
			NoConstSettingObject->IsIncludeProjectIni()
			);
		bCanExport = bHasBase && bHasVersionId && bHasAnyPakFiles && bHasPakPlatfotm && bHasSavePath;
	}
	return bCanExport;
}

bool UPatcherProxy::DoExport()
{
	FHotPatcherVersion BaseVersion;

	if (GetSettingObject()->IsByBaseVersion() && !GetSettingObject()->GetBaseVersionInfo(BaseVersion))
	{
		UE_LOG(LogHotPatcher, Error, TEXT("Deserialize Base Version Faild!"));
		return false;
	}
	else
	{
		// 在不进行外部文件diff的情况下清理掉基础版本的外部文件
		if (!GetSettingObject()->IsEnableExternFilesDiff())
		{
			BaseVersion.PlatformAssets.Empty();
		}
	}
	
	UFLibAssetManageHelperEx::UpdateAssetMangerDatabase(true);
	FChunkInfo NewVersionChunk = UFlibHotPatcherEditorHelper::MakeChunkFromPatchSettings(GetSettingObject());

	FHotPatcherVersion CurrentVersion = UFlibPatchParserHelper::ExportReleaseVersionInfoByChunk(
		GetSettingObject()->GetVersionId(),
		BaseVersion.VersionId,
		FDateTime::UtcNow().ToString(),
		NewVersionChunk,
		GetSettingObject()->IsIncludeHasRefAssetsOnly(),
		GetSettingObject()->IsAnalysisFilterDependencies()
	);

	FString CurrentVersionSavePath = GetSettingObject()->GetCurrentVersionSavePath();
	FPatchVersionDiff VersionDiffInfo = UFlibPatchParserHelper::DiffPatchVersionWithPatchSetting(*GetSettingObject(), BaseVersion, CurrentVersion);
	
	FString ReceiveMsg;
	if (!GetSettingObject()->IsCookPatchAssets() && !CheckPatchRequire(VersionDiffInfo, ReceiveMsg))
	{
		OnShowMsg.Broadcast(ReceiveMsg);
		return false;
	}
	
	int32 ChunkNum = GetSettingObject()->IsEnableChunk() ? GetSettingObject()->GetChunkInfos().Num() : 1;
	

	// save pakversion.json
	//FPakVersion CurrentPakVersion;
	//SavePatchVersionJson(CurrentVersion, CurrentVersionSavePath, CurrentPakVersion);

	// package all selected platform
	TMap<FString,TArray<FPakFileInfo>> PakFilesInfoMap;

	// Check Chunk
	if(GetSettingObject()->IsEnableChunk())
	{
		FString TotalMsg;
		FChunkInfo TotalChunk = UFlibPatchParserHelper::CombineChunkInfos(GetSettingObject()->GetChunkInfos());

		FChunkAssetDescribe ChunkDiffInfo = UFlibPatchParserHelper::DiffChunkWithPatchSetting(*GetSettingObject(), NewVersionChunk, TotalChunk);

		TArray<FString> AllUnselectedAssets = ChunkDiffInfo.GetAssetsStrings();
		TArray<FString> AllUnselectedExFiles;
		for(auto Platform:GetSettingObject()->GetPakTargetPlatforms())
		{
			AllUnselectedExFiles.Append(ChunkDiffInfo.GetExFileStrings(Platform));
		}
		
		TArray<FString> UnSelectedInternalFiles = ChunkDiffInfo.GetInternalFileStrings();

		auto ChunkCheckerMsg = [&TotalMsg](const FString& Category,const TArray<FString>& InAssetList)
		{
			if (!!InAssetList.Num())
			{
				TotalMsg.Append(FString::Printf(TEXT("\n%s:\n"),*Category));
				for (const auto& Asset : InAssetList)
				{
					TotalMsg.Append(FString::Printf(TEXT("%s\n"), *Asset));
				}
			}
		};
		ChunkCheckerMsg(TEXT("Unreal Asset"), AllUnselectedAssets);
		ChunkCheckerMsg(TEXT("External Files"), AllUnselectedExFiles);
		ChunkCheckerMsg(TEXT("Internal Files(Patch & Chunk setting not match)"), UnSelectedInternalFiles);

		if (!TotalMsg.IsEmpty())
		{
			OnShowMsg.Broadcast(FString::Printf(TEXT("Unselect in Chunk:\n%s"), *TotalMsg));
			return false;
		}
		else
		{
			OnShowMsg.Broadcast(TEXT(""));
		}
	}
	else
	{
		// 分析所选过滤器中的资源所依赖的过滤器添加到Chunk中
		// 因为默认情况下Chunk的过滤器不会进行依赖分析，当bEnableChunk未开启时，之前导出的Chunk中的过滤器不包含Patch中所选过滤器进行依赖分析之后的所有资源的模块。
		{
			TArray<FString> DependenciesFilters;
		
			auto GetKeysLambda = [&DependenciesFilters](const FAssetDependenciesInfo& Assets)
			{
				TArray<FAssetDetail> AllAssets;
				UFLibAssetManageHelperEx::GetAssetDetailsByAssetDependenciesInfo(Assets, AllAssets);
				for (const auto& Asset : AllAssets)
				{
					FString Path;
					FString Filename;
					FString Extension;
					FPaths::Split(Asset.mPackagePath, Path, Filename, Extension);
					DependenciesFilters.AddUnique(Path);
				}
			};
			GetKeysLambda(VersionDiffInfo.AssetDiffInfo.AddAssetDependInfo);
			GetKeysLambda(VersionDiffInfo.AssetDiffInfo.ModifyAssetDependInfo);

			TArray<FDirectoryPath> DepOtherModule;

			for (const auto& DependenciesFilter : DependenciesFilters)
			{
				if (!!NewVersionChunk.AssetIncludeFilters.Num())
				{
					for (const auto& includeFilter : NewVersionChunk.AssetIncludeFilters)
					{
						if (!includeFilter.Path.StartsWith(DependenciesFilter))
						{
							FDirectoryPath FilterPath;
							FilterPath.Path = DependenciesFilter;
							DepOtherModule.Add(FilterPath);
						}
					}
				}
				else
				{
					FDirectoryPath FilterPath;
					FilterPath.Path = DependenciesFilter;
					DepOtherModule.Add(FilterPath);

				}
			}
			NewVersionChunk.AssetIncludeFilters.Append(DepOtherModule);
		}
	}
	
	bool bEnableChunk = GetSettingObject()->IsEnableChunk();

	TArray<FChunkInfo> PakChunks;
	if (bEnableChunk)
	{
		PakChunks = GetSettingObject()->GetChunkInfos();
	}
	else
	{
		PakChunks.Add(NewVersionChunk);
	}

	float AmountOfWorkProgress = 2.f * (GetSettingObject()->GetPakTargetPlatforms().Num() * ChunkNum) + 4.0f;
	UScopedSlowTaskContext* UnrealPakSlowTask = NewObject<UScopedSlowTaskContext>();
	UnrealPakSlowTask->AddToRoot();
	UnrealPakSlowTask->init(AmountOfWorkProgress);
	
	for(const auto& PlatformName :GetSettingObject()->GetPakTargetPlatformNames())
	{
		if(GetSettingObject()->IsCookPatchAssets())
		{
			ETargetPlatform Platform;
			UFlibPatchParserHelper::GetEnumValueByName(PlatformName,Platform);
			CookChunkAssets(VersionDiffInfo,NewVersionChunk,TArray<ETargetPlatform>{Platform});
		}
		// PakModeSingleLambda(PlatformName, CurrentVersionSavePath);
		for (const auto& Chunk : PakChunks)
		{
			// Update Progress Dialog
			{
				FText Dialog = FText::Format(NSLOCTEXT("ExportPatch", "GeneratedPakCommands", "Generating UnrealPak Commands of {0} Platform Chunk {1}."), FText::FromString(PlatformName),FText::FromString(Chunk.ChunkName));
				OnPaking.Broadcast(TEXT("ExportPatch"),*Dialog.ToString());
				UnrealPakSlowTask->EnterProgressFrame(1.0, Dialog);
			}
			FString ChunkSaveBasePath = FPaths::Combine(GetSettingObject()->SavePath.Path, CurrentVersion.VersionId, PlatformName);
			TArray<FPakFileProxy> PakFileProxys;

			TArray<FPakCommand> ChunkPakCommands = UFlibPatchParserHelper::CollectPakCommandByChunk(VersionDiffInfo, Chunk, PlatformName, GetSettingObject()->GetPakCommandOptions());

			if (!ChunkPakCommands.Num())
			{
				FString Msg = FString::Printf(TEXT("Chunk:%s not contain any file!!!"), *Chunk.ChunkName);
				UE_LOG(LogHotPatcher, Warning, TEXT("%s"),*Msg);
				OnShowMsg.Broadcast(Msg);
				continue;
			}
			
			if(!Chunk.bMonolithic)
			{
				FPakFileProxy SinglePakForChunk;
				SinglePakForChunk.PakCommands = ChunkPakCommands;
				
				const FString ChunkPakName = MakePakShortName(CurrentVersion,Chunk,PlatformName);
				SinglePakForChunk.PakCommandSavePath = FPaths::Combine(ChunkSaveBasePath, FString::Printf(TEXT("%s_PakCommands.txt"), *ChunkPakName));
				SinglePakForChunk.PakSavePath = FPaths::Combine(ChunkSaveBasePath, FString::Printf(TEXT("%s.pak"), *ChunkPakName));
				PakFileProxys.Add(SinglePakForChunk);
			}
			else
			{
				for (const auto& PakCommand : ChunkPakCommands)
				{
					FPakFileProxy CurrentPak;
					CurrentPak.PakCommands.Add(PakCommand);
					FString Path;
					switch (Chunk.MonolithicPathMode)
					{
						case EMonolithicPathMode::MountPath:
						{
							Path = UFlibPatchParserHelper::MountPathToRelativePath(PakCommand.GetMountPath());
							break;

						};
						case  EMonolithicPathMode::PackagePath:
						{
							Path = PakCommand.AssetPackage;
							break;
						}
					}
					
					CurrentPak.PakCommandSavePath = FPaths::Combine(ChunkSaveBasePath, Chunk.ChunkName, FString::Printf(TEXT("%s_PakCommands.txt"), *Path));
					CurrentPak.PakSavePath = FPaths::Combine(ChunkSaveBasePath, Chunk.ChunkName, FString::Printf(TEXT("%s.pak"), *Path));
					PakFileProxys.Add(CurrentPak);
				}
			}

			// Update SlowTask Progress
			{
				FText Dialog = FText::Format(NSLOCTEXT("ExportPatch", "GeneratedPak", "Generating Pak list of {0} Platform Chunk {1}."), FText::FromString(PlatformName), FText::FromString(Chunk.ChunkName));
				OnPaking.Broadcast(TEXT("GeneratedPak"),*Dialog.ToString());
				UnrealPakSlowTask->EnterProgressFrame(1.0, Dialog);
			}

			TArray<FString> UnrealPakOptions = GetSettingObject()->GetUnrealPakOptions();
			TArray<FReplaceText> ReplacePakCommandTexts = GetSettingObject()->GetReplacePakCommandTexts();
			// 创建chunk的pak文件
			for (const auto& PakFileProxy : PakFileProxys)
			{
				++PakCounter;
				FThread CurrentPakThread(*PakFileProxy.PakSavePath, [/*CurrentPakVersion, */PlatformName, UnrealPakOptions, ReplacePakCommandTexts, PakFileProxy, &Chunk, &PakFilesInfoMap,this]()
				{

					bool PakCommandSaveStatus = FFileHelper::SaveStringArrayToFile(
						UFlibPatchParserHelper::GetPakCommandStrByCommands(PakFileProxy.PakCommands, ReplacePakCommandTexts),
						*PakFileProxy.PakCommandSavePath,
						FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);

					if (PakCommandSaveStatus)
					{
						TArray<FString> UnrealPakOptionsSinglePak = UnrealPakOptions;
						UnrealPakOptionsSinglePak.Add(
							FString::Printf(
								TEXT("%s -create=%s"),
								*(TEXT("\"") + PakFileProxy.PakSavePath + TEXT("\"")),
								*(TEXT("\"") + PakFileProxy.PakCommandSavePath + TEXT("\""))
							)
						);
						FString CommandLine;
						for (const auto& Option : UnrealPakOptionsSinglePak)
						{
							CommandLine.Append(FString::Printf(TEXT(" %s"), *Option));
						}
						ExecuteUnrealPak(*CommandLine);
						// FProcHandle ProcessHandle = UFlibPatchParserHelper::DoUnrealPak(UnrealPakOptionsSinglePak, true);

						if (FPaths::FileExists(PakFileProxy.PakSavePath))
						{
							if(IsRunningCommandlet())
							{
								FString Msg = FString::Printf(TEXT("Successd to Package the patch as %s."),*PakFileProxy.PakSavePath);
								OnPaking.Broadcast(TEXT("SavedPakFile"),Msg);
							}else
							{
								FText Msg = LOCTEXT("SavedPakFileMsg", "Successd to Package the patch as Pak.");
								UFlibHotPatcherEditorHelper::CreateSaveFileNotify(Msg, PakFileProxy.PakSavePath);
							}
							
							
							FPakFileInfo CurrentPakInfo;
							if (UFlibPatchParserHelper::GetPakFileInfo(PakFileProxy.PakSavePath, CurrentPakInfo))
							{
								// CurrentPakInfo.PakVersion = CurrentPakVersion;
								if (!PakFilesInfoMap.Contains(PlatformName))
								{
									PakFilesInfoMap.Add(PlatformName, TArray<FPakFileInfo>{CurrentPakInfo});
								}
								else
								{
									PakFilesInfoMap.Find(PlatformName)->Add(CurrentPakInfo);
								}
							}
						}
						if (!Chunk.bSavePakCommands)
						{
							IFileManager::Get().Delete(*PakFileProxy.PakCommandSavePath);
						}
					}

				});
				CurrentPakThread.Run();
			}
		}
	}

	// delete pakversion.json
	//{
	//	FString PakVersionSavedPath = FExportPatchSettingsEx::GetSavePakVersionPath(CurrentVersionSavePath,CurrentVersion);
	//	if (GetSettingObject()->IsIncludePakVersion() && FPaths::FileExists(PakVersionSavedPath))
	//	{
	//		IFileManager::Get().Delete(*PakVersionSavedPath);
	//	}
	//}



	// save asset dependency
	{
		if (GetSettingObject()->IsSaveAssetRelatedInfo() && GetPakCounter())
		{
			TArray<EAssetRegistryDependencyTypeEx> AssetDependencyTypes;
			AssetDependencyTypes.Add(EAssetRegistryDependencyTypeEx::Packages);

			TArray<FHotPatcherAssetDependency> AssetsDependency = UFlibPatchParserHelper::GetAssetsRelatedInfoByFAssetDependencies(
				UFLibAssetManageHelperEx::CombineAssetDependencies(VersionDiffInfo.AssetDiffInfo.AddAssetDependInfo, VersionDiffInfo.AssetDiffInfo.ModifyAssetDependInfo),
				AssetDependencyTypes
			);

			FString AssetsDependencyString = UFlibPatchParserHelper::SerializeAssetsDependencyAsJsonString(AssetsDependency);
			
			FString SaveAssetRelatedInfoToFile = FPaths::Combine(
				CurrentVersionSavePath,
				FString::Printf(TEXT("%s_AssetRelatedInfos.json"), *CurrentVersion.VersionId)
			);
			if (UFLibAssetManageHelperEx::SaveStringToFile(SaveAssetRelatedInfoToFile, AssetsDependencyString))
			{
				if(IsRunningCommandlet())
				{
					FString Msg = FString::Printf(TEXT("Succeed to export Asset Related infos:%s."),*SaveAssetRelatedInfoToFile);
					OnPaking.Broadcast(TEXT("SaveAssetRelatedInfo"),Msg);
				}
				else
				{
					auto Msg = LOCTEXT("SaveAssetRelatedInfo", "Succeed to export Asset Related infos.");
					UFlibHotPatcherEditorHelper::CreateSaveFileNotify(Msg, SaveAssetRelatedInfoToFile);
				}
			}
		}
	}

	// save difference to file
	if(GetPakCounter())
	{
		FText DiaLogMsg = FText::Format(NSLOCTEXT("ExportPatch", "ExportPatchDiffFile", "Generating Diff info of version {0}"), FText::FromString(CurrentVersion.VersionId));
		SavePatchDiffJson(CurrentVersion, VersionDiffInfo);
		if(IsRunningCommandlet())
		{
			OnPaking.Broadcast(TEXT("ExportPatchDiffFile"),*DiaLogMsg.ToString());
			
		}else
		{
			UnrealPakSlowTask->EnterProgressFrame(1.0, DiaLogMsg);
		}
	}

	// save Patch Tracked asset info to file
	if(GetPakCounter())
	{
		FText DiaLogMsg = FText::Format(NSLOCTEXT("ExportPatch", "ExportPatchAssetInfo", "Generating Patch Tacked Asset info of version {0}"), FText::FromString(CurrentVersion.VersionId));
		if(IsRunningCommandlet())
		{
			FString Msg = FString::Printf(TEXT("Generating Patch Tacked Asset info of version %s."),*CurrentVersion.VersionId);
			OnPaking.Broadcast(TEXT("ExportPatchAssetInfo"),DiaLogMsg.ToString());
		}
		else
		{
			UnrealPakSlowTask->EnterProgressFrame(1.0, DiaLogMsg);
		}
		
		FString SerializeReleaseVersionInfo;
		FHotPatcherVersion NewReleaseVersion = MakeNewRelease(BaseVersion, CurrentVersion,GetSettingObject());
		UFlibPatchParserHelper::TSerializeStructAsJsonString(NewReleaseVersion, SerializeReleaseVersionInfo);

		FString SaveCurrentVersionToFile = FPaths::Combine(
			CurrentVersionSavePath,
			FString::Printf(TEXT("%s_Release.json"), *CurrentVersion.VersionId)
		);
		if (UFLibAssetManageHelperEx::SaveStringToFile(SaveCurrentVersionToFile, SerializeReleaseVersionInfo))
		{
			if(IsRunningCommandlet())
			{
				FString Msg = FString::Printf(TEXT("Succeed to export New Release Info to %s."),*SaveCurrentVersionToFile);
				OnPaking.Broadcast(TEXT("SavePatchDiffInfo"),Msg);
			}else
			{
				auto Msg = LOCTEXT("SavePatchDiffInfo", "Succeed to export New Release Info.");
				UFlibHotPatcherEditorHelper::CreateSaveFileNotify(Msg, SaveCurrentVersionToFile);
			}
		}
	}

	// serialize all pak file info
	if(GetPakCounter())
	{
		FText DiaLogMsg = FText::Format(NSLOCTEXT("ExportPatch", "ExportPatchPakFileInfo", "Generating All Platform Pak info of version {0}"), FText::FromString(CurrentVersion.VersionId));
		if(IsRunningCommandlet())
		{
			OnPaking.Broadcast(TEXT("ExportPatchPakFileInfo"),*DiaLogMsg.ToString());
		}
		else
		{
			UnrealPakSlowTask->EnterProgressFrame(1.0,DiaLogMsg);
		}
		FString PakFilesInfoStr;
		UFlibPatchParserHelper::SerializePlatformPakInfoToString(PakFilesInfoMap, PakFilesInfoStr);

		if (!PakFilesInfoStr.IsEmpty())
		{
			FString SavePakFilesPath = FPaths::Combine(
				CurrentVersionSavePath,
				FString::Printf(TEXT("%s_PakFilesInfo.json"), *CurrentVersion.VersionId)
			);
			if (UFLibAssetManageHelperEx::SaveStringToFile(SavePakFilesPath, PakFilesInfoStr) && FPaths::FileExists(SavePakFilesPath))
			{
				if(IsRunningCommandlet())
				{
					FString Msg = FString::Printf(TEXT("Successd to Export the Pak File info to ."),*SavePakFilesPath);
					OnPaking.Broadcast(TEXT("SavedPakFileMsg"),Msg);
				}else
				{
					FText Msg = LOCTEXT("SavedPakFileMsg", "Successd to Export the Pak File info.");
					UFlibHotPatcherEditorHelper::CreateSaveFileNotify(Msg, SavePakFilesPath);
				}
			}
		}
	}

	// serialize patch config
	if(GetPakCounter())
	{
		FText DiaLogMsg = FText::Format(NSLOCTEXT("ExportPatch", "ExportPatchConfig", "Generating Current Patch Config of version {0}"), FText::FromString(CurrentVersion.VersionId));
		if(IsRunningCommandlet())
		{
			OnPaking.Broadcast(TEXT("ExportPatchConfig"),*DiaLogMsg.ToString());	
		}
		else
		{	
			UnrealPakSlowTask->EnterProgressFrame(1.0, DiaLogMsg);
		}

		FString SaveConfigPath = FPaths::Combine(
			CurrentVersionSavePath,
			FString::Printf(TEXT("%s_PatchConfig.json"),*CurrentVersion.VersionId)
		);

		if (GetSettingObject()->IsSavePatchConfig())
		{
			FString SerializedJsonStr;
			UFlibPatchParserHelper::TSerializeStructAsJsonString(*GetSettingObject(),SerializedJsonStr);
			if (FFileHelper::SaveStringToFile(SerializedJsonStr, *SaveConfigPath))
			{
				if(IsRunningCommandlet())
				{
					FString Msg = FString::Printf(TEXT("Successd to Export the Patch Config to %s."),*SaveConfigPath);
					OnPaking.Broadcast(TEXT("SavedPatchConfig"),Msg);
				}else
				{
					FText Msg = LOCTEXT("SavedPatchConfig", "Successd to Export the Patch Config.");
					UFlibHotPatcherEditorHelper::CreateSaveFileNotify(Msg, SaveConfigPath);
				}
			}
		}
	}

	if (!GetPakCounter())
	{
		UE_LOG(LogHotPatcher, Error, TEXT("The Patch not contain any invalie file!"));
		OnShowMsg.Broadcast(TEXT("The Patch not contain any invalie file!"));
	}
	UnrealPakSlowTask->Final();
	return true;
}

FString UPatcherProxy::MakePakShortName(const FHotPatcherVersion& InCurrentVersion, const FChunkInfo& InChunkInfo, const FString& InPlatform)
{
	struct FResularOperator
	{
		FResularOperator(const FString& InName,TFunction<FString(void)> InOperator)
			:Name(InName),Do(InOperator){}
		FString Name;
		TFunction<FString(void)> Do;
	};
	
	TArray<FResularOperator> RegularOpList;
	RegularOpList.Emplace(TEXT("{VERSION}"),[&InCurrentVersion]()->FString{return InCurrentVersion.VersionId;});
	RegularOpList.Emplace(TEXT("{BASEVERSION}"),[&InCurrentVersion]()->FString{return InCurrentVersion.BaseVersionId;});
	RegularOpList.Emplace(TEXT("{PLATFORM}"),[&InPlatform]()->FString{return InPlatform;});
	RegularOpList.Emplace(TEXT("{CHUNKNAME}"),[this,&InChunkInfo]()->FString
	{
		FString result;
		if(this->GetSettingObject()->bEnableChunk)
		{
			result = InChunkInfo.ChunkName;
		}
		return result;
	});
	
	auto CustomPakNameRegular = [](const TArray<FResularOperator>& Operators,const FString& Regular)->FString
	{
		FString Result = Regular;
		for(auto& Operator:Operators)
		{
			Result = Result.Replace(*Operator.Name,*(Operator.Do()));
		}
		while(Result.Contains(TEXT("__")))
		{
			Result = Result.Replace(TEXT("__"),TEXT("_"));
		}
		return Result;
 	};
	
	return CustomPakNameRegular(RegularOpList,GetSettingObject()->GetPakNameRegular());
}

void UPatcherProxy::CookChunkAssets(const FPatchVersionDiff& DiffInfo, const FChunkInfo& Chunk, const TArray<ETargetPlatform>& Platforms)
{
	FChunkAssetDescribe ChunkAssetsDescrible = UFlibPatchParserHelper::CollectFChunkAssetsDescribeByChunk(DiffInfo, Chunk ,Platforms);
	TArray<FAssetDetail> ChunkAssets;
	UFLibAssetManageHelperEx::GetAssetDetailsByAssetDependenciesInfo(ChunkAssetsDescrible.Assets,ChunkAssets);
	TArray<FSoftObjectPath> AssetsSoftPath;

	for(const auto& Asset:ChunkAssets)
	{
		FSoftObjectPath AssetSoftPath;
		AssetSoftPath.SetPath(Asset.mPackagePath);
		AssetsSoftPath.AddUnique(AssetSoftPath);
	}
	if(!!AssetsSoftPath.Num())
	{
		UFlibHotPatcherEditorHelper::CookAssets(AssetsSoftPath,Platforms,UFlibHotPatcherEditorHelper::GetProjectCookedDir());
	}
}

#undef LOCTEXT_NAMESPACE
