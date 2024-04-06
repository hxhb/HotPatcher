// project header
#include "CreatePatch/PatcherProxy.h"
#include "HotPatcherLog.h"
#include "ThreadUtils/FThreadUtils.hpp"
#include "CreatePatch/HotPatcherContext.h"
#include "CreatePatch/ScopedSlowTaskContext.h"
#include "CreatePatch/HotPatcherContext.h"
#include "FlibHotPatcherCoreHelper.h"
#include "ShaderLibUtils/FlibShaderCodeLibraryHelper.h"
#include "Cooker/MultiCooker/FCookShaderCollectionProxy.h"
#include "AssetRegistry.h"

// engine header
#include "Async/Async.h"
#include "CoreGlobals.h"
#include "ShaderCompiler.h"
#include "Dom/JsonValue.h"
#include "HAL/PlatformFilemanager.h"
#include "Kismet/KismetStringLibrary.h"
#include "PakFileUtilities.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/FileHelper.h"
#include "Templates/Function.h"
#include "BinariesPatchFeature.h"
#include "HotPatcherCore.h"
#include "HotPatcherDelegates.h"
#include "HotPatcherSettings.h"
#include "Features/IModularFeatures.h"
#include "Async/ParallelFor.h"
#include "Cooker/MultiCooker/FlibHotCookerHelper.h"
#include "Cooker/MultiCooker/FSingleCookerSettings.h"
#include "Cooker/MultiCooker/SingleCookerProxy.h"
#include "DependenciesParser/FDefaultAssetDependenciesParser.h"
#include "Misc/DataDrivenPlatformInfoRegistry.h"
#include "Serialization/ArrayWriter.h"
#include "ThreadUtils/FProcWorkerThread.hpp"

#if WITH_IO_STORE_SUPPORT
#include "IoStoreUtilities.h"
#endif

#define LOCTEXT_NAMESPACE "HotPatcherProxy"

UPatcherProxy::UPatcherProxy(const FObjectInitializer& ObjectInitializer):Super(ObjectInitializer){}

bool UPatcherProxy::CanExportPatch()const
{
	return UFlibPatchParserHelper::IsValidPatchSettings(const_cast<UPatcherProxy*>(this)->GetSettingObject(),false);
}

FString GetShaderLibDeterministicCmdByPlatforms(const TArray<FString>& PlatformNames)
{
	FString ShaderLibDeterministicCommand;
	if(!!PlatformNames.Num())
	{
		ShaderLibDeterministicCommand = TEXT("TARGETPLATFORM=");
		for(const auto& PlatfromName:PlatformNames)
		{
			ShaderLibDeterministicCommand += FString::Printf(TEXT("%s+"),*PlatfromName);
		}
		ShaderLibDeterministicCommand.RemoveFromEnd(TEXT("+"));
	}
	return ShaderLibDeterministicCommand;
}

namespace PatchWorker
{
	// setup 1
	bool BaseVersionReader(FHotPatcherPatchContext& Context);
	// setup 2
	bool MakeCurrentVersionWorker(FHotPatcherPatchContext& Context);
	// setup 3
	bool ParseVersionDiffWorker(FHotPatcherPatchContext& Context);
	// setup 3.1
	bool ParseDiffAssetOnlyWorker(FHotPatcherPatchContext& Context);
	// setup 4
	bool PatchRequireChekerWorker(FHotPatcherPatchContext& Context);
	// setup 5
	bool SavePakVersionWorker(FHotPatcherPatchContext& Context);
	// setup 6
	bool ParserChunkWorker(FHotPatcherPatchContext& Context);
	
	bool PreCookPatchAssets(FHotPatcherPatchContext& Context);
	// setup 7
	bool CookPatchAssetsWorker(FHotPatcherPatchContext& Context);
	// setup 7.1
	bool PostCookPatchAssets(FHotPatcherPatchContext& Context);

	// setup 7.1
	bool PatchAssetRegistryWorker(FHotPatcherPatchContext& Context);
	// setup 7.2
	bool GenerateGlobalAssetRegistryManifest(FHotPatcherPatchContext& Context);
	// setup 8
	bool GeneratePakProxysWorker(FHotPatcherPatchContext& Context);
	// setup 9
	bool CreatePakWorker(FHotPatcherPatchContext& Context);
	// io store
	bool CreateIoStoreWorker(FHotPatcherPatchContext& Context);

	bool SaveDifferenceWorker(FHotPatcherPatchContext& Context);
	// setup 12
	bool SaveNewReleaseWorker(FHotPatcherPatchContext& Context);
	// setup 13 serialize all pak file info
	bool SavePakFileInfoWorker(FHotPatcherPatchContext& Context);
	// setup 14 serialize patch config
	bool SavePatchConfigWorker(FHotPatcherPatchContext& Context);
	// setup 15
	// bool BackupMetadataWorker(FHotPatcherPatchContext& Context);
	// setup 16
	bool ShowSummaryWorker(FHotPatcherPatchContext& Context);
	// setup 17
	bool OnFaildDispatchWorker(FHotPatcherPatchContext& Context);
	// setup 18
	bool NotifyOperatorsWorker(FHotPatcherPatchContext& Context);

	void GenerateBinariesPatch(FHotPatcherPatchContext& Context,FChunkInfo& Chunk,ETargetPlatform Platform,TArray<FPakCommand>& PakCommands);
	
	struct FTrackPackageAction
	{
		FTrackPackageAction(FHotPatcherPatchContext& InContext,const FChunkInfo& InChunkInfo,const TArray<ETargetPlatform>& InPlatforms):Context(InContext),ChunkInfo(InChunkInfo),Platforms(InPlatforms)
		{
			if(Context.GetSettingObject()->IsPackageTracker())
			{
				PackageTrackerByDiff = MakeShareable(new FPackageTrackerByDiff(Context));
			}
		}
		~FTrackPackageAction()
		{
			if(Context.GetSettingObject()->IsPackageTracker() && PackageTrackerByDiff.IsValid())
			{
				TArray<FString> IgnoreFilters  = UFlibAssetManageHelper::DirectoriesToStrings(Context.GetSettingObject()->GetAssetIgnoreFilters());
				TArray<FString> ForceSkipFilters = UFlibAssetManageHelper::DirectoriesToStrings(Context.GetSettingObject()->GetForceSkipContentRules());
				TSet<FString> ForceSkipAssets = UFlibAssetManageHelper::SoftObjectPathsToStringsSet(Context.GetSettingObject()->GetForceSkipAssets());
				TSet<FName> ForceSkipTypes = UFlibAssetManageHelper::GetClassesNames(Context.GetSettingObject()->GetForceSkipClasses());
				
				TArray<FAssetDetail> TrackerAssetDetails;
				for(const auto& TrackPackage:PackageTrackerByDiff->GetTrackResult())
				{
					bool bSkiped = FAssetDependenciesParser::IsForceSkipAsset(TrackPackage.Key.ToString(),ForceSkipTypes,IgnoreFilters,ForceSkipFilters,ForceSkipAssets,true);
					bool bContainInBaseVersion = FTrackPackageAction::IsAssetContainIn(Context.BaseVersion.AssetInfo,TrackPackage.Value);
					if(!bSkiped && !bContainInBaseVersion)
					{
						TrackerAssetDetails.AddUnique(TrackPackage.Value);
						Context.AddAsset(ChunkInfo.ChunkName,TrackPackage.Value);
					}
				}
			}
		}
		static bool IsAssetContainIn(const FAssetDependenciesInfo& InAssetInfo,const FAssetDetail& InAssetDetail)
		{
			bool bContainInBaseVersion = false;
			FSoftObjectPath SoftObjectPath{InAssetDetail.PackagePath};
			FAssetDetail BaseAssetDetail;
			bool bHasInBase = InAssetInfo.GetAssetDetailByPackageName(SoftObjectPath.GetLongPackageName(),BaseAssetDetail);
			if(bHasInBase)
			{
				bContainInBaseVersion = (BaseAssetDetail.Guid == InAssetDetail.Guid);
			}
			return bContainInBaseVersion;
		}
	private:
		FHotPatcherPatchContext& Context;
		const FChunkInfo& ChunkInfo;
		TArray<ETargetPlatform> Platforms;
		TSharedPtr<FPackageTrackerByDiff> PackageTrackerByDiff;
	};

	template<typename T>
	void ExportStructToFile(FHotPatcherPatchContext& Context,T& Settings,const FString& SaveTo,bool bNotify,const FString& NotifyName)
	{
		FString SerializedJsonStr;
		THotPatcherTemplateHelper::TSerializeStructAsJsonString(Settings,SerializedJsonStr);
		if (FFileHelper::SaveStringToFile(SerializedJsonStr, *SaveTo))
		{
			if(::IsRunningCommandlet())
			{
				FString Msg = FString::Printf(TEXT("Export %s to %s."),*NotifyName,*SaveTo);
				Context.OnPaking.Broadcast(TEXT("SavedPatchConfig"),Msg);
			}else if(bNotify)
			{
				FString MsgStr = FString::Printf(TEXT("Export %s to Successfuly."),*NotifyName);
				FText Msg = UKismetTextLibrary::Conv_StringToText(MsgStr);
				FHotPatcherDelegates::Get().GetNotifyFileGenerated().Broadcast(Msg, SaveTo);
			}
		}
	};
}

#define ADD_PATCH_WORKER(FUNC_NAME) AddPatchWorker(ANSI_TO_TCHAR(#FUNC_NAME),&FUNC_NAME);

void UPatcherProxy::Init(FPatcherEntitySettingBase* InSetting)
{
	SCOPED_NAMED_EVENT_TEXT("UPatcherProxy::Init",FColor::Red);
	Super::Init(InSetting);
#if WITH_PACKAGE_CONTEXT
	PlatformSavePackageContexts = UFlibHotPatcherCoreHelper::CreatePlatformsPackageContexts(
		GetSettingObject()->GetPakTargetPlatforms(),
		GetSettingObject()->IoStoreSettings.bIoStore,
		GetSettingObject()->GetStorageCookedDir()
		);
#endif
	UFlibAssetManageHelper::UpdateAssetMangerDatabase(true);
	GetSettingObject()->Init();
	ADD_PATCH_WORKER(PatchWorker::BaseVersionReader);
	ADD_PATCH_WORKER(PatchWorker::MakeCurrentVersionWorker);
	ADD_PATCH_WORKER(PatchWorker::SavePatchConfigWorker);
	ADD_PATCH_WORKER(PatchWorker::ParseVersionDiffWorker);
	ADD_PATCH_WORKER(PatchWorker::ParseDiffAssetOnlyWorker);
	ADD_PATCH_WORKER(PatchWorker::PatchRequireChekerWorker);
	ADD_PATCH_WORKER(PatchWorker::SavePakVersionWorker);
	ADD_PATCH_WORKER(PatchWorker::ParserChunkWorker);
	ADD_PATCH_WORKER(PatchWorker::PreCookPatchAssets);
	ADD_PATCH_WORKER(PatchWorker::CookPatchAssetsWorker);
	// cook finished
	ADD_PATCH_WORKER(PatchWorker::PostCookPatchAssets);
	ADD_PATCH_WORKER(PatchWorker::PatchAssetRegistryWorker);
	ADD_PATCH_WORKER(PatchWorker::GenerateGlobalAssetRegistryManifest);
	ADD_PATCH_WORKER(PatchWorker::GeneratePakProxysWorker);
	ADD_PATCH_WORKER(PatchWorker::CreatePakWorker);
	ADD_PATCH_WORKER(PatchWorker::CreateIoStoreWorker);
	ADD_PATCH_WORKER(PatchWorker::SaveDifferenceWorker);
	ADD_PATCH_WORKER(PatchWorker::SaveNewReleaseWorker);
	ADD_PATCH_WORKER(PatchWorker::SavePakFileInfoWorker);
	// ADD_PATCH_WORKER(PatchWorker::BackupMetadataWorker);
	ADD_PATCH_WORKER(PatchWorker::ShowSummaryWorker);
	ADD_PATCH_WORKER(PatchWorker::OnFaildDispatchWorker);
}

