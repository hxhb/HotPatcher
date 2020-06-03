#include "CreatePatch/PatcherProxy.h"
// engine header
#include "Dom/JsonValue.h"
#include "HAL/PlatformFilemanager.h"
#include "Kismet/KismetStringLibrary.h"
#include "PakFileUtilities.h"

#define LOCTEXT_NAMESPACE "HotPatcherProxy"


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
		FAssetDependenciesInfo AllChangedAssetInfo = UFLibAssetManageHelperEx::CombineAssetDependencies(InDiff.AddAssetDependInfo, InDiff.ModifyAssetDependInfo);
		bool bSelectedCookStatus = CheckSelectedAssetsCookStatus(ExportPatchSetting->GetPakTargetPlatformNames(), AllChangedAssetInfo, GenErrorMsg);

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
	OutPakVersion = UExportPatchSettings::GetPakVersion(InSaveVersion, FDateTime::UtcNow().ToString());
	{
		if (ExportPatchSetting->IsIncludePakVersion())
		{
			FString SavePakVersionFilePath = UExportPatchSettings::GetSavePakVersionPath(InSavePath, InSaveVersion);

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
	if (ExportPatchSetting->IsSaveDiffAnalysis())
	{
		FString SerializeDiffInfo =
			FString::Printf(TEXT("%s\n%s\n"),
				*UFlibPatchParserHelper::SerializeDiffAssetsInfomationToString(InDiff.AddAssetDependInfo, InDiff.ModifyAssetDependInfo, InDiff.DeleteAssetDependInfo),
				ExportPatchSetting->IsEnableExternFilesDiff() ?
				*UFlibPatchParserHelper::SerializeDiffExternalFilesInfomationToString(InDiff.AddExternalFiles, InDiff.ModifyExternalFiles, InDiff.DeleteExternalFiles) :
				*UFlibPatchParserHelper::SerializeDiffExternalFilesInfomationToString(ExportPatchSetting->GetAllExternFiles(), TArray<FExternAssetFileInfo>{}, TArray<FExternAssetFileInfo>{})
			);


		FString SaveDiffToFile = FPaths::Combine(
			ExportPatchSetting->GetCurrentVersionSavePath(),
			FString::Printf(TEXT("%s_%s_Diff.json"), *InSaveVersion.BaseVersionId, *InSaveVersion.VersionId)
		);
		if (UFLibAssetManageHelperEx::SaveStringToFile(SaveDiffToFile, SerializeDiffInfo))
		{
			bStatus = true;
			// auto Msg = LOCTEXT("SavePatchDiffInfo", "Succeed to export New Patch Diff Info.");
			// UFlibHotPatcherEditorHelper::CreateSaveFileNotify(Msg, SaveDiffToFile);

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
		TArray<FString> OutPakCommand = ExportPatchSetting->MakeAllPakCommandsByTheSetting(InPlatformName, InDiffInfo, ExportPatchSetting->IsEnableExternFilesDiff());

		// save paklist to file
		if (FFileHelper::SaveStringArrayToFile(OutPakCommand, *InSavePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
		{
			if (ExportPatchSetting->IsSavePakList())
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


FHotPatcherVersion UPatcherProxy::MakeNewRelease(const FHotPatcherVersion& InBaseVersion, const FHotPatcherVersion& InCurrentVersion)const
{
	FHotPatcherVersion BaseVersion = InBaseVersion;
	FHotPatcherVersion NewRelease = InCurrentVersion;
	FPatchVersionDiff DiffInfo = UFlibPatchParserHelper::DiffPatchVersion(BaseVersion,InCurrentVersion);
	
	FAssetDependenciesInfo& BaseAssetInfoRef = BaseVersion.AssetInfo;
	TMap<FString, FExternAssetFileInfo>& BaseExternalFilesRef = BaseVersion.ExternalFiles;

	// Modify Asset
	auto DeleteOldAssetLambda = [&BaseAssetInfoRef](const FAssetDependenciesInfo& InAssetDependenciesInfo)
	{
		for (const auto& AssetsModulePair : InAssetDependenciesInfo.mDependencies)
		{
			FAssetDependenciesDetail* NewReleaseModuleAssets = BaseAssetInfoRef.mDependencies.Find(AssetsModulePair.Key);

			for (const auto& NeedDeleteAsset : AssetsModulePair.Value.mDependAssetDetails)
			{
				if (NewReleaseModuleAssets->mDependAssetDetails.Contains(NeedDeleteAsset.Key))
				{
					NewReleaseModuleAssets->mDependAssetDetails.Remove(NeedDeleteAsset.Key);
				}
			}
		}
	};
	
	DeleteOldAssetLambda(DiffInfo.ModifyAssetDependInfo);
	// DeleteOldAssetLambda(DiffInfo.DeleteAssetDependInfo);

	// Add Asset
	BaseAssetInfoRef = UFLibAssetManageHelperEx::CombineAssetDependencies(BaseAssetInfoRef, DiffInfo.AddAssetDependInfo);
	// modify Asset
	BaseAssetInfoRef = UFLibAssetManageHelperEx::CombineAssetDependencies(BaseAssetInfoRef, DiffInfo.ModifyAssetDependInfo);
	NewRelease.AssetInfo = BaseAssetInfoRef;

	// external files
	auto DeleteOldExternalFilesLambda = [&BaseExternalFilesRef](const TArray<FExternAssetFileInfo>& InFiles)
	{
		for (const auto& File : InFiles)
		{
			if (BaseExternalFilesRef.Contains(File.FilePath.FilePath))
			{
				BaseExternalFilesRef.Remove(File.FilePath.FilePath);
			}
		}
	};
	DeleteOldExternalFilesLambda(DiffInfo.ModifyExternalFiles);
	// DeleteOldExternalFilesLambda(DiffInfo.DeleteExternalFiles);

	auto AddExternalFilesLambda = [&BaseExternalFilesRef](const TArray<FExternAssetFileInfo>& InFiles)
	{
		for (const auto& File : InFiles)
		{
			if (!BaseExternalFilesRef.Contains(File.FilePath.FilePath))
			{
				BaseExternalFilesRef.Add(File.FilePath.FilePath,File);
			}
		}
	};
	AddExternalFilesLambda(DiffInfo.AddExternalFiles);
	AddExternalFilesLambda(DiffInfo.ModifyExternalFiles);
	NewRelease.ExternalFiles = BaseExternalFilesRef;

	return NewRelease;
}

bool UPatcherProxy::CanExportPatch()const
{
	bool bCanExport = false;
	if (ExportPatchSetting)
	{
		bool bHasBase = false;
		if (ExportPatchSetting->IsByBaseVersion())
			bHasBase = !ExportPatchSetting->GetBaseVersion().IsEmpty() && FPaths::FileExists(ExportPatchSetting->GetBaseVersion());
		else
			bHasBase = true;
		bool bHasVersionId = !ExportPatchSetting->GetVersionId().IsEmpty();
		bool bHasFilter = !!ExportPatchSetting->GetAssetIncludeFiltersPaths().Num();
		bool bHasSpecifyAssets = !!ExportPatchSetting->GetIncludeSpecifyAssets().Num();
		bool bHasExternFiles = !!ExportPatchSetting->GetAddExternFiles().Num();
		bool bHasExDirs = !!ExportPatchSetting->GetAddExternDirectory().Num();
		bool bHasSavePath = !ExportPatchSetting->GetSaveAbsPath().IsEmpty();
		bool bHasPakPlatfotm = !!ExportPatchSetting->GetPakTargetPlatforms().Num();

		bool bHasAnyPakFiles = (
			bHasFilter || bHasSpecifyAssets || bHasExternFiles || bHasExDirs ||
			ExportPatchSetting->IsIncludeEngineIni() ||
			ExportPatchSetting->IsIncludePluginIni() ||
			ExportPatchSetting->IsIncludeProjectIni()
			);
		bCanExport = bHasBase && bHasVersionId && bHasAnyPakFiles && bHasPakPlatfotm && bHasSavePath;
	}
	return bCanExport;
}

bool UPatcherProxy::DoExportPatch()
{
	FHotPatcherVersion BaseVersion;

	if (ExportPatchSetting->IsByBaseVersion() && !ExportPatchSetting->GetBaseVersionInfo(BaseVersion))
	{
		UE_LOG(LogTemp, Error, TEXT("Deserialize Base Version Faild!"));
		return false;
	}
	else
	{
		// 在不进行外部文件diff的情况下清理掉基础版本的外部文件
		if (!ExportPatchSetting->IsEnableExternFilesDiff())
		{
			BaseVersion.ExternalFiles.Empty();
		}
	}
	
	UFLibAssetManageHelperEx::UpdateAssetMangerDatabase(true);
	FChunkInfo NewVersionChunk = UFlibHotPatcherEditorHelper::MakeChunkFromPatchSettings(ExportPatchSetting);

	FHotPatcherVersion CurrentVersion = UFlibPatchParserHelper::ExportReleaseVersionInfoByChunk(
		ExportPatchSetting->GetVersionId(),
		BaseVersion.VersionId,
		FDateTime::UtcNow().ToString(),
		NewVersionChunk,
		ExportPatchSetting->IsIncludeHasRefAssetsOnly()
	);

	FString CurrentVersionSavePath = ExportPatchSetting->GetCurrentVersionSavePath();
	FPatchVersionDiff VersionDiffInfo = UFlibPatchParserHelper::DiffPatchVersion(BaseVersion, CurrentVersion);

	FString ReceiveMsg;
	if (!CheckPatchRequire(VersionDiffInfo, ReceiveMsg))
	{
		return false;
	}

	int32 ChunkNum = ExportPatchSetting->IsEnableChunk() ? ExportPatchSetting->GetChunkInfos().Num() : 1;
	

	// save pakversion.json
	//FPakVersion CurrentPakVersion;
	//SavePatchVersionJson(CurrentVersion, CurrentVersionSavePath, CurrentPakVersion);

	// package all selected platform
	TMap<FString,TArray<FPakFileInfo>> PakFilesInfoMap;

	// Check Chunk
	if(ExportPatchSetting->IsEnableChunk())
	{
		FString TotalMsg;
		FChunkInfo TotalChunk = UFlibPatchParserHelper::CombineChunkInfos(ExportPatchSetting->GetChunkInfos());

		FChunkAssetDescribe ChunkDiffInfo = UFlibPatchParserHelper::DiffChunk(NewVersionChunk, TotalChunk, ExportPatchSetting->IsIncludeHasRefAssetsOnly());

		TArray<FString> AllUnselectedAssets = ChunkDiffInfo.GetAssetsStrings();
		TArray<FString> AllUnselectedExFiles = ChunkDiffInfo.GetExFileStrings();
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
			// ShowMsg(FString::Printf(TEXT("Unselect in Chunk:\n%s"), *TotalMsg));
			return false;
		}
		else
		{
			// ShowMsg(TEXT(""));
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
			GetKeysLambda(VersionDiffInfo.AddAssetDependInfo);
			GetKeysLambda(VersionDiffInfo.ModifyAssetDependInfo);

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

	bool bEnableChunk = ExportPatchSetting->IsEnableChunk();

	TArray<FChunkInfo> PakChunks;
	if (bEnableChunk)
	{
		PakChunks = ExportPatchSetting->GetChunkInfos();
	}
	else
	{
		PakChunks.Add(NewVersionChunk);
	}

	float AmountOfWorkProgress = 2.f * (ExportPatchSetting->GetPakTargetPlatforms().Num() * ChunkNum) + 4.0f;

	for(const auto& PlatformName :ExportPatchSetting->GetPakTargetPlatformNames())
	{
		// PakModeSingleLambda(PlatformName, CurrentVersionSavePath);
		for (const auto& Chunk : PakChunks)
		{
			// Update Progress Dialog
			{
				FString Msg = FString::Printf(TEXT("Generating UnrealPak Commands of %s Platform Chunk %s"),*PlatformName,*Chunk.ChunkName);
				OnPaking.Broadcast(TEXT("ExportPatch"),Msg);
			}
			
			
			FString ChunkSaveBasePath = FPaths::Combine(ExportPatchSetting->SavePath.Path, CurrentVersion.VersionId, PlatformName);
			TArray<FPakFileProxy> PakFileProxys;

			TArray<FPakCommand> ChunkPakCommands = UFlibPatchParserHelper::CollectPakCommandByChunk(VersionDiffInfo, Chunk, PlatformName, ExportPatchSetting->GetPakCommandOptions());
			FString PakPostfix = TEXT("_001_P");
			if(!Chunk.bMonolithic)
			{
				FPakFileProxy SinglePakForChunk;
				SinglePakForChunk.PakCommands = ChunkPakCommands;

				FString ChunkSaveName = ExportPatchSetting->IsEnableChunk() ? FString::Printf(TEXT("%s_%s"), *CurrentVersion.VersionId, *Chunk.ChunkName) : FString::Printf(TEXT("%s"), *CurrentVersion.VersionId);
				SinglePakForChunk.PakCommandSavePath = FPaths::Combine(ChunkSaveBasePath, FString::Printf(TEXT("%s_%s_PakCommands.txt"), *ChunkSaveName, *PlatformName));
				SinglePakForChunk.PakSavePath = FPaths::Combine(ChunkSaveBasePath, FString::Printf(TEXT("%s_%s%s.pak"), *ChunkSaveName, *PlatformName,*PakPostfix));
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
				FString Msg = FString::Printf(TEXT("Generating Pak list of %s Platform Chunk %s"),*PlatformName,*Chunk.ChunkName);
				OnPaking.Broadcast(TEXT("GeneratedPak"),Msg);
			}

			TArray<FString> UnrealPakOptions = ExportPatchSetting->GetUnrealPakOptions();
			TArray<FReplaceText> ReplacePakCommandTexts = ExportPatchSetting->GetReplacePakCommandTexts();
			// 创建chunk的pak文件
			for (const auto& PakFileProxy : PakFileProxys)
			{

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
							FString Msg = FString::Printf(TEXT("Successd to Package the patch as %s."),*PakFileProxy.PakSavePath);
							OnPaking.Broadcast(TEXT("SavedPakFile"),Msg);
							
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
	//	FString PakVersionSavedPath = UExportPatchSettings::GetSavePakVersionPath(CurrentVersionSavePath,CurrentVersion);
	//	if (ExportPatchSetting->IsIncludePakVersion() && FPaths::FileExists(PakVersionSavedPath))
	//	{
	//		IFileManager::Get().Delete(*PakVersionSavedPath);
	//	}
	//}



	// save asset dependency
	{
		if (ExportPatchSetting->IsSaveAssetRelatedInfo())
		{
			TArray<EAssetRegistryDependencyTypeEx> AssetDependencyTypes;
			AssetDependencyTypes.Add(EAssetRegistryDependencyTypeEx::Packages);

			TArray<FAssetRelatedInfo> AssetsDependency = UFlibPatchParserHelper::GetAssetsRelatedInfoByFAssetDependencies(
				UFLibAssetManageHelperEx::CombineAssetDependencies(VersionDiffInfo.AddAssetDependInfo, VersionDiffInfo.ModifyAssetDependInfo),
				AssetDependencyTypes
			);

			FString AssetsDependencyString;
			UFlibPatchParserHelper::SerializeAssetsRelatedInfoAsString(AssetsDependency, AssetsDependencyString);
			FString SaveAssetRelatedInfoToFile = FPaths::Combine(
				CurrentVersionSavePath,
				FString::Printf(TEXT("%s_AssetRelatedInfos.json"), *CurrentVersion.VersionId)
			);
			if (UFLibAssetManageHelperEx::SaveStringToFile(SaveAssetRelatedInfoToFile, AssetsDependencyString))
			{
				// auto Msg = LOCTEXT("SaveAssetRelatedInfo", "Succeed to export Asset Related infos.");
				// UFlibHotPatcherEditorHelper::CreateSaveFileNotify(Msg, SaveAssetRelatedInfoToFile);

				FString Msg = FString::Printf(TEXT("Succeed to export Asset Related infos."),*SaveAssetRelatedInfoToFile);
				OnPaking.Broadcast(TEXT("SaveAssetRelatedInfo"),Msg);
			}
		}
	}

	// save difference to file
	{
		FString Msg = FString::Printf(TEXT("Generating Diff info of version %s."),*CurrentVersion.VersionId);
		OnPaking.Broadcast(TEXT("ExportPatchDiffFile"),Msg);
		SavePatchDiffJson(CurrentVersion, VersionDiffInfo);
	}

	// save Patch Tracked asset info to file
	{
		{
			FString Msg = FString::Printf(TEXT("Generating Patch Tacked Asset info of version %s."),*CurrentVersion.VersionId);
			OnPaking.Broadcast(TEXT("ExportPatchAssetInfo"),Msg);
		}
		FString SerializeReleaseVersionInfo;
		FHotPatcherVersion NewReleaseVersion = MakeNewRelease(BaseVersion, CurrentVersion);
		UFlibPatchParserHelper::SerializeHotPatcherVersionToString(NewReleaseVersion, SerializeReleaseVersionInfo);

		FString SaveCurrentVersionToFile = FPaths::Combine(
			CurrentVersionSavePath,
			FString::Printf(TEXT("%s_Release.json"), *CurrentVersion.VersionId)
		);
		if (UFLibAssetManageHelperEx::SaveStringToFile(SaveCurrentVersionToFile, SerializeReleaseVersionInfo))
		{
			// auto Msg = LOCTEXT("SavePatchDiffInfo", "Succeed to export New Release Info.");
			// UFlibHotPatcherEditorHelper::CreateSaveFileNotify(Msg, SaveCurrentVersionToFile);
			FString Msg = FString::Printf(TEXT("Succeed to export New Release Info to %s."),*SaveCurrentVersionToFile);
			OnPaking.Broadcast(TEXT("SavePatchDiffInfo"),Msg);
		}
	}

	// serialize all pak file info
	{
		{
			FString Msg = FString::Printf(TEXT("Generating All Platform Pak info of version %s."),*CurrentVersion.VersionId);
			OnPaking.Broadcast(TEXT("ExportPatchPakFileInfo"),Msg);
			
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
				// FText Msg = LOCTEXT("SavedPakFileMsg", "Successd to Export the Pak File info.");
				// UFlibHotPatcherEditorHelper::CreateSaveFileNotify(Msg, SavePakFilesPath);
				FString Msg = FString::Printf(TEXT("Successd to Export the Pak File info to ."),*SavePakFilesPath);
				OnPaking.Broadcast(TEXT("SavedPakFileMsg"),Msg);
			}
		}
	}

	// serialize patch config
	{
		{
			FString Msg = FString::Printf(TEXT("Generating Current Patch Config of version %s."),*CurrentVersion.VersionId);
			OnPaking.Broadcast(TEXT("ExportPatchConfig"),Msg);	
		}

		FString SaveConfigPath = FPaths::Combine(
			CurrentVersionSavePath,
			FString::Printf(TEXT("%s_PatchConfig.json"),*CurrentVersion.VersionId)
		);

		if (ExportPatchSetting->IsSavePatchConfig())
		{
			FString SerializedJsonStr;
			ExportPatchSetting->SerializePatchConfigToString(SerializedJsonStr);

			if (FFileHelper::SaveStringToFile(SerializedJsonStr, *SaveConfigPath))
			{
				// FText Msg = LOCTEXT("SavedPatchConfig", "Successd to Export the Patch Config.");
				// UFlibHotPatcherEditorHelper::CreateSaveFileNotify(Msg, SaveConfigPath);
				FString Msg = FString::Printf(TEXT("Successd to Export the Patch Config to %s."),*SaveConfigPath);
				OnPaking.Broadcast(TEXT("SavedPatchConfig"),Msg);
			}
		}
	}

	return true;
}

#undef LOCTEXT_NAMESPACE