void UPatcherProxy::Shutdown()
{
	Super::Shutdown();
}

bool UPatcherProxy::DoExport()
{
	FScopedNamedEventStatic DoExportTag(FColor::Red,*FString::Printf(TEXT("DoExport_%s"),*GetSettingObject()->VersionId));
	PatchContext = MakeShareable(new FHotPatcherPatchContext);
	PatchContext->PatchProxy = this;
	PatchContext->OnPaking.AddLambda([this](const FString& One,const FString& Msg){this->OnPaking.Broadcast(One,Msg);});
	PatchContext->OnShowMsg.AddLambda([this](const FString& Msg){ this->OnShowMsg.Broadcast(Msg);});
	PatchContext->UnrealPakSlowTask = NewObject<UScopedSlowTaskContext>();
	PatchContext->UnrealPakSlowTask->AddToRoot();
	PatchContext->ContextSetting = GetSettingObject();
	PatchContext->Init();
	
	// wait cook complete
	if(PatchContext->GetSettingObject()->IsBinariesPatch())
	{
		this->OnPakListGenerated.AddStatic(&PatchWorker::GenerateBinariesPatch);
	}
	
	float AmountOfWorkProgress =  (float)GetPatchWorkers().Num() + (float)(GetSettingObject()->GetPakTargetPlatforms().Num() * FMath::Max(PatchContext->PakChunks.Num(),1));
	PatchContext->UnrealPakSlowTask->init(AmountOfWorkProgress);
	
	bool bRet = true;
	for(auto Worker:GetPatchWorkers())
	{
		if(!Worker.Value(*PatchContext))
		{
			bRet = false;
			break;
		}
	}
	
	PatchContext->UnrealPakSlowTask->Final();
	PatchContext->Shurdown();
	return bRet;
}


namespace PatchWorker
{
	// setup 1
	bool BaseVersionReader(FHotPatcherPatchContext& Context)
	{
		SCOPED_NAMED_EVENT_TEXT("BaseVersionReader",FColor::Red);
		TimeRecorder ReadBaseVersionTR(TEXT("Deserialize Base Version"));
		if (Context.GetSettingObject()->IsByBaseVersion() && !Context.GetSettingObject()->GetBaseVersionInfo(Context.BaseVersion))
		{
			UE_LOG(LogHotPatcher, Error, TEXT("Deserialize Base Version Faild!"));
			return false;
		}
		else
		{
			// 在不进行外部文件diff的情况下清理掉基础版本的外部文件
			if (!Context.GetSettingObject()->IsEnableExternFilesDiff())
			{
				Context.BaseVersion.PlatformAssets.Empty();
			}
		}
		return true;
	};
	
	// setup 2
	bool MakeCurrentVersionWorker(FHotPatcherPatchContext& Context)
	{
		SCOPED_NAMED_EVENT_TEXT("MakeCurrentVersionWorker",FColor::Red);
		UE_LOG(LogHotPatcher,Display,TEXT("Make Patch Setting..."));
		TimeRecorder ExportNewVersionTR(TEXT("Make Release Chunk/Export Release Version Info By Chunk"));

		// import cook dir and uasset / not-uasset directories to settings
		if(Context.GetSettingObject()->IsImportProjectSettings())
		{
			UFlibHotPatcherCoreHelper::ImportProjectSettingsToScannerConfig(Context.GetSettingObject()->GetAssetScanConfigRef());
			
			ETargetPlatform AddProjectDirToTarget = ETargetPlatform::AllPlatforms;
            
			TSet<ETargetPlatform> AllConfigPlatformSet;
			for(const auto& PakTargetPlatform:Context.GetSettingObject()->GetPakTargetPlatforms())
			{
				AllConfigPlatformSet.Add(PakTargetPlatform);
			}
            
			AddProjectDirToTarget = AllConfigPlatformSet.Num() > 1 ? ETargetPlatform::AllPlatforms : AllConfigPlatformSet.Array()[0];
			
			UFlibHotPatcherCoreHelper::ImportProjectNotAssetDir(
				Context.GetSettingObject()->GetAddExternAssetsToPlatform(),
				AddProjectDirToTarget
				);
		}
		
		Context.NewVersionChunk = UFlibHotPatcherCoreHelper::MakeChunkFromPatchSettings(Context.GetSettingObject());

		UE_LOG(LogHotPatcher,Display,TEXT("Deserialize Release Version by Patch Setting..."));
		Context.CurrentVersion = UFlibPatchParserHelper::ExportReleaseVersionInfoByChunk(
			Context.GetSettingObject()->GetVersionId(),
			Context.BaseVersion.VersionId,
			FDateTime::UtcNow().ToString(),
			Context.NewVersionChunk,
			Context.GetSettingObject()->IsIncludeHasRefAssetsOnly(),
			Context.GetSettingObject()->IsAnalysisFilterDependencies(),
			Context.GetSettingObject()->GetHashCalculator()
		);
		return true;
	};

	// setup 3
	bool ParseVersionDiffWorker(FHotPatcherPatchContext& Context)
	{
		SCOPED_NAMED_EVENT_TEXT("ParseVersionDiffWorker",FColor::Red);
		TimeRecorder DiffVersionTR(TEXT("Diff Base Version And Current Project Version"));
		Context.VersionDiff = UFlibHotPatcherCoreHelper::DiffPatchVersionWithPatchSetting(*Context.GetSettingObject(), Context.BaseVersion, Context.CurrentVersion);
		return true;
	};

	bool ParseDiffAssetOnlyWorker(FHotPatcherPatchContext& Context)
	{
		SCOPED_NAMED_EVENT_TEXT("ParseDiffAssetOnlyWorker",FColor::Red);
		TimeRecorder DiffVersionAssetOnlyTR(TEXT("Parse Diff Asset Dependencies Only Worker"));
		if(Context.GetSettingObject()->IsAnalysisDiffAssetDependenciesOnly())
		{
			FAssetDependenciesInfo AddAndModifyInfo = UFlibAssetManageHelper::CombineAssetDependencies(Context.VersionDiff.AssetDiffInfo.AddAssetDependInfo,Context.VersionDiff.AssetDiffInfo.ModifyAssetDependInfo);
			const TArray<FAssetDetail>& DiffAssetDetails = AddAndModifyInfo.GetAssetDetails();

			FChunkInfo DiffChunk;
			for(const auto& AssetDetail:DiffAssetDetails)
			{
				FPatcherSpecifyAsset CurrentAsets;
				CurrentAsets.Asset = FSoftObjectPath(AssetDetail.PackagePath.ToString());
				CurrentAsets.bAnalysisAssetDependencies = true;
				CurrentAsets.AssetRegistryDependencyTypes.AddUnique(EAssetRegistryDependencyTypeEx::Packages);
				DiffChunk.IncludeSpecifyAssets.Add(CurrentAsets);
			}
			
			FHotPatcherVersion DiffVersion = UFlibPatchParserHelper::ExportReleaseVersionInfoByChunk(
				Context.GetSettingObject()->GetVersionId(),
				Context.BaseVersion.VersionId,
				FDateTime::UtcNow().ToString(),
				DiffChunk,
				Context.GetSettingObject()->IsIncludeHasRefAssetsOnly(),
				Context.GetSettingObject()->IsAnalysisFilterDependencies(),
				Context.GetSettingObject()->GetHashCalculator()
			);
			{
				TimeRecorder DiffTR(TEXT("Base Version And Diff Version total time"));
				TMap<ETargetPlatform,FPatchVersionExternDiff> BackupExternalFiles = Context.VersionDiff.PlatformExternDiffInfo;
				Context.VersionDiff = UFlibHotPatcherCoreHelper::DiffPatchVersionWithPatchSetting(*Context.GetSettingObject(), Context.BaseVersion, DiffVersion);
				Context.VersionDiff.PlatformExternDiffInfo = BackupExternalFiles;
			}
		}
		return true;
	}
	// setup 4
	bool PatchRequireChekerWorker(FHotPatcherPatchContext& Context)
	{
		SCOPED_NAMED_EVENT_TEXT("PatchRequireChekerWorker",FColor::Red);
		bool result = true;
		TimeRecorder CheckRequireTR(TEXT("Check Patch Require"));
		// FString ReceiveMsg;
		// if (!Context.GetSettingObject()->IsCookPatchAssets() &&
		// 	!UFlibHotPatcherCoreHelper::CheckPatchRequire(
		// 	Context.GetSettingObject()->GetStorageCookedDir(),
		// 	Context.VersionDiff,Context.GetSettingObject()->GetPakTargetPlatformNames(), ReceiveMsg))
		// {
		// 	Context.OnShowMsg.Broadcast(ReceiveMsg);
		// 	result = false;
		// }
		return result;
	};

	// setup 5
	bool SavePakVersionWorker(FHotPatcherPatchContext& Context)
	{
		SCOPED_NAMED_EVENT_TEXT("SavePakVersionWorker",FColor::Red);
		// save pakversion.json
		TimeRecorder SavePakVersionTR(TEXT("Save Pak Version"));
		if(Context.GetSettingObject()->IsIncludePakVersion())
		{
			FPakVersion CurrentPakVersion;
			auto SavePatchVersionJson = [&Context](const FHotPatcherVersion& InSaveVersion, const FString& InSavePath, FPakVersion& OutPakVersion)->bool
			{
				bool bStatus = false;
				OutPakVersion = FExportPatchSettings::GetPakVersion(InSaveVersion, FDateTime::UtcNow().ToString());
				{
					if (Context.GetSettingObject()->IsIncludePakVersion())
					{
						FString SavePakVersionFilePath = FExportPatchSettings::GetSavePakVersionPath(InSavePath, InSaveVersion);

						FString OutString;
						if (THotPatcherTemplateHelper::TSerializeStructAsJsonString(OutPakVersion,OutString))
						{
							bStatus = UFlibAssetManageHelper::SaveStringToFile(SavePakVersionFilePath, OutString);
						}
					}
				}
				return bStatus;
			};
			
			SavePatchVersionJson(Context.CurrentVersion, Context.GetSettingObject()->GetCurrentVersionSavePath(), CurrentPakVersion);
			FPakCommand VersionCmd;
			FString AbsPath = FExportPatchSettings::GetSavePakVersionPath(Context.GetSettingObject()->GetCurrentVersionSavePath(), Context.CurrentVersion);
			FString MountPath = Context.GetSettingObject()->GetPakVersionFileMountPoint();
			VersionCmd.MountPath = MountPath;
			VersionCmd.PakCommands = TArray<FString>{
				FString::Printf(TEXT("\"%s\" \"%s\""),*AbsPath,*MountPath)
			};
			VersionCmd.AssetPackage = UFlibPatchParserHelper::MountPathToRelativePath(MountPath);
			Context.AdditionalFileToPak.AddUnique(VersionCmd);

			UE_LOG(LogHotPatcher,Display,TEXT("Save current patch pakversion.json to %s ..."),*AbsPath);
		}
		return true;
	};
	// setup 6
	bool ParserChunkWorker(FHotPatcherPatchContext& Context)
	{
		SCOPED_NAMED_EVENT_TEXT("ParserChunkWorker",FColor::Red);
		// Check Chunk
		if(Context.GetSettingObject()->IsEnableChunk())
		{
			TimeRecorder AnalysisChunkTR(TEXT("Analysis Chunk Info(enable chunk)"));
			FString TotalMsg;
			FChunkInfo TotalChunk = UFlibPatchParserHelper::CombineChunkInfos(Context.GetSettingObject()->GetChunkInfos());

			FChunkAssetDescribe ChunkDiffInfo = UFlibHotPatcherCoreHelper::DiffChunkWithPatchSetting(
				*Context.GetSettingObject(),
				Context.NewVersionChunk,
				TotalChunk
			);
			
			if(ChunkDiffInfo.HasValidAssets())
			{
				if(Context.GetSettingObject()->IsCreateDefaultChunk())
				{
				
					Context.PakChunks.Add(ChunkDiffInfo.AsChunkInfo(TEXT("Default")));
				}
				else
				{
					TArray<FName> AllUnselectedAssets = ChunkDiffInfo.GetAssetsStrings();
					TArray<FName> AllUnselectedExFiles;
					for(auto Platform:Context.GetSettingObject()->GetPakTargetPlatforms())
					{
						AllUnselectedExFiles.Append(ChunkDiffInfo.GetExternalFileNames(Platform));
					}
			
					TArray<FName> UnSelectedInternalFiles = ChunkDiffInfo.GetInternalFileNames();

					auto ChunkCheckerMsg = [&TotalMsg](const FString& Category,const TArray<FName>& InAssetList)
					{
						if (!!InAssetList.Num())
						{
							TotalMsg.Append(FString::Printf(TEXT("\n%s:\n"),*Category));
							for (const auto& Asset : InAssetList)
							{
								TotalMsg.Append(FString::Printf(TEXT("%s\n"), *Asset.ToString()));
							}
						}
					};
					ChunkCheckerMsg(TEXT("Unreal Asset"), AllUnselectedAssets);
					ChunkCheckerMsg(TEXT("External Files"), AllUnselectedExFiles);
					ChunkCheckerMsg(TEXT("Internal Files(Patch & Chunk setting not match)"), UnSelectedInternalFiles);

					if (!TotalMsg.IsEmpty())
					{
						Context.OnShowMsg.Broadcast(FString::Printf(TEXT("Unselect in Chunk:\n%s"), *TotalMsg));
						return false;
					}
					else
					{
						Context.OnShowMsg.Broadcast(TEXT(""));
					}
				}
			}
		}
		
		bool bEnableChunk = Context.GetSettingObject()->IsEnableChunk();

		// TArray<FChunkInfo> PakChunks;
		if (bEnableChunk)
		{
			Context.PakChunks.Append(Context.GetSettingObject()->GetChunkInfos());
		}
		else
		{
			Context.PakChunks.Add(Context.NewVersionChunk);
		}
		return !!Context.PakChunks.Num();
	};
	
	bool PreCookPatchAssets(FHotPatcherPatchContext& Context)
	{
		SCOPED_NAMED_EVENT_TEXT("PreCookPatchAssets",FColor::Red);
		// wait global / compiling shader by load asset
		UFlibShaderCodeLibraryHelper::WaitShaderCompilingComplete();
		
		return true;
	}
	
	bool PostCookPatchAssets(FHotPatcherPatchContext& Context)
	{
		SCOPED_NAMED_EVENT_TEXT("PostCookPatchAssets",FColor::Red);
		if(Context.GetSettingObject()->GetCookShaderOptions().bSharedShaderLibrary)
		{
			for(const auto& PlatformName :Context.GetSettingObject()->GetPakTargetPlatformNames())
			{
				for(auto& Chunk:Context.PakChunks)
				{
					FString ChunkSavedDir = Context.GetSettingObject()->GetChunkSavedDir(Context.CurrentVersion.VersionId,Context.CurrentVersion.BaseVersionId,Chunk.ChunkName,PlatformName);
					FString SavePath = FPaths::Combine(ChunkSavedDir,TEXT("Metadatas"),PlatformName);
					TArray<FString> FoundShaderLibs = UFlibShaderCodeLibraryHelper::FindCookedShaderLibByPlatform(PlatformName,SavePath,false);
		
					if(Context.PakChunks.Num())
					{
						for(const auto& FilePath:FoundShaderLibs)
						{
							if(!Context.GetSettingObject()->GetCookShaderOptions().bNativeShaderToPak &&
								(FilePath.EndsWith(TEXT("metallib")) || FilePath.EndsWith(TEXT("metalmap"))))
							{
								// don't add metalib and metalmap to pak
								continue;
							}
							FExternFileInfo AddShaderLib;
							{
								FString FileName = FPaths::GetBaseFilename(FilePath,true);
								FString FileExtersion = FPaths::GetExtension(FilePath,false);
									
								AddShaderLib.Type = EPatchAssetType::NEW;
								AddShaderLib.SetFilePath(FPaths::ConvertRelativePathToFull(FilePath));
								AddShaderLib.MountPath = FPaths::Combine(
									UFlibPatchParserHelper::ParserMountPointRegular(Chunk.CookShaderOptions.GetShaderLibMountPointRegular()),
									FString::Printf(TEXT("%s.%s"),*FileName,*FileExtersion)
									);
								AddShaderLib.GenerateFileHash(Context.GetSettingObject()->GetHashCalculator());
							}
							Context.AddExternalFile(PlatformName,Chunk.ChunkName,AddShaderLib);
						}
					}
				}
			}
		}
		return true;
	}
	
	// setup 7
	bool CookPatchAssetsWorker(FHotPatcherPatchContext& Context)
	{
		SCOPED_NAMED_EVENT_TEXT("CookPatchAssetsWorker",FColor::Red);
		TimeRecorder CookAssetsTotalTR(FString::Printf(TEXT("Cook All Assets in Patch Total time:")));
		
		for(const auto& PlatformName :Context.GetSettingObject()->GetPakTargetPlatformNames())
		{
			for(auto& Chunk:Context.PakChunks)
			{
				TimeRecorder CookPlatformChunkAssetsTotalTR(FString::Printf(TEXT("Cook Chunk %s as %s Total time:"),*Chunk.ChunkName,*PlatformName));
				if(Context.GetSettingObject()->IsCookPatchAssets() || Context.GetSettingObject()->GetIoStoreSettings().bIoStore)
				{
					TimeRecorder CookAssetsTR(FString::Printf(TEXT("Cook %s assets in the patch."),*PlatformName));

					ETargetPlatform Platform;
					THotPatcherTemplateHelper::GetEnumValueByName(PlatformName,Platform);

					FChunkAssetDescribe ChunkAssetsDescrible = UFlibPatchParserHelper::CollectFChunkAssetsDescribeByChunk(
						Context.GetSettingObject(),
						Context.VersionDiff,
						Chunk, TArray<ETargetPlatform>{Platform}
					);
					
					const TArray<FAssetDetail>& ChunkAssets = ChunkAssetsDescrible.Assets.GetAssetDetails();
					Context.PatchProxy->GetPatcherResult().PatcherAssetDetails.Append(ChunkAssets);
					bool bSharedShaderLibrary = Context.GetSettingObject()->GetCookShaderOptions().bSharedShaderLibrary;
					if(Context.GetSettingObject()->IsCookPatchAssets() || bSharedShaderLibrary)
					{
						TArray<FAssetDetail> AllShaderAssets;
						bool bOnlyCookShaders = !Context.GetSettingObject()->IsCookPatchAssets() && bSharedShaderLibrary;
						if(bOnlyCookShaders) // only cook shaders
						{
							SCOPED_NAMED_EVENT_TEXT("ParserAllShaderAssets",FColor::Red);
							TSet<FName> ShadersClasses = UFlibHotPatcherCoreHelper::GetAllMaterialClassesNames();
							for(const auto& Asset:ChunkAssets){
								if(ShadersClasses.Contains(Asset.AssetType)){ AllShaderAssets.Add(Asset);}
							}
						}
						FTrackPackageAction TrackChunkPackageAction(Context,Chunk,TArray<ETargetPlatform>{Platform});
						FSingleCookerSettings EmptySetting;
						EmptySetting.MissionID = 0;
						EmptySetting.MissionName = FString::Printf(TEXT("%s_Cooker_%s"),FApp::GetProjectName(),*Chunk.ChunkName);
						EmptySetting.ShaderLibName = Chunk.GetShaderLibraryName();
						EmptySetting.CookTargetPlatforms = TArray<ETargetPlatform>{Platform};
						EmptySetting.CookAssets = bOnlyCookShaders ? AllShaderAssets: ChunkAssets;
						// EmptySetting.ForceSkipClasses = {};
						EmptySetting.bPackageTracker = Context.GetSettingObject()->IsPackageTracker();
						EmptySetting.ShaderOptions.bSharedShaderLibrary = Context.GetSettingObject()->GetCookShaderOptions().bSharedShaderLibrary;
						EmptySetting.ShaderOptions.bNativeShader = Context.GetSettingObject()->GetCookShaderOptions().bNativeShader;
						EmptySetting.ShaderOptions.bMergeShaderLibrary = false;
						EmptySetting.IoStoreSettings = Context.GetSettingObject()->GetIoStoreSettings();
						EmptySetting.IoStoreSettings.bStorageBulkDataInfo = false;// dont save platform context data to disk
						EmptySetting.bSerializeAssetRegistry = Context.GetSettingObject()->GetSerializeAssetRegistryOptions().bSerializeAssetRegistry;
						EmptySetting.bPreGeneratePlatformData = Context.GetSettingObject()->IsCookParallelSerialize();
						EmptySetting.bWaitEachAssetCompleted = Context.GetSettingObject()->IsCookParallelSerialize();
						EmptySetting.bConcurrentSave = Context.GetSettingObject()->IsCookParallelSerialize();
						// for current impl arch
						EmptySetting.bForceCookInOneFrame = true;
						EmptySetting.NumberOfAssetsPerFrame = Context.GetSettingObject()->CookAdvancedOptions.NumberOfAssetsPerFrame;
						EmptySetting.OverrideNumberOfAssetsPerFrame = Context.GetSettingObject()->CookAdvancedOptions.GetOverrideNumberOfAssetsPerFrame();
						EmptySetting.bDisplayConfig = false;
						EmptySetting.StorageCookedDir = Context.GetSettingObject()->GetStorageCookedDir();//FPaths::Combine(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()),TEXT("Cooked"));

						EmptySetting.bAccompanyCook = Context.GetSettingObject()->CookAdvancedOptions.bAccompanyCookForShader;
						
						FString ChunkSavedDir = Context.GetSettingObject()->GetChunkSavedDir(Context.CurrentVersion.VersionId,Context.CurrentVersion.BaseVersionId,Chunk.ChunkName,PlatformName);
						EmptySetting.StorageMetadataDir = FPaths::Combine(ChunkSavedDir,TEXT("Metadatas"));
#if WITH_PACKAGE_CONTEXT
						EmptySetting.bOverrideSavePackageContext = true;
						EmptySetting.PlatformSavePackageContexts = Context.PatchProxy->GetPlatformSavePackageContexts();
#endif
						USingleCookerProxy* SingleCookerProxy = NewObject<USingleCookerProxy>();
						SingleCookerProxy->AddToRoot();
						SingleCookerProxy->Init(&EmptySetting);
						bool bExportStatus = SingleCookerProxy->DoExport();
						const FCookCluster& AdditionalCluster = SingleCookerProxy->GetPackageTrackerAsCluster(false);
						for(const auto& AssetDetail:AdditionalCluster.AssetDetails)
						{
							FSoftObjectPath ObjectPath{AssetDetail.PackagePath};
							bool bContainInBaseVersion = FTrackPackageAction::IsAssetContainIn(Context.BaseVersion.AssetInfo,AssetDetail);
							
							FString ReceiveReason;
							if(!Context.GetSettingObject()->GetAssetScanConfig().IsMatchForceSkip(ObjectPath,ReceiveReason) && !bContainInBaseVersion)
							{
								Context.PatchProxy->GetPatcherResult().PatcherAssetDetails.Add(AssetDetail);
								Context.VersionDiff.AssetDiffInfo.AddAssetDependInfo.AddAssetsDetail(AssetDetail);
							}
							else
							{
								UE_LOG(LogHotPatcher,Display,TEXT("[PackageTracker] %s Match ForceSkipRule,Reason %s"),*ObjectPath.GetLongPackageName(),*ReceiveReason);
							}
						}
						
						SingleCookerProxy->Shutdown();
						SingleCookerProxy->RemoveFromRoot();

#if WITH_UE5_BY_COOKCMDLT // add WP additional
						TSet<FName> WorldPackages;
						int32 Size = ChunkAssets.Num();
						FCriticalSection	LocalSynchronizationObject;
						ParallelFor(Size,[&](int32 index)
						{
							if(ChunkAssets[index].AssetType.IsEqual(TEXT("World")))
							{
								FScopeLock Lock(&LocalSynchronizationObject);
								WorldPackages.Add(ChunkAssets[index].PackagePath);
							}
						});
						FString StorageCookedDir = Context.GetSettingObject()->GetStorageCookedDir();
						for(const auto& WorldPackage:WorldPackages)
						{
							FExternDirectoryInfo DirectoryInfo;
							{
								FSoftObjectPath ObjectPath{WorldPackage};
								// abs
								FString WorldCookedPath = UFlibHotPatcherCoreHelper::GetAssetCookedSavePath(StorageCookedDir,ObjectPath.GetLongPackageName(), PlatformName);
								FString EndWith = FPaths::GetExtension(WorldCookedPath,true);
								WorldCookedPath.RemoveFromEnd(EndWith);
								// // mount path
								FString WorldMountPath = WorldCookedPath;
								WorldMountPath.RemoveFromStart(FPaths::Combine(StorageCookedDir,PlatformName));
								WorldMountPath = FString::Printf(TEXT("../../..%s"),*WorldMountPath);
								FPaths::NormalizeFilename(WorldMountPath);
						
								DirectoryInfo.DirectoryPath.Path = WorldCookedPath;
								DirectoryInfo.MountPoint = WorldMountPath;
							}
							if(FPaths::DirectoryExists(DirectoryInfo.DirectoryPath.Path))
							{
								const TArray<FExternFileInfo>& WPAdditional = UFlibPatchParserHelper::ParserExDirectoryAsExFiles(TArray<FExternDirectoryInfo>{DirectoryInfo},Context.GetSettingObject()->GetHashCalculator());
								for(const auto& WPAdditionalFile:WPAdditional)
								{
									Context.AddExternalFile(PlatformName,Chunk.ChunkName,WPAdditionalFile);
								}
							}
						}
#endif
					}
				}
			}
		}

		return true;
	};
	
	bool PatchAssetRegistryWorker(FHotPatcherPatchContext& Context)
	{
		SCOPED_NAMED_EVENT_TEXT("PatchAssetRegistryWorker",FColor::Red);
		if(Context.GetSettingObject()->GetSerializeAssetRegistryOptions().bSerializeAssetRegistry)
		{
			auto SerializeAssetRegistry = [](FHotPatcherPatchContext& Context,const FChunkInfo& Chunk,const FString& PlatformName)
			{
				ETargetPlatform Platform;
				THotPatcherTemplateHelper::GetEnumValueByName(PlatformName,Platform);
				FChunkAssetDescribe ChunkAssetsDescrible = UFlibPatchParserHelper::CollectFChunkAssetsDescribeByChunk(
					Context.GetSettingObject(),
					Context.VersionDiff,
					Chunk, TArray<ETargetPlatform>{Platform}
				);
				const TArray<FAssetDetail>& ChunkAssets = ChunkAssetsDescrible.Assets.GetAssetDetails();
				
				if(Context.GetSettingObject()->GetSerializeAssetRegistryOptions().bSerializeAssetRegistry)
				{
					FString ChunkAssetRegistryName = Context.GetSettingObject()->GetSerializeAssetRegistryOptions().GetAssetRegistryNameRegular(Chunk.ChunkName);
					// // Save the generated registry
					FString AssetRegistryPath = FPaths::Combine(
						Context.GetSettingObject()->GetChunkSavedDir(Context.CurrentVersion.VersionId,Context.BaseVersion.VersionId,Context.NewVersionChunk.ChunkName,PlatformName),
						ChunkAssetRegistryName);
					if(UFlibHotPatcherCoreHelper::SerializeAssetRegistryByDetails(Context.PatchProxy->GetAssetRegistry(),PlatformName,ChunkAssets, AssetRegistryPath))
					{
						FExternFileInfo AssetRegistryFileInfo;
						AssetRegistryFileInfo.Type = EPatchAssetType::NEW;
						AssetRegistryFileInfo.SetFilePath(AssetRegistryPath);
						AssetRegistryFileInfo.MountPath = FPaths::Combine(
							UFlibPatchParserHelper::ParserMountPointRegular(Context.GetSettingObject()->GetSerializeAssetRegistryOptions().GetAssetRegistryMountPointRegular())
							,ChunkAssetRegistryName
							);
						Context.GetPatcherDiffInfoByName(PlatformName)->AddExternalFiles.Add(AssetRegistryFileInfo);
						Context.GetPatcherChunkInfoByName(PlatformName,Chunk.ChunkName)->AddExternFileToPak.Add(AssetRegistryFileInfo);
					}
				}
			};
			
			switch(Context.GetSettingObject()->GetSerializeAssetRegistryOptions().AssetRegistryRule)
			{
			case EAssetRegistryRule::PATCH:
				{
					for(const auto& PlatformName:Context.GetSettingObject()->GetPakTargetPlatformNames())
					{
						SerializeAssetRegistry(Context,Context.NewVersionChunk,PlatformName);
					}
					break;
				}
			case EAssetRegistryRule::PER_CHUNK:
				{
					for(const auto& PlatformName:Context.GetSettingObject()->GetPakTargetPlatformNames())
					{
						for(const auto& Chunk:Context.PakChunks)
						{
							SerializeAssetRegistry(Context,Chunk,PlatformName);
						}
					}
					break;
				}
				default:{}
			};
		}
		return true;
	};
	bool GenerateGlobalAssetRegistryManifest(FHotPatcherPatchContext& Context)
	{
		SCOPED_NAMED_EVENT_TEXT("GenerateGlobalAssetRegistryManifest",FColor::Red);
		if(Context.GetSettingObject()->GetSerializeAssetRegistryOptions().bSerializeAssetRegistryManifest)
		{
			FAssetDependenciesInfo TotalDiffAssets = UFlibAssetManageHelper::CombineAssetDependencies(Context.VersionDiff.AssetDiffInfo.AddAssetDependInfo,Context.VersionDiff.AssetDiffInfo.ModifyAssetDependInfo);
			const TArray<FAssetDetail>& AllAssets = TotalDiffAssets.GetAssetDetails();

			TSet<FName> PackageAssetsSet;
			for(const auto& Asset:AllAssets)
			{
				FString LongPackageName = UFlibAssetManageHelper::PackagePathToLongPackageName(Asset.PackagePath.ToString());
				PackageAssetsSet.Add(*LongPackageName);
			}
			for(const auto& PlatformName:Context.GetSettingObject()->GetPakTargetPlatformNames())
			{
				ITargetPlatform* PlatformIns = UFlibHotPatcherCoreHelper::GetPlatformByName(PlatformName);
			
				UFlibHotPatcherCoreHelper::SerializeChunksManifests(PlatformIns,PackageAssetsSet,TSet<FName>{},true);
			}
		}
		
		return true;
	}

	void GenerateBinariesPatch(FHotPatcherPatchContext& Context,FChunkInfo& Chunk,ETargetPlatform Platform,TArray<FPakCommand>& PakCommands)
	{
		SCOPED_NAMED_EVENT_TEXT("GenerateBinariesPatch",FColor::Red);
		TArray<IBinariesDiffPatchFeature*> ModularFeatures = IModularFeatures::Get().GetModularFeatureImplementations<IBinariesDiffPatchFeature>(BINARIES_DIFF_PATCH_FEATURE_NAME);
		if(!ModularFeatures.Num())
			return;
		IBinariesDiffPatchFeature* UseFeature = ModularFeatures[0];
		for(const auto& Feature: ModularFeatures)
		{
			if(!Context.GetSettingObject()->GetBinariesPatchConfig().GetBinariesPatchFeatureName().IsEmpty() &&
				Feature->GetFeatureName().Equals(Context.GetSettingObject()->GetBinariesPatchConfig().GetBinariesPatchFeatureName(),ESearchCase::IgnoreCase))
			{
				UseFeature = Feature;
				break;
			}
		}
		UE_LOG(LogHotPatcher,Log,TEXT("Use BinariesPatchFeature %s"),*UseFeature->GetFeatureName());
		FString PlatformName = THotPatcherTemplateHelper::GetEnumNameByValue(Platform);
		UHotPatcherSettings* Settings = GetMutableDefault<UHotPatcherSettings>();
		FString TempSavedPath = Settings->GetTempSavedDir();
		TimeRecorder BinariesPatchToralTR(FString::Printf(TEXT("Generate Binaries Patch of %s all chunks  Total Time:"),*PlatformName));
		FString OldCookedDir = Context.GetSettingObject()->GetBinariesPatchConfig().GetOldCookedDir();
		if(!FPaths::FileExists(OldCookedDir))
		{
			OldCookedDir = FPaths::ConvertRelativePathToFull(FPaths::Combine(TempSavedPath,Context.BaseVersion.VersionId));
		}

		FString ExtractCryptoFile = Context.GetSettingObject()->GetBinariesPatchConfig().GetBasePakExtractCryptoJson();
		FString ExtractCryptoCmd = FPaths::FileExists(ExtractCryptoFile) ? FString::Printf(TEXT("-cryptokeys=\"%s\""),*ExtractCryptoFile) : TEXT("");
		FString ExtractDir = FPaths::ConvertRelativePathToFull(FPaths::Combine(TempSavedPath,Context.BaseVersion.VersionId,PlatformName));
		// Extract Asset by Base Version Paks
		auto ExtractByPak = [&Context,Platform,ExtractDir,ExtractCryptoCmd,ExtractCryptoFile](const FPakCommand& PakCommand)->bool
		{
			FString PakCommandMountOnlyPath;
			{
				FString postfix;
				PakCommand.MountPath.Split(TEXT("."),&PakCommandMountOnlyPath,&postfix,ESearchCase::IgnoreCase,ESearchDir::FromEnd);
			}
			
			TArray<FString> CurrentPlatformPaks = Context.GetSettingObject()->GetBinariesPatchConfig().GetBaseVersionPakByPlatform(Platform);
			bool bExtraced = false;
			for(const auto& Pak:CurrentPlatformPaks)
			{
				if(!FPaths::FileExists(Pak))
					continue;
				FString AESKeyString = UFlibPatchParserHelper::LoadAESKeyStringFromCryptoFile(ExtractCryptoFile);
				FString PakMountPoint = UFlibPakHelper::GetPakFileMountPoint(Pak,AESKeyString);
				if(!(!PakMountPoint.IsEmpty() && PakCommand.MountPath.StartsWith(PakMountPoint)))
				{
					// file not in the pak
					continue;
				}
				
				FString ExtractFilter = FString::Printf(
					TEXT("-Filter=\"%s.*\""),
					*UKismetStringLibrary::GetSubstring(PakCommandMountOnlyPath,PakMountPoint.Len(),PakCommandMountOnlyPath.Len() - PakMountPoint.Len())
				);
				FString RelativeMountPoint = PakMountPoint;
				while(RelativeMountPoint.RemoveFromStart(TEXT("../"))){}
				FString FinalExtractDir = FPaths::Combine(ExtractDir,RelativeMountPoint);
				FPaths::NormalizeFilename(FinalExtractDir);
				FString FinalCommand = FString::Printf(TEXT("-Extract \"%s\" \"%s\" %s %s"),*Pak,*FinalExtractDir,*ExtractCryptoCmd,*ExtractFilter);
				UE_LOG(LogHotPatcher,Log,TEXT("Extract Base Version Pak Command: %s"),*FinalCommand);
				if(ExecuteUnrealPak(*FinalCommand))
				{
					bExtraced = true;
				}
			}
			return bExtraced;
		};
		
		for(auto& PakCommand:PakCommands)
		{
			if(PakCommand.Type == EPatchAssetType::NEW || PakCommand.Type == EPatchAssetType::None)
				continue;
			if(!ExtractByPak(PakCommand))
				continue;
			TArray<FString> PatchedPakCommand;
			for(const auto& PakAssetPath: PakCommand.PakCommands)
			{
				FPakCommandItem PakAssetInfo = UFlibHotPatcherCoreHelper::ParsePakResponseFileLine(PakAssetPath);
				if(Context.GetSettingObject()->GetBinariesPatchConfig().IsMatchIgnoreRules(PakAssetInfo))
				{
					PatchedPakCommand.AddUnique(PakAssetPath);
					continue;
				}
				FString ProjectCookedDir = Context.GetSettingObject()->GetStorageCookedDir();// FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Cooked")));
				FString OldAsset = PakAssetInfo.AssetAbsPath.Replace(
					*ProjectCookedDir,
					*OldCookedDir,ESearchCase::CaseSensitive);
				FString PatchSaveToPath = PakAssetInfo.AssetAbsPath.Replace(
					*ProjectCookedDir,
					*FPaths::ConvertRelativePathToFull(FPaths::Combine(Settings->GetTempSavedDir(),TEXT("BinariesPatch")))
				) + TEXT(".patch");
				FString PatchSaveToMountPath = PakAssetInfo.AssetMountPath + TEXT(".patch");
				bool bPatch = false;
				if(FPaths::FileExists(PakAssetInfo.AssetAbsPath) && FPaths::FileExists(OldAsset))
				{
					TArray<uint8> OldAssetData;
					TArray<uint8> NewAssetData;
					TArray<uint8> PatchData;
					bool bLoadOld = FFileHelper::LoadFileToArray(OldAssetData,*OldAsset);
					bool bLoadNew = FFileHelper::LoadFileToArray(NewAssetData,*PakAssetInfo.AssetAbsPath);
					
					if(UseFeature->CreateDiff(NewAssetData,OldAssetData,PatchData))
					{
						if(FFileHelper::SaveArrayToFile(PatchData,*PatchSaveToPath))
						{
							PatchedPakCommand.AddUnique(FString::Printf(TEXT("\"%s\" \"%s\""),*PatchSaveToPath,*PatchSaveToMountPath));
							bPatch = true;
						}
					}
				}
				if(!bPatch)
				{
					PatchedPakCommand.AddUnique(PakAssetPath);
				}
			}
			if(!!PatchedPakCommand.Num())
			{
				PakCommand.PakCommands = PatchedPakCommand;
			}
		}
	}

	// setup 8
	bool GeneratePakProxysWorker(FHotPatcherPatchContext& Context)
	{
		SCOPED_NAMED_EVENT_TEXT("GeneratePakProxysWorker",FColor::Red);
		TimeRecorder PakChunkToralTR(FString::Printf(TEXT("Generate all platform pakproxys of all chunks Total Time:")));
		TArray<ETargetPlatform> PakPlatforms = Context.GetSettingObject()->GetPakTargetPlatforms();
		for(const auto& Platform :PakPlatforms)
		{
			FString PlatformName = THotPatcherTemplateHelper::GetEnumNameByValue(Platform);
			// PakModeSingleLambda(PlatformName, CurrentVersionSavePath);
			for (auto& Chunk : Context.PakChunks)
			{
				TimeRecorder PakChunkTR(FString::Printf(TEXT("Generate Chunk Platform:%s ChunkName:%s PakProxy Time:"),*PlatformName,*Chunk.ChunkName));
				// Update Progress Dialog
				{
					FText Dialog = FText::Format(NSLOCTEXT("ExportPatch", "GeneratedPakCommands", "Generating UnrealPak Commands of {0} Platform Chunk {1}."), FText::FromString(PlatformName),FText::FromString(Chunk.ChunkName));
					Context.OnPaking.Broadcast(TEXT("ExportPatch"),*Dialog.ToString());
					Context.UnrealPakSlowTask->EnterProgressFrame(1.0, Dialog);
				}

				FString ChunkSaveBasePath = Context.GetSettingObject()->GetChunkSavedDir(Context.CurrentVersion.VersionId,Context.CurrentVersion.BaseVersionId,Chunk.ChunkName,PlatformName);
				
				TArray<FPakCommand> ChunkPakListCommands;
				{
					TimeRecorder CookAssetsTR(FString::Printf(TEXT("CollectPakCommandByChunk Platform:%s ChunkName:%s."),*PlatformName,*Chunk.ChunkName));
					ChunkPakListCommands= UFlibPatchParserHelper::CollectPakCommandByChunk(
						Context.VersionDiff,
						Chunk,
						PlatformName,
						Context.GetSettingObject()
					);
				}
				
				if(Context.PatchProxy)
					Context.PatchProxy->OnPakListGenerated.Broadcast(Context,Chunk,Platform,ChunkPakListCommands);

				TArray<FString> IgnoreCompressFormats = UFlibHotPatcherCoreHelper::GetExtensionsToNotUsePluginCompressionByGConfig();
				for(auto& PakCommand:ChunkPakListCommands)
				{
					TArray<FString> EmptyArray;
					TArray<FString> CompressOption{TEXT("-compress")};

					auto AppendPakCommandOptions = [&Context](FPakCommand& PakCommand,
						const TArray<FString>& Options,
						bool bAppendAllMatch,
						const TArray<FString>& AppendFileExtersions,
						const TArray<FString>& IgnoreFormats, // 忽略追加Option的文件格式
						const TArray<FString>& InIgnoreFormatsOptions // 给忽略的格式忽略追加哪些option，如-compress不在ini中添加
						)
					{
						UFlibHotPatcherCoreHelper::AppendPakCommandOptions(
							PakCommand.PakCommands,
							Options,
							bAppendAllMatch,
							AppendFileExtersions,
							IgnoreFormats,
							InIgnoreFormatsOptions
							);
						UFlibHotPatcherCoreHelper::AppendPakCommandOptions(
							PakCommand.IoStoreCommands,
							Options,
							bAppendAllMatch,
							AppendFileExtersions,
							IgnoreFormats,
							InIgnoreFormatsOptions
							);
					};
					// 给所有的文件添加PakCommand的Option参数，默认只包含-compress，除了ExtensionsToNotUsePluginCompression中配置的文件都会添加
					UFlibHotPatcherCoreHelper::AppendPakCommandOptions(PakCommand.PakCommands,Context.GetSettingObject()->GetUnrealPakSettings().UnrealPakListOptions,true,EmptyArray,IgnoreCompressFormats,CompressOption);
					UFlibHotPatcherCoreHelper::AppendPakCommandOptions(PakCommand.PakCommands,Context.GetSettingObject()->GetDefaultPakListOptions(),true,EmptyArray,IgnoreCompressFormats,CompressOption);
					UFlibHotPatcherCoreHelper::AppendPakCommandOptions(PakCommand.IoStoreCommands,Context.GetSettingObject()->GetIoStoreSettings().IoStorePakListOptions,true,EmptyArray,IgnoreCompressFormats,CompressOption);
					UFlibHotPatcherCoreHelper::AppendPakCommandOptions(PakCommand.IoStoreCommands,Context.GetSettingObject()->GetDefaultPakListOptions(),true,EmptyArray,IgnoreCompressFormats,CompressOption);

					FEncryptSetting EncryptSettings = UFlibPatchParserHelper::GetCryptoSettingByPakEncryptSettings(Context.GetSettingObject()->GetEncryptSettings());
					
					// 加密所有文件
					if(EncryptSettings.bEncryptAllAssetFiles)
					{
						TArray<FString> EncryptOption{TEXT("-encrypt")};
						AppendPakCommandOptions(PakCommand,EncryptOption,true,EmptyArray,EmptyArray,EmptyArray);
					}
					else
					{
						// 加密 uasset
						if(EncryptSettings.bEncryptUAssetFiles)
						{
							TArray<FString> EncryptOption{TEXT("-encrypt")};
							TArray<FString> EncryptFileExtersion{TEXT("uasset")};
						
							AppendPakCommandOptions(PakCommand,EncryptOption,false,EncryptFileExtersion,EmptyArray,EmptyArray);
						}
						// 加密 ini
						if(EncryptSettings.bEncryptIniFiles)
						{
							TArray<FString> EncryptOption{TEXT("-encrypt")};
							TArray<FString> EncryptFileExtersion{TEXT("ini")};
							AppendPakCommandOptions(PakCommand,EncryptOption,false,EncryptFileExtersion,EmptyArray,EmptyArray);
						}
					}
				}
				
				if (!ChunkPakListCommands.Num())
				{
					FString Msg = FString::Printf(TEXT("Chunk:%s not contain any file!!!"), *Chunk.ChunkName);
					UE_LOG(LogHotPatcher, Warning, TEXT("%s"),*Msg);
					Context.OnShowMsg.Broadcast(Msg);
					continue;
				}
				
				if(!Chunk.bMonolithic)
				{
					FPakFileProxy SinglePakForChunk;
					SinglePakForChunk.Platform = Platform;
					SinglePakForChunk.PakCommands = ChunkPakListCommands;
					// add extern file to pak(version file)
					SinglePakForChunk.PakCommands.Append(Context.AdditionalFileToPak);

					FReplacePakRegular PakPathRegular{
						Context.CurrentVersion.VersionId,
						Context.CurrentVersion.BaseVersionId,
						Chunk.ChunkName,
						PlatformName
					};
					
					const FString ChunkPakName = UFlibHotPatcherCoreHelper::ReplacePakRegular(PakPathRegular,Context.GetSettingObject()->GetPakNameRegular());
					SinglePakForChunk.ChunkStoreName = ChunkPakName;
					SinglePakForChunk.StorageDirectory = ChunkSaveBasePath;
					Chunk.GetPakFileProxys().Add(SinglePakForChunk);
				}
				else
				{
					for (const auto& PakCommand : ChunkPakListCommands)
					{
						FPakFileProxy CurrentPak;
						CurrentPak.Platform = Platform;
						CurrentPak.PakCommands.Add(PakCommand);
						// add extern file to pak(version file)
						CurrentPak.PakCommands.Append(Context.AdditionalFileToPak);
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
						CurrentPak.ChunkStoreName = Path;
						CurrentPak.StorageDirectory = FPaths::Combine(ChunkSaveBasePath, Chunk.ChunkName);
						Chunk.GetPakFileProxys().Add(CurrentPak);
					}
				}
			}
		}
		return true;
	};

	// setup 9
	bool CreatePakWorker(FHotPatcherPatchContext& Context)
	{
		SCOPED_NAMED_EVENT_TEXT("CreatePakWorker",FColor::Red);
		TimeRecorder CreateAllPakToralTR(FString::Printf(TEXT("Generate all platform pak of all chunks Total Time:")));
		// PakModeSingleLambda(PlatformName, CurrentVersionSavePath);
		for (const auto& Chunk : Context.PakChunks)
		{
			TimeRecorder PakChunkToralTR(FString::Printf(TEXT("Package ChunkName:%s Total Time:"),*Chunk.ChunkName));
				
			// // Update SlowTask Progress
			{
				FText Dialog = FText::Format(NSLOCTEXT("ExportPatch_GeneratedPakList", "GeneratedPak", "Generating Pak list of Chunk {0}."), FText::FromString(Chunk.ChunkName));
				Context.OnPaking.Broadcast(TEXT("GeneratedPak"),*Dialog.ToString());
				Context.UnrealPakSlowTask->EnterProgressFrame(1.0, Dialog);
			}

			TArray<FString> UnrealPakCommandletGeneralOptions = Context.GetSettingObject()->GetUnrealPakSettings().UnrealCommandletOptions;
			UnrealPakCommandletGeneralOptions.Append(Context.GetSettingObject()->GetDefaultCommandletOptions());
			
			TArray<FReplaceText> ReplacePakListTexts = Context.GetSettingObject()->GetReplacePakListTexts();
			TArray<FThreadWorker> PakWorker;
				
			// TMap<FString,TArray<FPakFileInfo>>& PakFilesInfoMap = Context.PakFilesInfoMap;
				
			// 创建chunk的pak文件
			for (const auto& PakFileProxy : Chunk.GetPakFileProxys())
			{
				FString PlatformName = THotPatcherTemplateHelper::GetEnumNameByValue(PakFileProxy.Platform);
				TArray<FString> UnrealPakCommandletOptions;
				UnrealPakCommandletOptions.Add(FString::Printf(TEXT("-AlignForMemoryMapping=%d"),UFlibHotPatcherCoreHelper::GetMemoryMappingAlignment(PlatformName)));
				// can override
				UnrealPakCommandletOptions.Append(UnrealPakCommandletGeneralOptions);
				UnrealPakCommandletOptions.Add(UFlibHotPatcherCoreHelper::GetEncryptSettingsCommandlineOptions(Context.GetSettingObject()->GetEncryptSettings(),UFlibHotPatcherCoreHelper::Conv2IniPlatform(PlatformName)));
				TimeRecorder CookAssetsTR(FString::Printf(TEXT("Create Pak Platform:%s ChunkName:%s."),*PlatformName,*Chunk.ChunkName));
				// ++PakCounter;
				uint32 index = PakWorker.Emplace(*PakFileProxy.ChunkStoreName, [/*CurrentPakVersion, */PlatformName, UnrealPakCommandletOptions, ReplacePakListTexts, PakFileProxy, &Chunk,&Context]()
				{
					FString PakListFile = FPaths::Combine(PakFileProxy.StorageDirectory, FString::Printf(TEXT("%s_PakCommands.txt"), *PakFileProxy.ChunkStoreName));
					FString PakSavePath = FPaths::Combine(PakFileProxy.StorageDirectory, FString::Printf(TEXT("%s.pak"), *PakFileProxy.ChunkStoreName));
					bool PakCommandSaveStatus = FFileHelper::SaveStringArrayToFile(
						UFlibPatchParserHelper::GetPakCommandStrByCommands(PakFileProxy.PakCommands, ReplacePakListTexts,false),
						*PakListFile,
						FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
					if (PakCommandSaveStatus)
					{
						TArray<FString> UnrealPakCommandletOptionsSinglePak;
						UnrealPakCommandletOptionsSinglePak.Add(
							FString::Printf(
								TEXT("%s -create=%s"),
								*(TEXT("\"") + PakSavePath + TEXT("\"")),
								*(TEXT("\"") + PakListFile + TEXT("\""))
							)
						);
						UnrealPakCommandletOptionsSinglePak.Append(UnrealPakCommandletOptions);
							
						FString CommandLine;
						for (const auto& Option : UnrealPakCommandletOptionsSinglePak)
						{
							CommandLine.Append(FString::Printf(TEXT(" %s"), *Option));
						}
						Context.OnPaking.Broadcast(TEXT("Create Pak Commandline: %s"),CommandLine);
						UE_LOG(LogHotPatcher,Log,TEXT("Create Pak Commandline: %s"),*CommandLine);
						ExecuteUnrealPak(*CommandLine);
						// FProcHandle ProcessHandle = UFlibPatchParserHelper::DoUnrealPak(UnrealPakCommandletOptionsSinglePak, true);

						// AsyncTask(ENamedThreads::GameThread, [this,PakFileProxy,&PakFilesInfoMap,PlatformName]()
						{
							if (FPaths::FileExists(PakSavePath))
							{
								if(::IsRunningCommandlet())
								{
									FString Msg = FString::Printf(TEXT("Package the Patch as %s."),*PakSavePath);
									Context.OnPaking.Broadcast(TEXT("SavedPakFile"),Msg);
								}else
								{
									FString MsgStr = FString::Printf(TEXT("Package %s for %s Successfuly."),*Chunk.ChunkName,*PlatformName);
									FText Msg = UKismetTextLibrary::Conv_StringToText(MsgStr);
									FHotPatcherDelegates::Get().GetNotifyFileGenerated().Broadcast(Msg,PakSavePath);
								}
								FPakFileInfo CurrentPakInfo;
								if (UFlibPatchParserHelper::GetPakFileInfo(PakSavePath, CurrentPakInfo))
								{
									// CurrentPakInfo.PakVersion = CurrentPakVersion;
									if (!Context.PakFilesInfoMap.PakFilesMap.Contains(PlatformName))
									{
										FPakFileArray PakFileArray;
										PakFileArray.PakFileInfos = TArray<FPakFileInfo>{CurrentPakInfo};
										Context.PakFilesInfoMap.PakFilesMap.Add(PlatformName, PakFileArray);
									}
									else
									{
										Context.PakFilesInfoMap.PakFilesMap.Find(PlatformName)->PakFileInfos.Add(CurrentPakInfo);
									}
								}
							}
						}//);
							
						if (!(Context.GetSettingObject()->bStorageUnrealPakList && Chunk.bStorageUnrealPakList && Chunk.bOutputDebugInfo))
						{
							IFileManager::Get().Delete(*PakListFile);
						}
					}
				});
				PakWorker[index].Run();
			}
		}
		return true;
	};

	// setup 9
	bool CreateIoStoreWorker(FHotPatcherPatchContext& Context)
	{
		SCOPED_NAMED_EVENT_TEXT("CreateIoStoreWorker",FColor::Red);
#if WITH_IO_STORE_SUPPORT
		if(!Context.GetSettingObject()->GetIoStoreSettings().bIoStore)
			return true;
		TimeRecorder CreateAllIoStoreToralTR(FString::Printf(TEXT("Generate all platform Io Store of all chunks Total Time:")));

		TArray<FString> AdditionalIoStoreCommandletOptions;
		AdditionalIoStoreCommandletOptions.Append(Context.GetSettingObject()->GetDefaultCommandletOptions());
		AdditionalIoStoreCommandletOptions.Append(Context.GetSettingObject()->GetIoStoreSettings().IoStoreCommandletOptions);
		
		FIoStoreSettings IoStoreSettings = Context.GetSettingObject()->GetIoStoreSettings();
		// PakModeSingleLambda(PlatformName, CurrentVersionSavePath);
		for (const auto& Chunk : Context.PakChunks)
		{
			TimeRecorder PakChunkAllIoStoreToralTR(FString::Printf(TEXT("Package ChunkName:%s Io Store Total Time:"),*Chunk.ChunkName));
				
			// // Update SlowTask Progress
			{
				FText Dialog = FText::Format(NSLOCTEXT("ExportPatch_GeneratedPak", "GeneratedPak", "Generating Io Store list of Chunk {0}."), FText::FromString(Chunk.ChunkName));
				Context.OnPaking.Broadcast(TEXT("GeneratedIoStore"),*Dialog.ToString());
				Context.UnrealPakSlowTask->EnterProgressFrame(1.0, Dialog);
			}

			TArray<FReplaceText> ReplacePakListTexts = Context.GetSettingObject()->GetReplacePakListTexts();
			if(!Chunk.GetPakFileProxys().Num())
			{
				UE_LOG(LogHotPatcher,Error,TEXT("Chunk %s Not Contain Any valid PakFileProxy!!!"),*Chunk.ChunkName);
				continue;
			}
			// 创建chunk的Io Store文件
			TArray<FString> IoStoreCommands;
			for (const auto& PakFileProxy : Chunk.GetPakFileProxys())
			{
				if(!IoStoreSettings.PlatformContainers.Contains(PakFileProxy.Platform))
					return true;
				TArray<FString> FinalIoStoreCommandletOptions;
				FString  PlatformName = THotPatcherTemplateHelper::GetEnumNameByValue(PakFileProxy.Platform);
				FinalIoStoreCommandletOptions.Add(FString::Printf(TEXT("-AlignForMemoryMapping=%d"),UFlibHotPatcherCoreHelper::GetPlatformByName(PlatformName)->GetMemoryMappingAlignment()));
				FinalIoStoreCommandletOptions.Append(AdditionalIoStoreCommandletOptions);
				FString IoStoreCommandletOptions;
				for(const auto& Option:FinalIoStoreCommandletOptions) { IoStoreCommandletOptions+=FString::Printf(TEXT("%s "),*Option); }
				IoStoreCommandletOptions += UFlibHotPatcherCoreHelper::GetEncryptSettingsCommandlineOptions(Context.GetSettingObject()->GetEncryptSettings(),UFlibHotPatcherCoreHelper::Conv2IniPlatform(PlatformName));
					
				TimeRecorder CookAssetsTR(FString::Printf(TEXT("Create Pak Platform:%s ChunkName:%s."),*PlatformName,*Chunk.ChunkName));
				FString PakListFile = FPaths::Combine(PakFileProxy.StorageDirectory, FString::Printf(TEXT("%s_IoStorePakList.txt"), *PakFileProxy.ChunkStoreName));
				FString PakSavePath = FPaths::Combine(PakFileProxy.StorageDirectory, FString::Printf(TEXT("%s.utoc"), *PakFileProxy.ChunkStoreName));

				FString PatchSource;
				
				FString BasePacakgeRootDir = FPaths::ConvertRelativePathToFull(UFlibPatchParserHelper::ReplaceMarkPath(Context.GetSettingObject()->GetIoStoreSettings().PlatformContainers.Find(PakFileProxy.Platform)->BasePackageStagedRootDir.Path));
				if(FPaths::DirectoryExists(BasePacakgeRootDir))
				{
					PatchSource = FPaths::Combine(BasePacakgeRootDir,FApp::GetProjectName(),FString::Printf(TEXT("Content/Paks/%s*.utoc"),FApp::GetProjectName()));
				}
					
				FString PatchSoruceSetting = UFlibPatchParserHelper::ReplaceMarkPath(IoStoreSettings.PlatformContainers.Find(PakFileProxy.Platform)->PatchSourceOverride.FilePath);
				if(FPaths::FileExists(PatchSoruceSetting))
				{
					PatchSource = PatchSoruceSetting;
				}
					
				bool PakCommandSaveStatus = FFileHelper::SaveStringArrayToFile(
					UFlibPatchParserHelper::GetPakCommandStrByCommands(PakFileProxy.PakCommands, ReplacePakListTexts,true),
					*PakListFile,
					FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
				FString PatchDiffCommand = IoStoreSettings.PlatformContainers.Find(PakFileProxy.Platform)->bGenerateDiffPatch ?
					FString::Printf(TEXT("-PatchSource=\"%s\" -GenerateDiffPatch"),*PatchSource):TEXT("");
					
				IoStoreCommands.Emplace(
					FString::Printf(TEXT("-Output=\"%s\" -ContainerName=\"%s\" -ResponseFile=\"%s\" %s"),
						*PakSavePath,
						*PakFileProxy.ChunkStoreName,
						*PakListFile,
						*PatchDiffCommand
					)
				);
#if ENGINE_MAJOR_VERSION < 5 && ENGINE_MINOR_VERSION < 26
				FString OutputDirectoryCmd = FString::Printf(TEXT("-OutputDirectory=%s"),*FPaths::Combine(Context.GetSettingObject()->GetSaveAbsPath()));
#else
				FString OutputDirectoryCmd = TEXT("");
#endif
				FString IoStoreCommandsFile = FPaths::Combine(Chunk.GetPakFileProxys()[0].StorageDirectory,FString::Printf(TEXT("%s_IoStoreCommands.txt"),*Chunk.ChunkName));
					
				FString PlatformGlocalContainers;
				// FString BasePacakgeRootDir = FPaths::ConvertRelativePathToFull(UFlibPatchParserHelper::ReplaceMarkPath(Context.GetSettingObject()->GetIoStoreSettings().PlatformContainers.Find(PakFileProxy.Platform)->BasePackageStagedRootDir.Path));
				if(FPaths::DirectoryExists(BasePacakgeRootDir))
				{
					PlatformGlocalContainers = FPaths::Combine(BasePacakgeRootDir,FApp::GetProjectName(),TEXT("Content/Paks/global.utoc"));
				}
				FString GlobalUtocFile = UFlibPatchParserHelper::ReplaceMarkPath(IoStoreSettings.PlatformContainers.Find(PakFileProxy.Platform)->GlobalContainersOverride.FilePath);
				if(FPaths::FileExists(GlobalUtocFile))
				{
					PlatformGlocalContainers = GlobalUtocFile;
				}
					
				FString PlatformCookDir = FPaths::ConvertRelativePathToFull(FPaths::Combine(Context.GetSettingObject()->GetStorageCookedDir(),PlatformName));

				if(FPaths::FileExists(PlatformGlocalContainers))
				{
					FString CookOrderFile = FPaths::Combine(Context.GetSettingObject()->GetSaveAbsPath(),Context.NewVersionChunk.ChunkName,PlatformName,TEXT("CookOpenOrder.txt"));
					FFileHelper::SaveStringArrayToFile(IoStoreCommands,*IoStoreCommandsFile);
					FString IoStoreCommandlet = FString::Printf(
						TEXT("-CreateGlobalContainer=\"%s\" -CookedDirectory=\"%s\" -Commands=\"%s\" -CookerOrder=\"%s\" -TargetPlatform=\"%s\" %s %s"),
						// *UFlibHotPatcherCoreHelper::GetUECmdBinary(),
						// *UFlibPatchParserHelper::GetProjectFilePath(),
						*PlatformGlocalContainers,
						*PlatformCookDir,
						*IoStoreCommandsFile,
						*CookOrderFile,
						*PlatformName,
						*IoStoreCommandletOptions,
						*OutputDirectoryCmd
					);
					FString IoStoreNativeCommandlet = FString::Printf(
						TEXT("\"%s\" \"%s\" -run=IoStore %s"),
						*UFlibHotPatcherCoreHelper::GetUECmdBinary(),
						*UFlibPatchParserHelper::GetProjectFilePath(),
						*IoStoreCommandlet
					);
						
					// FCommandLine::Set(*IoStoreCommandlet);
					UE_LOG(LogHotPatcher,Log,TEXT("%s"),*IoStoreNativeCommandlet);
					// CreateIoStoreContainerFiles(*IoStoreCommandlet);

					TSharedPtr<FProcWorkerThread> IoStoreProc = MakeShareable(
						new FProcWorkerThread(
							TEXT("PakIoStoreThread"),
							UFlibHotPatcherCoreHelper::GetUECmdBinary(),
							FString::Printf(TEXT("\"%s\" -run=IoStore %s"), *UFlibPatchParserHelper::GetProjectFilePath(),*IoStoreCommandlet)
						)
					);
					IoStoreProc->ProcOutputMsgDelegate.BindStatic(&::ReceiveOutputMsg);
					IoStoreProc->Execute();
					IoStoreProc->Join();
				}
			}
		}
#endif
		return true;
	};

	// setup 11 save difference to file
	bool SaveDifferenceWorker(FHotPatcherPatchContext& Context)
	{
		SCOPED_NAMED_EVENT_TEXT("SaveDifferenceWorker",FColor::Red);
		if(Context.GetPakFileNum())
		{
			TimeRecorder SaveDiffTR(FString::Printf(TEXT("Save Patch Diff info")));
			FText DiaLogMsg = FText::Format(NSLOCTEXT("ExportPatch", "ExportPatchDiffFile", "Generating Diff info of version {0}"), FText::FromString(Context.CurrentVersion.VersionId));

			auto SavePatchDiffJsonLambda = [&Context](const FHotPatcherVersion& InSaveVersion, const FPatchVersionDiff& InDiff)->bool
			{
				bool bStatus = false;
				for(ETargetPlatform Platform:Context.GetSettingObject()->PakTargetPlatforms)
				{
					FString PlatformName = THotPatcherTemplateHelper::GetEnumNameByValue(Platform);
					if (Context.GetSettingObject()->IsSaveDiffAnalysis())
					{
						auto SerializeChangedAssetInfo = [](const FPatchVersionDiff& InAssetInfo)->FString
						{
							FString AddAssets;
							THotPatcherTemplateHelper::TSerializeStructAsJsonString(InAssetInfo,AddAssets);
							return AddAssets;
						};
		
						FString SerializeDiffInfo = SerializeChangedAssetInfo(InDiff);
		
						// FString::Printf(TEXT("%s"),*SerializeDiffInfo);

						FString SaveDiffToFile = FPaths::Combine(
							// Context.GetSettingObject()->GetCurrentVersionSavePath(),
							Context.GetSettingObject()->GetChunkSavedDir(InSaveVersion.VersionId,InSaveVersion.BaseVersionId,TEXT(""),PlatformName),
							FString::Printf(TEXT("%s_%s_Diff.json"), *InSaveVersion.BaseVersionId, *InSaveVersion.VersionId)
						);
						if (UFlibAssetManageHelper::SaveStringToFile(SaveDiffToFile, SerializeDiffInfo))
						{
							bStatus = true;

							FString Msg = FString::Printf(TEXT("Succeed to export New Patch Diff Info."),*SaveDiffToFile);
							Context.OnPaking.Broadcast(TEXT("SavePatchDiffInfo"),Msg);
						}
					}
				}
				return bStatus;
			};
			
			SavePatchDiffJsonLambda(Context.CurrentVersion, Context.VersionDiff);
			
			if(::IsRunningCommandlet())
			{
				Context.OnPaking.Broadcast(TEXT("ExportPatchDiffFile"),*DiaLogMsg.ToString());
			
			}
			else
			{
				Context.UnrealPakSlowTask->EnterProgressFrame(1.0, DiaLogMsg);
			}
		}
		return true;
	};

	// setup 12
	bool SaveNewReleaseWorker(FHotPatcherPatchContext& Context)
	{
		SCOPED_NAMED_EVENT_TEXT("SaveNewReleaseWorker",FColor::Red);
		if(!Context.GetSettingObject()->IsStorageNewRelease())
			return true;
		// save Patch Tracked asset info to file
		if(Context.GetPakFileNum())
		{
			TimeRecorder TR(FString::Printf(TEXT("Save New Release info")));
			FText DiaLogMsg = FText::Format(NSLOCTEXT("ExportPatch", "ExportPatchAssetInfo", "Generating Patch Tacked Asset info of version {0}"), FText::FromString(Context.CurrentVersion.VersionId));
			if(::IsRunningCommandlet())
			{
				FString Msg = FString::Printf(TEXT("Generating Patch Tacked Asset info of version %s."),*Context.CurrentVersion.VersionId);
				Context.OnPaking.Broadcast(TEXT("ExportPatchAssetInfo"),DiaLogMsg.ToString());
			}
			else
			{
				Context.UnrealPakSlowTask->EnterProgressFrame(1.0, DiaLogMsg);
			}
		
			FString SerializeReleaseVersionInfo;
			Context.NewReleaseVersion = UFlibHotPatcherCoreHelper::MakeNewReleaseByDiff(Context.BaseVersion, Context.VersionDiff,Context.GetSettingObject());
			THotPatcherTemplateHelper::TSerializeStructAsJsonString(Context.NewReleaseVersion, SerializeReleaseVersionInfo);

			FString SaveCurrentVersionToFile = FPaths::Combine(
				Context.GetSettingObject()->GetCurrentVersionSavePath(),
				FString::Printf(TEXT("%s_Release.json"), *Context.CurrentVersion.VersionId)
			);
			if (UFlibAssetManageHelper::SaveStringToFile(SaveCurrentVersionToFile, SerializeReleaseVersionInfo))
			{
				if(::IsRunningCommandlet())
				{
					FString Msg = FString::Printf(TEXT("Export NewRelease to %s."),*SaveCurrentVersionToFile);
					Context.OnPaking.Broadcast(TEXT("SavePatchDiffInfo"),Msg);
				}else
				{
					auto Msg = LOCTEXT("SavePatchDiffInfo", "Export NewRelease Successfuly.");
					FHotPatcherDelegates::Get().GetNotifyFileGenerated().Broadcast(Msg, SaveCurrentVersionToFile);
				}
			}
		}
		return true;
	};

	// setup 13 serialize all pak file info
	bool SavePakFileInfoWorker(FHotPatcherPatchContext& Context)
	{
		SCOPED_NAMED_EVENT_TEXT("SavePakFileInfoWorker",FColor::Red);
		const FPatherResult& PatherResult = Context.PatchProxy->GetPatcherResult();
		FString PakResultPath = FPaths::Combine(
			Context.GetSettingObject()->GetCurrentVersionSavePath(),
			FString::Printf(TEXT("%s_PakResults.json"),*Context.CurrentVersion.VersionId)
		);
		if(PatherResult.PatcherAssetDetails.Num())
		{
			ExportStructToFile(Context,PatherResult,PakResultPath,true,TEXT("PakResults"));
		}
		if(!Context.GetSettingObject()->IsStoragePakFileInfo())
			return true;
		if(Context.GetSettingObject())
		{
			TimeRecorder TR(FString::Printf(TEXT("Save All Pak file info")));
			FText DiaLogMsg = FText::Format(NSLOCTEXT("ExportPatch", "ExportPatchPakFileInfo", "Generating All Platform Pak info of version {0}"), FText::FromString(Context.CurrentVersion.VersionId));
			if(::IsRunningCommandlet())
			{
				Context.OnPaking.Broadcast(TEXT("ExportPatchPakFileInfo"),*DiaLogMsg.ToString());
			}
			else
			{
				Context.UnrealPakSlowTask->EnterProgressFrame(1.0,DiaLogMsg);
			}
			FString PakFilesInfoStr;
			THotPatcherTemplateHelper::TSerializeStructAsJsonString(Context.PakFilesInfoMap, PakFilesInfoStr);

			if (!PakFilesInfoStr.IsEmpty())
			{
				FString SavePakFilesPath = FPaths::Combine(
					Context.GetSettingObject()->GetCurrentVersionSavePath(),
					FString::Printf(TEXT("%s_PakFilesInfo.json"), *Context.CurrentVersion.VersionId)
				);
				if (UFlibAssetManageHelper::SaveStringToFile(SavePakFilesPath, PakFilesInfoStr) && FPaths::FileExists(SavePakFilesPath))
				{
					if(::IsRunningCommandlet())
					{
						FString Msg = FString::Printf(TEXT("Export PakFileInfo to %s."),*SavePakFilesPath);
						Context.OnPaking.Broadcast(TEXT("SavedPakFileMsg"),Msg);
					}else
					{
						FText Msg = LOCTEXT("SavedPakFileMsg_ExportSuccessed", "Export PakFileInfo Successfuly.");
						FHotPatcherDelegates::Get().GetNotifyFileGenerated().Broadcast(Msg, SavePakFilesPath);
					}
				}
			}
		}
		return true;
	};
	
	// setup 14 serialize patch config
	bool SavePatchConfigWorker(FHotPatcherPatchContext& Context)
	{
		SCOPED_NAMED_EVENT_TEXT("SavePatchConfigWorker",FColor::Red);
		
		TimeRecorder TR(FString::Printf(TEXT("Save patch config")));
		FText DiaLogMsg = FText::Format(NSLOCTEXT("ExportPatch", "ExportPatchConfig", "Generating Current Patch Config of version {0}"), FText::FromString(Context.CurrentVersion.VersionId));
		if(::IsRunningCommandlet())
		{
			Context.OnPaking.Broadcast(TEXT("ExportPatchConfig"),*DiaLogMsg.ToString());	
		}
		else
		{	
			Context.UnrealPakSlowTask->EnterProgressFrame(1.0, DiaLogMsg);
		}
		if (Context.GetSettingObject()->IsSaveConfig())
		{
			TArray<ETargetPlatform> PakTargetPlatforms = Context.GetSettingObject()->GetPakTargetPlatforms();
			for(ETargetPlatform Platform:PakTargetPlatforms)
			{
				FString PlatformName = THotPatcherTemplateHelper::GetEnumNameByValue(Platform);
				FString SavedBaseDir = Context.GetSettingObject()->GetChunkSavedDir(Context.CurrentVersion.VersionId,Context.CurrentVersion.BaseVersionId,Context.GetSettingObject()->GetVersionId(),PlatformName);
				FString SaveConfigPath = FPaths::Combine(
				SavedBaseDir,
					FString::Printf(TEXT("%s_%s_PatchConfig.json"),*PlatformName,*Context.CurrentVersion.VersionId)
				);
				FExportPatchSettings TempPatchSettings = *Context.GetSettingObject();
				TempPatchSettings.PakTargetPlatforms.Empty();
				TempPatchSettings.PakTargetPlatforms.Add(Platform);
				ExportStructToFile(Context,TempPatchSettings,SaveConfigPath,false,TEXT("PatchConfig"));
			}
			if(PakTargetPlatforms.Num())
			{
				FString SaveConfigPath = FPaths::Combine(
					Context.GetSettingObject()->GetCurrentVersionSavePath(),
					FString::Printf(TEXT("%s_PatchConfig.json"),*Context.CurrentVersion.VersionId)
				);
				ExportStructToFile(Context,*Context.GetSettingObject(),SaveConfigPath,true,TEXT("PatchConfig"));
			}
		}
		
		return true;
	};
	
	// setup 15
	// bool BackupMetadataWorker(FHotPatcherPatchContext& Context)
	// {
	// 	SCOPED_NAMED_EVENT_TEXT("BackupMetadataWorker",FColor::Red);
	// 	// backup Metadata
	// 	if(Context.GetPakFileNum())
	// 	{
	// 		TimeRecorder TR(FString::Printf(TEXT("Backup Metadata")));
	// 		FText DiaLogMsg = FText::Format(NSLOCTEXT("BackupMetadata", "BackupMetadata", "Backup Release {0} Metadatas."), FText::FromString(Context.GetSettingObject()->GetVersionId()));
	// 		Context.UnrealPakSlowTask->EnterProgressFrame(1.0, DiaLogMsg);
	// 		if(Context.GetSettingObject()->IsBackupMetadata())
	// 		{
	// 			UFlibHotPatcherCoreHelper::BackupMetadataDir(
	// 				FPaths::ProjectDir(),
	// 				FApp::GetProjectName(),
	// 				Context.GetSettingObject()->GetPakTargetPlatforms(),
	// 				FPaths::Combine(Context.GetSettingObject()->GetSaveAbsPath(),
	// 				Context.GetSettingObject()->GetVersionId())
	// 			);
	// 		}
	// 	}
	// 	return true;
	// };

	// setup 16
	bool ShowSummaryWorker(FHotPatcherPatchContext& Context)
	{
		SCOPED_NAMED_EVENT_TEXT("ShowSummaryWorker",FColor::Red);
		// show summary infomation
		if(Context.GetPakFileNum())
		{
			Context.OnShowMsg.Broadcast(UFlibHotPatcherCoreHelper::PatchSummary(Context.VersionDiff));
			Context.OnShowMsg.Broadcast(UFlibHotPatcherCoreHelper::ReleaseSummary(Context.NewReleaseVersion));
		}
		return true;
	};

	// setup 17
	bool OnFaildDispatchWorker(FHotPatcherPatchContext& Context)
	{
		SCOPED_NAMED_EVENT_TEXT("OnFaildDispatchWorker",FColor::Red);
		if (!Context.GetPakFileNum())
		{
			UE_LOG(LogHotPatcher, Warning, TEXT("The Patch not contain any invalie file!"));
			Context.OnShowMsg.Broadcast(TEXT("The Patch not contain any invalie file!"));
		}
		else
		{
			Context.OnShowMsg.Broadcast(TEXT(""));
		}
		return true;
	};

};

#undef LOCTEXT_NAMESPACE
