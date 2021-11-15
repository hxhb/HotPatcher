#include "CreatePatch/PatcherProxy.h"
#include "HotPatcherLog.h"
#include "ThreadUtils/FThreadUtils.hpp"
#include "CreatePatch/HotPatcherContext.h"
#include "CreatePatch/ScopedSlowTaskContext.h"
#include "CreatePatch/HotPatcherContext.h"
#include "HotPatcherEditor.h"

// engine header
#include "Async/Async.h"
#include "CoreGlobals.h"
#include "FlibHotPatcherEditorHelper.h"
#include "Dom/JsonValue.h"
#include "HAL/PlatformFilemanager.h"
#include "Kismet/KismetStringLibrary.h"
#include "PakFileUtilities.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/FileHelper.h"
#include "Templates/Function.h"
#include "BinariesPatchFeature.h"
#include "Async/ParallelFor.h"
#include "Misc/DataDrivenPlatformInfoRegistry.h"
#include "Serialization/ArrayWriter.h"
#include "ShaderPatch/FlibShaderCodeLibraryHelper.h"
#include "AssetRegistryState.h"


#if WITH_IO_STORE_SUPPORT
#include "IoStoreUtilities.h"
#endif

#define LOCTEXT_NAMESPACE "HotPatcherProxy"

UPatcherProxy::UPatcherProxy(const FObjectInitializer& ObjectInitializer):Super(ObjectInitializer){}

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
	// setup 7
	bool CookPatchAssetsWorker(FHotPatcherPatchContext& Context);
	// setup 7.1
	bool PatchAssetRegistryWorker(FHotPatcherPatchContext& Context);
	// setup 7.2
	bool GenerateGlobalAssetRegistryData(FHotPatcherPatchContext& Context);
	// setup 8
	bool GeneratePakProxysWorker(FHotPatcherPatchContext& Context);
	// setup 9
	bool CreatePakWorker(FHotPatcherPatchContext& Context);
	// io store
	bool CreateIoStoreWorker(FHotPatcherPatchContext& Context);
	// setup 10
	bool SaveAssetDependenciesWorker(FHotPatcherPatchContext& Context);
	// setup 11 save difference to file
	bool SaveDifferenceWorker(FHotPatcherPatchContext& Context);
	// setup 12
	bool SaveNewReleaseWorker(FHotPatcherPatchContext& Context);
	// setup 13 serialize all pak file info
	bool SavePakFileInfoWorker(FHotPatcherPatchContext& Context);
	// setup 14 serialize patch config
	bool SavePatchConfigWorker(FHotPatcherPatchContext& Context);
	// setup 15
	bool BackupMetadataWorker(FHotPatcherPatchContext& Context);
	// setup 16
	bool ShowSummaryWorker(FHotPatcherPatchContext& Context);
	// setup 17
	bool OnFaildDispatchWorker(FHotPatcherPatchContext& Context);
	// setup 18
	bool NotifyOperatorsWorker(FHotPatcherPatchContext& Context);

	void GenerateBinariesPatch(FHotPatcherPatchContext& Context,FChunkInfo& Chunk,ETargetPlatform Platform,TArray<FPakCommand>& PakCommands);
}


bool UPatcherProxy::DoExport()
{
	UFLibAssetManageHelperEx::UpdateAssetMangerDatabase(true);
	GetSettingObject()->Init();
	
	PatchContext = MakeShareable(new FHotPatcherPatchContext);
	PatchContext->PatchProxy = this;
	PatchContext->OnPaking.AddLambda([this](const FString& One,const FString& Msg){this->OnPaking.Broadcast(One,Msg);});
	PatchContext->OnShowMsg.AddLambda([this](const FString& Msg){ this->OnShowMsg.Broadcast(Msg);});
	PatchContext->UnrealPakSlowTask = NewObject<UScopedSlowTaskContext>();
	PatchContext->UnrealPakSlowTask->AddToRoot();
	PatchContext->ContextSetting = GetSettingObject();
	PatchContext->Init();
	// TimeRecorder TotalTimeTR(TEXT("Generate the patch total time"));

	TArray<TFunction<bool(FHotPatcherPatchContext&)>> PreCookPatchWorkers;
	TArray<TFunction<bool(FHotPatcherPatchContext&)>> PostCookPatchWorkers;
	PreCookPatchWorkers.Emplace(&PatchWorker::BaseVersionReader);
	PreCookPatchWorkers.Emplace(&PatchWorker::MakeCurrentVersionWorker);
	PreCookPatchWorkers.Emplace(&PatchWorker::ParseVersionDiffWorker);
	PreCookPatchWorkers.Emplace(&PatchWorker::ParseDiffAssetOnlyWorker);
	PreCookPatchWorkers.Emplace(&PatchWorker::PatchRequireChekerWorker);
	PreCookPatchWorkers.Emplace(&PatchWorker::SavePakVersionWorker);
	PreCookPatchWorkers.Emplace(&PatchWorker::ParserChunkWorker);
	PreCookPatchWorkers.Emplace(&PatchWorker::CookPatchAssetsWorker);

	// wait cook complete
	if(PatchContext->GetSettingObject()->IsBinariesPatch())
	{
		this->OnPakListGenerated.AddStatic(&PatchWorker::GenerateBinariesPatch);
	}
	PostCookPatchWorkers.Emplace(&PatchWorker::PatchAssetRegistryWorker);
	PostCookPatchWorkers.Emplace(&PatchWorker::GenerateGlobalAssetRegistryData);
	PostCookPatchWorkers.Emplace(&PatchWorker::GeneratePakProxysWorker);
	PostCookPatchWorkers.Emplace(&PatchWorker::CreatePakWorker);
	PostCookPatchWorkers.Emplace(&PatchWorker::CreateIoStoreWorker);
	PostCookPatchWorkers.Emplace(&PatchWorker::SaveAssetDependenciesWorker);
	PostCookPatchWorkers.Emplace(&PatchWorker::SaveDifferenceWorker);
	PostCookPatchWorkers.Emplace(&PatchWorker::SaveNewReleaseWorker);
	PostCookPatchWorkers.Emplace(&PatchWorker::SavePakFileInfoWorker);
	PostCookPatchWorkers.Emplace(&PatchWorker::SavePatchConfigWorker);
	PostCookPatchWorkers.Emplace(&PatchWorker::BackupMetadataWorker);
	PostCookPatchWorkers.Emplace(&PatchWorker::ShowSummaryWorker);
	PostCookPatchWorkers.Emplace(&PatchWorker::OnFaildDispatchWorker);
	
	float AmountOfWorkProgress =  (float)PreCookPatchWorkers.Num() + (float)PostCookPatchWorkers.Num() + (float)(GetSettingObject()->GetPakTargetPlatforms().Num() * FMath::Max(PatchContext->PakChunks.Num(),1));
	PatchContext->UnrealPakSlowTask->init(AmountOfWorkProgress);

	bool bRet = true;
	for(TFunction<bool(FHotPatcherPatchContext&)> Worker:PreCookPatchWorkers)
	{
		if(!Worker(*PatchContext))
		{
			bRet = false;
			PatchContext->UnrealPakSlowTask->Final();
			break;
		}
	}
	
	if(bRet)
	{
		ThreadWorker = MakeShareable(new FThreadWorker(TEXT("PatchProxy_WaitCook"),[this,PostCookPatchWorkers]()
		{
			UPackage::WaitForAsyncFileWrites();
		}));
		ThreadWorker->Execute();
		ThreadWorker->Join();
		for(TFunction<bool(FHotPatcherPatchContext&)> Worker:PostCookPatchWorkers)
		{
			if(!Worker(*PatchContext))
			{
				bRet = false;
				break;
			}
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
		UE_LOG(LogHotPatcher,Display,TEXT("Make Patch Setting..."));
		TimeRecorder ExportNewVersionTR(TEXT("Make Release Chunk/Export Release Version Info By Chunk"));
		Context.NewVersionChunk = UFlibHotPatcherEditorHelper::MakeChunkFromPatchSettings(Context.GetSettingObject());

		UE_LOG(LogHotPatcher,Display,TEXT("Deserialize Release Version by Patch Setting..."));
		Context.CurrentVersion = UFlibPatchParserHelper::ExportReleaseVersionInfoByChunk(
			Context.GetSettingObject()->GetVersionId(),
			Context.BaseVersion.VersionId,
			FDateTime::UtcNow().ToString(),
			Context.NewVersionChunk,
			Context.GetSettingObject()->GetAssetsDependenciesScanedCaches(),
			Context.GetSettingObject()->IsIncludeHasRefAssetsOnly(),
			Context.GetSettingObject()->IsAnalysisFilterDependencies()
		);
		UE_LOG(LogHotPatcher,Display,TEXT("New Version total asset number is %d."),Context.GetSettingObject()->GetAssetsDependenciesScanedCaches().Num());
		return true;
	};

	// setup 3
	bool ParseVersionDiffWorker(FHotPatcherPatchContext& Context)
	{
		TimeRecorder DiffVersionTR(TEXT("Diff Base Version And Current Project Version"));
		Context.VersionDiff = UFlibHotPatcherEditorHelper::DiffPatchVersionWithPatchSetting(*Context.GetSettingObject(), Context.BaseVersion, Context.CurrentVersion);
		return true;
	};

	bool ParseDiffAssetOnlyWorker(FHotPatcherPatchContext& Context)
	{
		
		TimeRecorder DiffVersionAssetOnlyTR(TEXT("Parse Diff Asset Dependencies Only Worker"));
		if(Context.GetSettingObject()->IsAnalysisDiffAssetDependenciesOnly())
		{
			FAssetDependenciesInfo AddAndModifyInfo = UFLibAssetManageHelperEx::CombineAssetDependencies(Context.VersionDiff.AssetDiffInfo.AddAssetDependInfo,Context.VersionDiff.AssetDiffInfo.ModifyAssetDependInfo);
			TArray<FAssetDetail> DiffAssetDetails;
			UFLibAssetManageHelperEx::GetAssetDetailsByAssetDependenciesInfo(AddAndModifyInfo,DiffAssetDetails);

			FChunkInfo DiffChunk;
			for(const auto& AssetDetail:DiffAssetDetails)
			{
				FPatcherSpecifyAsset CurrentAsets;
				CurrentAsets.Asset = FSoftObjectPath(AssetDetail.mPackagePath);
				CurrentAsets.bAnalysisAssetDependencies = true;
				CurrentAsets.AssetRegistryDependencyTypes.AddUnique(EAssetRegistryDependencyTypeEx::Packages);
				DiffChunk.IncludeSpecifyAssets.Add(CurrentAsets);
			}
			
			FHotPatcherVersion DiffVersion = UFlibPatchParserHelper::ExportReleaseVersionInfoByChunk(
				Context.GetSettingObject()->GetVersionId(),
				Context.BaseVersion.VersionId,
				FDateTime::UtcNow().ToString(),
				DiffChunk,
				Context.GetSettingObject()->GetAssetsDependenciesScanedCaches(),
				Context.GetSettingObject()->IsIncludeHasRefAssetsOnly(),
				Context.GetSettingObject()->IsAnalysisFilterDependencies()
			);
			{
				TimeRecorder DiffTR(TEXT("Base Version And Diff Version total time"));
				TMap<ETargetPlatform,FPatchVersionExternDiff> BackupExternalFiles = Context.VersionDiff.PlatformExternDiffInfo;
				Context.VersionDiff = UFlibHotPatcherEditorHelper::DiffPatchVersionWithPatchSetting(*Context.GetSettingObject(), Context.BaseVersion, DiffVersion);
				Context.VersionDiff.PlatformExternDiffInfo = BackupExternalFiles;
			}
		}
		return true;
	}
	// setup 4
	bool PatchRequireChekerWorker(FHotPatcherPatchContext& Context)
	{
		bool result = true;
		TimeRecorder CheckRequireTR(TEXT("Check Patch Require"));
		FString ReceiveMsg;
		if (!Context.GetSettingObject()->IsCookPatchAssets() && !UFlibHotPatcherEditorHelper::CheckPatchRequire(Context.VersionDiff,Context.GetSettingObject()->GetPakTargetPlatformNames(), ReceiveMsg))
		{
			Context.OnShowMsg.Broadcast(ReceiveMsg);
			result = false;
		}
		return result;
	};

	// setup 5
	bool SavePakVersionWorker(FHotPatcherPatchContext& Context)
	{
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
						if (UFlibPakHelper::SerializePakVersionToString(OutPakVersion, OutString))
						{
							bStatus = UFLibAssetManageHelperEx::SaveStringToFile(SavePakVersionFilePath, OutString);
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
		// Check Chunk
		if(Context.GetSettingObject()->IsEnableChunk())
		{
			TimeRecorder AnalysisChunkTR(TEXT("Analysis Chunk Info(enable chunk)"));
			FString TotalMsg;
			FChunkInfo TotalChunk = UFlibPatchParserHelper::CombineChunkInfos(Context.GetSettingObject()->GetChunkInfos());

			FChunkAssetDescribe ChunkDiffInfo = UFlibHotPatcherEditorHelper::DiffChunkWithPatchSetting(
				*Context.GetSettingObject(),
				Context.NewVersionChunk,
				TotalChunk,
				Context.GetSettingObject()->GetAssetsDependenciesScanedCaches()
			);
			
			if(ChunkDiffInfo.HasValidAssets())
			{
				if(Context.GetSettingObject()->IsCreateDefaultChunk())
				{
				
					Context.PakChunks.Add(ChunkDiffInfo.AsChunkInfo(TEXT("Default")));
				}
				else
				{
					TArray<FString> AllUnselectedAssets = ChunkDiffInfo.GetAssetsStrings();
					TArray<FString> AllUnselectedExFiles;
					for(auto Platform:Context.GetSettingObject()->GetPakTargetPlatforms())
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
		else
		{
			TimeRecorder AnalysisChunkTR(TEXT("Analysis Chunk Info(not enable chunk)"));
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
				GetKeysLambda(Context.VersionDiff.AssetDiffInfo.AddAssetDependInfo);
				GetKeysLambda(Context.VersionDiff.AssetDiffInfo.ModifyAssetDependInfo);

				TArray<FDirectoryPath> DepOtherModule;

				for (const auto& DependenciesFilter : DependenciesFilters)
				{
					if (!!Context.NewVersionChunk.AssetIncludeFilters.Num())
					{
						for (const auto& includeFilter : Context.NewVersionChunk.AssetIncludeFilters)
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
				Context.NewVersionChunk.AssetIncludeFilters.Append(DepOtherModule);
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


	// setup 7
	bool CookPatchAssetsWorker(FHotPatcherPatchContext& Context)
	{
		TimeRecorder CookAssetsTotalTR(FString::Printf(TEXT("Cook All Assets in Patch Total time:")));
		for(const auto& PlatformName :Context.GetSettingObject()->GetPakTargetPlatformNames())
		{
			for(auto& Chunk:Context.PakChunks)
			{
				TimeRecorder CookPlatformChunkAssetsTotalTR(FString::Printf(TEXT("Cook Chunk %s as %s Total time:"),*Chunk.ChunkName,*PlatformName));
				TSharedPtr<FCookShaderCollectionProxy> CookShaderCollection;
				if(Context.GetSettingObject()->GetCookShaderOptions().bSharedShaderLibrary)
				{
					FString SavePath = FPaths::Combine(Context.GetSettingObject()->GetSaveAbsPath(),Context.CurrentVersion.VersionId);
					FString ActualLibraryName = UFlibShaderCodeLibraryHelper::GenerateShaderCodeLibraryName(FApp::GetProjectName(),false);
					FString ShaderLibraryName = Context.GetSettingObject()->GetShaderLibraryName();
					CookShaderCollection = MakeShareable(
						new FCookShaderCollectionProxy(
							PlatformName,
							ShaderLibraryName,
							Context.GetSettingObject()->GetCookShaderOptions().bNativeShader,
							SavePath
							));
					CookShaderCollection->Init();
				}
			
				if(Context.GetSettingObject()->IsCookPatchAssets() || Context.GetSettingObject()->GetIoStoreSettings().bIoStore)
				{
					TimeRecorder CookAssetsTR(FString::Printf(TEXT("Cook %s assets in the patch."),*PlatformName));

					ETargetPlatform Platform;
					UFlibPatchParserHelper::GetEnumValueByName(PlatformName,Platform);

					FChunkAssetDescribe ChunkAssetsDescrible = UFlibPatchParserHelper::CollectFChunkAssetsDescribeByChunk(
						Context.VersionDiff,
						Chunk,
						TArray<ETargetPlatform>{Platform},
						Context.GetSettingObject()->GetAssetsDependenciesScanedCaches()
					);
					TArray<FAssetDetail> ChunkAssets;
					UFLibAssetManageHelperEx::GetAssetDetailsByAssetDependenciesInfo(ChunkAssetsDescrible.Assets,ChunkAssets);

					if(Context.GetSettingObject()->IsCookPatchAssets())
					{
						UFlibHotPatcherEditorHelper::CookChunkAssets(
							ChunkAssets,
							TArray<ETargetPlatform>{Platform}
	#if WITH_PACKAGE_CONTEXT
							,Context.GetSettingObject()->GetPlatformSavePackageContexts()
	#endif
						);
						if(Context.GetSettingObject()->GetIoStoreSettings().bStorageBulkDataInfo)
						{
#if WITH_PACKAGE_CONTEXT
							Context.GetSettingObject()->SavePlatformBulkDataManifest(Platform);
#endif
						}
					}
				
					if(Context.GetSettingObject()->GetIoStoreSettings().bIoStore)
					{
						TimeRecorder StorageCookOpenOrderTR(FString::Printf(TEXT("Storage CookOpenOrder.txt for %s, time:"),*PlatformName));
						struct CookOpenOrder
						{
							CookOpenOrder()=default;
							CookOpenOrder(const FString& InPath,int32 InOrder):uasset_relative_path(InPath),order(InOrder){}
							FString uasset_relative_path;
							int32 order;
						};
						auto MakeCookOpenOrder = [](const TArray<FAssetDetail>& Assets)->TArray<CookOpenOrder>
						{
							TArray<CookOpenOrder> result;
							TArray<FAssetData> AssetsData;
							TArray<FString> AssetPackagePaths;
							for (auto Asset : Assets)
							{
								FSoftObjectPath ObjectPath;
								ObjectPath.SetPath(Asset.mPackagePath);
								TArray<FAssetData> AssetData;
								UFLibAssetManageHelperEx::GetSpecifyAssetData(ObjectPath.GetLongPackageName(),AssetData,true);
								AssetsData.Append(AssetData);
							}

							// UFLibAssetManageHelperEx::GetAssetsData(AssetPackagePaths,AssetsData);
					
							for(int32 index=0;index<AssetsData.Num();++index)
							{
								UPackage* Package = AssetsData[index].GetPackage();
								FString LocalPath;
								const FString* PackageExtension = Package->ContainsMap() ? &FPackageName::GetMapPackageExtension() : &FPackageName::GetAssetPackageExtension();
								FPackageName::TryConvertLongPackageNameToFilename(AssetsData[index].PackageName.ToString(), LocalPath, *PackageExtension);
								result.Emplace(LocalPath,index);
							}
							return result;
						};
						TArray<CookOpenOrder> CookOpenOrders = MakeCookOpenOrder(ChunkAssets);

						auto SaveCookOpenOrder = [](const TArray<CookOpenOrder>& CookOpenOrders,const FString& File)
						{
							TArray<FString> result;
							for(const auto& OrderFile:CookOpenOrders)
							{
								result.Emplace(FString::Printf(TEXT("\"%s\" %d"),*OrderFile.uasset_relative_path,OrderFile.order));
							}
							FFileHelper::SaveStringArrayToFile(result,*FPaths::ConvertRelativePathToFull(File));
						};
						SaveCookOpenOrder(CookOpenOrders,FPaths::Combine(Context.GetSettingObject()->GetSaveAbsPath(),Context.NewVersionChunk.ChunkName,PlatformName,TEXT("CookOpenOrder.txt")));
					}
				}
			
				if(Context.GetSettingObject()->GetCookShaderOptions().bSharedShaderLibrary)
				{
					CookShaderCollection->Shutdown();
					if(CookShaderCollection->IsSuccessed())
					{
						FString SavePath = FPaths::Combine(Context.GetSettingObject()->GetSaveAbsPath(),Context.CurrentVersion.VersionId,PlatformName);
						TArray<FString> FoundShaderLibs = UFlibShaderCodeLibraryHelper::FindCookedShaderLibByPlatform(PlatformName,SavePath);

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
								FString FileName = FPaths::GetBaseFilename(FilePath,true);
								FString FileExtersion = FPaths::GetExtension(FilePath,false);
								FExternFileInfo AddShaderLib;
								AddShaderLib.Type = EPatchAssetType::NEW;
								AddShaderLib.FilePath.FilePath = FPaths::ConvertRelativePathToFull(FilePath);
								AddShaderLib.MountPath = FPaths::Combine(Context.GetSettingObject()->CookShaderOptions.ShderLibMountPoint,FString::Printf(TEXT("%s.%s"),*FileName,*FileExtersion));
								Context.GetPatcherDiffInfoByName(PlatformName)->AddExternalFiles.Add(AddShaderLib);
								Context.GetPatcherChunkInfoByName(PlatformName,Chunk.ChunkName)->AddExternFileToPak.Add(AddShaderLib);
							}
						}
					}
					else
					{
						UE_LOG(LogHotPatcher,Error,TEXT("This mission generate shader library faild!"));
					}
				}
			}
		}
		return true;
	};
	
	bool PatchAssetRegistryWorker(FHotPatcherPatchContext& Context)
	{
		if(Context.GetSettingObject()->GetSerializeAssetRegistryOptions().bSerializeAssetRegistry)
		{
			auto SerializeAssetRegistry = [](FHotPatcherPatchContext& Context,const FChunkInfo& Chunk,const FString& PlatformName)
			{
				ETargetPlatform Platform;
				UFlibPatchParserHelper::GetEnumValueByName(PlatformName,Platform);
				FChunkAssetDescribe ChunkAssetsDescrible = UFlibPatchParserHelper::CollectFChunkAssetsDescribeByChunk(
						Context.VersionDiff,
						Chunk,
						TArray<ETargetPlatform>{Platform},
						Context.GetSettingObject()->GetAssetsDependenciesScanedCaches()
					);
				TArray<FAssetDetail> ChunkAssets;
				UFLibAssetManageHelperEx::GetAssetDetailsByAssetDependenciesInfo(ChunkAssetsDescrible.Assets,ChunkAssets);
				
				if(Context.GetSettingObject()->GetSerializeAssetRegistryOptions().bSerializeAssetRegistry)
				{
					FString ChunkAssetRegistryName = Context.GetSettingObject()->GetSerializeAssetRegistryOptions().GetAssetRegistryNameRegular(Chunk.ChunkName);
					// // Save the generated registry
					FString AssetRegistryPath = FPaths::Combine(
						Context.GetSettingObject()->GetSaveAbsPath(),
						Context.NewVersionChunk.ChunkName,
						PlatformName,
						ChunkAssetRegistryName)
					;
					if(UFlibHotPatcherEditorHelper::SerializeAssetRegistryByDetails(PlatformName,ChunkAssets,AssetRegistryPath))
					{
						FExternFileInfo AssetRegistryFileInfo;
						AssetRegistryFileInfo.Type = EPatchAssetType::NEW;
						AssetRegistryFileInfo.FilePath.FilePath = AssetRegistryPath;
						AssetRegistryFileInfo.MountPath = FPaths::Combine(Context.GetSettingObject()->GetSerializeAssetRegistryOptions().AssetRegistryMountPoint,ChunkAssetRegistryName);
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
			};
		}
		return true;
	};
	bool GenerateGlobalAssetRegistryData(FHotPatcherPatchContext& Context)
	{
		FAssetDependenciesInfo TotalDiffAssets = UFLibAssetManageHelperEx::CombineAssetDependencies(Context.VersionDiff.AssetDiffInfo.AddAssetDependInfo,Context.VersionDiff.AssetDiffInfo.ModifyAssetDependInfo);
		TArray<FAssetDetail> AllAssets;
		UFLibAssetManageHelperEx::GetAssetDetailsByAssetDependenciesInfo(TotalDiffAssets,AllAssets);

		TSet<FName> PackageAssetsSet;
		for(const auto& Asset:AllAssets)
		{
			FString LongPackageName;
			if(UFLibAssetManageHelperEx::ConvPackagePathToLongPackageName(Asset.mPackagePath,LongPackageName))
			{
				PackageAssetsSet.Add(*LongPackageName);
			}
		}
		for(const auto& PlatformName:Context.GetSettingObject()->GetPakTargetPlatformNames())
		{
			ITargetPlatform* PlatformIns = UFlibHotPatcherEditorHelper::GetPlatformByName(PlatformName);
			
			UFlibHotPatcherEditorHelper::GeneratorGlobalAssetRegistryData(PlatformIns,PackageAssetsSet,TSet<FName>{},true);
		}
		return true;
	}

	void GenerateBinariesPatch(FHotPatcherPatchContext& Context,FChunkInfo& Chunk,ETargetPlatform Platform,TArray<FPakCommand>& PakCommands)
	{
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
		FString PlatformName = UFlibPatchParserHelper::GetEnumNameByValue(Platform);
		UHotPatcherSettings* Settings = GetMutableDefault<UHotPatcherSettings>();
		TimeRecorder BinariesPatchToralTR(FString::Printf(TEXT("Generate Binaries Patch of %s all chunks  Total Time:"),*PlatformName));
		FString OldCookedDir = Context.GetSettingObject()->GetBinariesPatchConfig().GetOldCookedDir();
		if(!FPaths::FileExists(OldCookedDir))
		{
			OldCookedDir = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(),Settings->TempPakDir,Context.BaseVersion.VersionId));
		}

		FString ExtractCryptoFile = Context.GetSettingObject()->GetBinariesPatchConfig().GetBasePakExtractCryptoJson();
		FString ExtractCryptoCmd = FPaths::FileExists(ExtractCryptoFile) ? FString::Printf(TEXT("-cryptokeys=\"%s\""),*ExtractCryptoFile) : TEXT("");
		FString ExtractDir = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(),Settings->TempPakDir,Context.BaseVersion.VersionId,PlatformName));
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
				auto RemoveDoubleQuoteLambda = [](const FString& InStr)->FString
				{
					FString resultStr = InStr;
					if(resultStr.StartsWith(TEXT("\"")))
					{
						resultStr.RemoveAt(0);
					}
					if(resultStr.EndsWith(TEXT("\"")))
					{
						resultStr.RemoveAt(resultStr.Len() - 1);
					}
					return resultStr;
				};

				auto ParseUassetLambda = [&RemoveDoubleQuoteLambda](const FString& InAsset)->FPakCommandItem
				{
					FPakCommandItem result;
					TArray<FString> AssetPakCmd = UKismetStringLibrary::ParseIntoArray(InAsset,TEXT("\" "));

					FString AssetAbsPath = AssetPakCmd[0];
					FString AssetMountPath = AssetPakCmd[1];
					result.AssetAbsPath = RemoveDoubleQuoteLambda(AssetAbsPath);
					result.AssetMountPath = RemoveDoubleQuoteLambda(AssetMountPath);
					return result;
				};

				FPakCommandItem PakAssetInfo = ParseUassetLambda(PakAssetPath);
				if(Context.GetSettingObject()->GetBinariesPatchConfig().IsMatchIgnoreRules(PakAssetInfo))
				{
					PatchedPakCommand.AddUnique(PakAssetPath);
					continue;
				}
				FString ProjectCookedDir = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Cooked")));
				FString OldAsset = PakAssetInfo.AssetAbsPath.Replace(
					*ProjectCookedDir,
					*OldCookedDir,ESearchCase::CaseSensitive);
				FString PatchSaveToPath = PakAssetInfo.AssetAbsPath.Replace(
					*ProjectCookedDir,
					*FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(),Settings->TempPakDir,TEXT("BinariesPatch")))
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
		TimeRecorder PakChunkToralTR(FString::Printf(TEXT("Generate all platform pakproxys of all chunks Total Time:")));
		TArray<ETargetPlatform> PakPlatforms = Context.GetSettingObject()->GetPakTargetPlatforms();
		for(const auto& Platform :PakPlatforms)
		{
			FString PlatformName = UFlibPatchParserHelper::GetEnumNameByValue(Platform);
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
				FString ChunkSaveBasePath = FPaths::Combine(Context.GetSettingObject()->GetSaveAbsPath(), Context.CurrentVersion.VersionId, PlatformName);
				
				TArray<FPakCommand> ChunkPakListCommands;
				{
					TimeRecorder CookAssetsTR(FString::Printf(TEXT("CollectPakCommandByChunk Platform:%s ChunkName:%s."),*PlatformName,*Chunk.ChunkName));
					ChunkPakListCommands= UFlibPatchParserHelper::CollectPakCommandByChunk(
						Context.VersionDiff,
						Chunk,
						PlatformName,
						// Context.GetSettingObject()->GetUnrealPakListOptions(),
						Context.GetSettingObject()->GetAssetsDependenciesScanedCaches(),
						Context.GetSettingObject()
					);
				}
				
				if(Context.PatchProxy)
					Context.PatchProxy->OnPakListGenerated.Broadcast(Context,Chunk,Platform,ChunkPakListCommands);
				
				auto AppendOptionsLambda = [](TArray<FString>& OriginCommands,const TArray<FString>& Options)
				{
					// [Pak]
					// +ExtensionsToNotUsePluginCompression=uplugin
					// +ExtensionsToNotUsePluginCompression=upluginmanifest
					// +ExtensionsToNotUsePluginCompression=uproject
					// +ExtensionsToNotUsePluginCompression=ini
					// +ExtensionsToNotUsePluginCompression=icu
					// +ExtensionsToNotUsePluginCompression=res
					TArray<FString> IgnoreCompressFormats;
					GConfig->GetArray(TEXT("Pak"),TEXT("ExtensionsToNotUsePluginCompression"),IgnoreCompressFormats,GEngineIni);

					auto GetPakCommandFileExtensionLambda = [](const FString& Command)->FString
					{
						FString result;
						int32 DotPos = Command.Find(TEXT("."), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
						if (DotPos != INDEX_NONE)
						{
							result =  Command.Mid(DotPos + 1);
							if(result.EndsWith(TEXT("\"")))
							{
								result.RemoveAt(result.Len()-1,1);
							}
						}
						return result;
					};
					
					for(auto& Command:OriginCommands)
					{
						FString PakOptionsStr;
						for (const auto& Param : Options)
						{
							FString FileExtension = GetPakCommandFileExtensionLambda(Command);
							if(IgnoreCompressFormats.Contains(FileExtension) && Param.Equals(TEXT("-compress"),ESearchCase::IgnoreCase))
							{
								continue;
							}
							PakOptionsStr += TEXT(" ") + Param;
						}
						Command = FString::Printf(TEXT("%s%s"),*Command,*PakOptionsStr);
					}
				};
				
				for(auto& PakCommand:ChunkPakListCommands)
				{
					AppendOptionsLambda(PakCommand.PakCommands,Context.GetSettingObject()->GetUnrealPakSettings().UnrealPakListOptions);
					AppendOptionsLambda(PakCommand.PakCommands,Context.GetSettingObject()->GetDefaultPakListOptions());
					AppendOptionsLambda(PakCommand.IoStoreCommands,Context.GetSettingObject()->GetIoStoreSettings().IoStorePakListOptions);
					AppendOptionsLambda(PakCommand.IoStoreCommands,Context.GetSettingObject()->GetDefaultPakListOptions());
				}
				
				if (!ChunkPakListCommands.Num())
				{
					FString Msg = FString::Printf(TEXT("Chunk:%s not contain any file!!!"), *Chunk.ChunkName);
					UE_LOG(LogHotPatcher, Error, TEXT("%s"),*Msg);
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
					
					const FString ChunkPakName = UFlibHotPatcherEditorHelper::MakePakShortName(Context.CurrentVersion,Chunk,PlatformName,Context.GetSettingObject()->GetPakNameRegular());
					SinglePakForChunk.ChunkStoreName = ChunkPakName;
					SinglePakForChunk.StorageDirectory = ChunkSaveBasePath;
					Chunk.PakFileProxys.Add(SinglePakForChunk);
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
						Chunk.PakFileProxys.Add(CurrentPak);
					}
				}
			}
		}
		return true;
	};

	// setup 9
	bool CreatePakWorker(FHotPatcherPatchContext& Context)
	{
		TimeRecorder CreateAllPakToralTR(FString::Printf(TEXT("Generate all platform pak of all chunks Total Time:")));
		// PakModeSingleLambda(PlatformName, CurrentVersionSavePath);
		for (const auto& Chunk : Context.PakChunks)
		{
			TimeRecorder PakChunkToralTR(FString::Printf(TEXT("Package ChunkName:%s Total Time:"),*Chunk.ChunkName));
				
			// // Update SlowTask Progress
			{
				FText Dialog = FText::Format(NSLOCTEXT("ExportPatch", "GeneratedPak", "Generating Pak list of Chunk {0}."), FText::FromString(Chunk.ChunkName));
				Context.OnPaking.Broadcast(TEXT("GeneratedPak"),*Dialog.ToString());
				Context.UnrealPakSlowTask->EnterProgressFrame(1.0, Dialog);
			}

			TArray<FString> UnrealPakCommandletGeneralOptions = Context.GetSettingObject()->GetUnrealPakSettings().UnrealCommandletOptions;
			UnrealPakCommandletGeneralOptions.Append(Context.GetSettingObject()->GetDefaultCommandletOptions());
			
			
			TArray<FReplaceText> ReplacePakListTexts = Context.GetSettingObject()->GetReplacePakListTexts();
			TArray<FThreadWorker> PakWorker;
				
			// TMap<FString,TArray<FPakFileInfo>>& PakFilesInfoMap = Context.PakFilesInfoMap;
				
			// 创建chunk的pak文件
			for (const auto& PakFileProxy : Chunk.PakFileProxys)
			{
				FString PlatformName = UFlibPatchParserHelper::GetEnumNameByValue(PakFileProxy.Platform);
				TArray<FString> UnrealPakCommandletOptions;
				UnrealPakCommandletOptions.Add(FString::Printf(TEXT("-AlignForMemoryMapping=%d"),UFlibHotPatcherEditorHelper::GetPlatformByName(PlatformName)->GetMemoryMappingAlignment()));
				// can override
				UnrealPakCommandletOptions.Append(UnrealPakCommandletGeneralOptions);
				UnrealPakCommandletOptions.Add(UFlibHotPatcherEditorHelper::GetEncryptSettingsCommandlineOptions(Context.GetSettingObject()->GetEncryptSettings(),UFlibHotPatcherEditorHelper::Conv2IniPlatform(PlatformName)));
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
									FString Msg = FString::Printf(TEXT("Successed to Package the patch as %s."),*PakSavePath);
									Context.OnPaking.Broadcast(TEXT("SavedPakFile"),Msg);
								}else
								{
									FText Msg = LOCTEXT("SavedPakFileMsg", "Successed to Package the patch as Pak.");
									UFlibHotPatcherEditorHelper::CreateSaveFileNotify(Msg, PakSavePath);
								}
								FPakFileInfo CurrentPakInfo;
								if (UFlibPatchParserHelper::GetPakFileInfo(PakSavePath, CurrentPakInfo))
								{
									// CurrentPakInfo.PakVersion = CurrentPakVersion;
									if (!Context.PakFilesInfoMap.Contains(PlatformName))
									{
										Context.PakFilesInfoMap.Add(PlatformName, TArray<FPakFileInfo>{CurrentPakInfo});
									}
									else
									{
										Context.PakFilesInfoMap.Find(PlatformName)->Add(CurrentPakInfo);
									}
								}
							}
						}//);
							
						if (!Chunk.bStorageUnrealPakList)
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
				FText Dialog = FText::Format(NSLOCTEXT("ExportPatch", "GeneratedPak", "Generating Io Store list of Chunk {0}."), FText::FromString(Chunk.ChunkName));
				Context.OnPaking.Broadcast(TEXT("GeneratedIoStore"),*Dialog.ToString());
				Context.UnrealPakSlowTask->EnterProgressFrame(1.0, Dialog);
			}

			TArray<FReplaceText> ReplacePakListTexts = Context.GetSettingObject()->GetReplacePakListTexts();
			if(!Chunk.PakFileProxys.Num())
			{
				UE_LOG(LogHotPatcher,Error,TEXT("Chunk %s Not Contain Any valid PakFileProxy!!!"),*Chunk.ChunkName);
				continue;
			}
			// 创建chunk的Io Store文件
			TArray<FString> IoStoreCommands;
			for (const auto& PakFileProxy : Chunk.PakFileProxys)
			{
				if(!IoStoreSettings.PlatformContainers.Contains(PakFileProxy.Platform))
					return true;
				TArray<FString> FinalIoStoreCommandletOptions;
				FString  PlatformName = UFlibPatchParserHelper::GetEnumNameByValue(PakFileProxy.Platform);
				FinalIoStoreCommandletOptions.Add(FString::Printf(TEXT("-AlignForMemoryMapping=%d"),UFlibHotPatcherEditorHelper::GetPlatformByName(PlatformName)->GetMemoryMappingAlignment()));
				FinalIoStoreCommandletOptions.Append(AdditionalIoStoreCommandletOptions);
				FString IoStoreCommandletOptions;
				for(const auto& Option:FinalIoStoreCommandletOptions) { IoStoreCommandletOptions+=FString::Printf(TEXT("%s "),*Option); }
				IoStoreCommandletOptions += UFlibHotPatcherEditorHelper::GetEncryptSettingsCommandlineOptions(Context.GetSettingObject()->GetEncryptSettings(),UFlibHotPatcherEditorHelper::Conv2IniPlatform(PlatformName));
					
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
				FString IoStoreCommandsFile = FPaths::Combine(Chunk.PakFileProxys[0].StorageDirectory,FString::Printf(TEXT("%s_IoStoreCommands.txt"),*Chunk.ChunkName));
					
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
					
				FString PlatformCookDir = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(),TEXT("Saved/Cooked/"),PlatformName));

				if(FPaths::FileExists(PlatformGlocalContainers))
				{
					FString CookOrderFile = FPaths::Combine(Context.GetSettingObject()->GetSaveAbsPath(),Context.NewVersionChunk.ChunkName,PlatformName,TEXT("CookOpenOrder.txt"));
					FFileHelper::SaveStringArrayToFile(IoStoreCommands,*IoStoreCommandsFile);
					FString IoStoreCommandlet = FString::Printf(
						TEXT("-CreateGlobalContainer=\"%s\" -CookedDirectory=\"%s\" -Commands=\"%s\" -CookerOrder=\"%s\" -TargetPlatform=\"%s\" %s %s"),
						// *UFlibHotPatcherEditorHelper::GetUECmdBinary(),
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
						*UFlibHotPatcherEditorHelper::GetUECmdBinary(),
						*UFlibPatchParserHelper::GetProjectFilePath(),
						*IoStoreCommandlet
					);
						
					// FCommandLine::Set(*IoStoreCommandlet);
					UE_LOG(LogHotPatcher,Log,TEXT("%s"),*IoStoreNativeCommandlet);
					// CreateIoStoreContainerFiles(*IoStoreCommandlet);

					TSharedPtr<FProcWorkerThread> IoStoreProc = MakeShareable(
						new FProcWorkerThread(
							TEXT("PakIoStoreThread"),
							UFlibHotPatcherEditorHelper::GetUECmdBinary(),
							FString::Printf(TEXT("\"%s\" -run=IoStore %s"), *UFlibPatchParserHelper::GetProjectFilePath(),*IoStoreCommandlet)
						)
					);
					IoStoreProc->ProcOutputMsgDelegate.AddStatic(&::ReceiveOutputMsg);
					IoStoreProc->Execute();
					IoStoreProc->Join();
				}
			}
		}
#endif
		return true;
	};

	// delete pakversion.json
	//{
	//	FString PakVersionSavedPath = FExportPatchSettingsEx::GetSavePakVersionPath(CurrentVersionSavePath,CurrentVersion);
	//	if (GetSettingObject()->IsIncludePakVersion() && FPaths::FileExists(PakVersionSavedPath))
	//	{
	//		IFileManager::Get().Delete(*PakVersionSavedPath);
	//	}
	//}

	// setup 10
	// save asset dependency
	bool SaveAssetDependenciesWorker(FHotPatcherPatchContext& Context)
	{
		TimeRecorder CookAssetsTR(FString::Printf(TEXT("Save asset dependencies")));
		if (Context.GetSettingObject()->IsSaveAssetRelatedInfo() && Context.GetPakFileNum())
		{
			TArray<EAssetRegistryDependencyTypeEx> AssetDependencyTypes;
			AssetDependencyTypes.Add(EAssetRegistryDependencyTypeEx::Packages);

			TArray<FHotPatcherAssetDependency> AssetsDependency = UFlibPatchParserHelper::GetAssetsRelatedInfoByFAssetDependencies(
				UFLibAssetManageHelperEx::CombineAssetDependencies(Context.VersionDiff.AssetDiffInfo.AddAssetDependInfo, Context.VersionDiff.AssetDiffInfo.ModifyAssetDependInfo),
				AssetDependencyTypes,
				Context.GetSettingObject()->GetAssetsDependenciesScanedCaches()
			);

			FString AssetsDependencyString = UFlibPatchParserHelper::SerializeAssetsDependencyAsJsonString(AssetsDependency);
			
			FString SaveAssetRelatedInfoToFile = FPaths::Combine(
				Context.GetSettingObject()->GetCurrentVersionSavePath(),
				FString::Printf(TEXT("%s_AssetRelatedInfos.json"), *Context.CurrentVersion.VersionId)
			);
			if (UFLibAssetManageHelperEx::SaveStringToFile(SaveAssetRelatedInfoToFile, AssetsDependencyString))
			{
				if(::IsRunningCommandlet())
				{
					FString Msg = FString::Printf(TEXT("Succeed to export Asset Related infos:%s."),*SaveAssetRelatedInfoToFile);
					Context.OnPaking.Broadcast(TEXT("SaveAssetRelatedInfo"),Msg);
				}
				else
				{
					auto Msg = LOCTEXT("SaveAssetRelatedInfo", "Succeed to export Asset Related infos.");
					UFlibHotPatcherEditorHelper::CreateSaveFileNotify(Msg, SaveAssetRelatedInfoToFile);
				}
			}
		}
		return true;
	};

	// setup 11 save difference to file
	bool SaveDifferenceWorker(FHotPatcherPatchContext& Context)
	{
		if(Context.GetPakFileNum())
		{
			TimeRecorder SaveDiffTR(FString::Printf(TEXT("Save Patch Diff info")));
			FText DiaLogMsg = FText::Format(NSLOCTEXT("ExportPatch", "ExportPatchDiffFile", "Generating Diff info of version {0}"), FText::FromString(Context.CurrentVersion.VersionId));

			auto SavePatchDiffJsonLambda = [&Context](const FHotPatcherVersion& InSaveVersion, const FPatchVersionDiff& InDiff)->bool
			{
				bool bStatus = false;
				if (Context.GetSettingObject()->IsSaveDiffAnalysis())
				{
					auto SerializeChangedAssetInfo = [](const FPatchVersionDiff& InAssetInfo)->FString
					{
						FString AddAssets;
						UFlibPatchParserHelper::TSerializeStructAsJsonString(InAssetInfo,AddAssets);
						return AddAssets;
					};
		
					FString SerializeDiffInfo = SerializeChangedAssetInfo(InDiff);
		
					// FString::Printf(TEXT("%s"),*SerializeDiffInfo);

					FString SaveDiffToFile = FPaths::Combine(
						Context.GetSettingObject()->GetCurrentVersionSavePath(),
						FString::Printf(TEXT("%s_%s_Diff.json"), *InSaveVersion.BaseVersionId, *InSaveVersion.VersionId)
					);
					if (UFLibAssetManageHelperEx::SaveStringToFile(SaveDiffToFile, SerializeDiffInfo))
					{
						bStatus = true;

						FString Msg = FString::Printf(TEXT("Succeed to export New Patch Diff Info."),*SaveDiffToFile);
						Context.OnPaking.Broadcast(TEXT("SavePatchDiffInfo"),Msg);
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
			Context.NewReleaseVersion = UFlibHotPatcherEditorHelper::MakeNewReleaseByDiff(Context.BaseVersion, Context.VersionDiff,Context.GetSettingObject());
			UFlibPatchParserHelper::TSerializeStructAsJsonString(Context.NewReleaseVersion, SerializeReleaseVersionInfo);

			FString SaveCurrentVersionToFile = FPaths::Combine(
				Context.GetSettingObject()->GetCurrentVersionSavePath(),
				FString::Printf(TEXT("%s_Release.json"), *Context.CurrentVersion.VersionId)
			);
			if (UFLibAssetManageHelperEx::SaveStringToFile(SaveCurrentVersionToFile, SerializeReleaseVersionInfo))
			{
				if(::IsRunningCommandlet())
				{
					FString Msg = FString::Printf(TEXT("Succeed to export New Release Info to %s."),*SaveCurrentVersionToFile);
					Context.OnPaking.Broadcast(TEXT("SavePatchDiffInfo"),Msg);
				}else
				{
					auto Msg = LOCTEXT("SavePatchDiffInfo", "Succeed to export New Release Info.");
					UFlibHotPatcherEditorHelper::CreateSaveFileNotify(Msg, SaveCurrentVersionToFile);
				}
			}
		}
		return true;
	};

	// setup 13 serialize all pak file info
	bool SavePakFileInfoWorker(FHotPatcherPatchContext& Context)
	{
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
			UFlibPatchParserHelper::SerializePlatformPakInfoToString(Context.PakFilesInfoMap, PakFilesInfoStr);

			if (!PakFilesInfoStr.IsEmpty())
			{
				FString SavePakFilesPath = FPaths::Combine(
					Context.GetSettingObject()->GetCurrentVersionSavePath(),
					FString::Printf(TEXT("%s_PakFilesInfo.json"), *Context.CurrentVersion.VersionId)
				);
				if (UFLibAssetManageHelperEx::SaveStringToFile(SavePakFilesPath, PakFilesInfoStr) && FPaths::FileExists(SavePakFilesPath))
				{
					if(::IsRunningCommandlet())
					{
						FString Msg = FString::Printf(TEXT("Successed to Export the Pak File info to ."),*SavePakFilesPath);
						Context.OnPaking.Broadcast(TEXT("SavedPakFileMsg"),Msg);
					}else
					{
						FText Msg = LOCTEXT("SavedPakFileMsg", "Successed to Export the Pak File info.");
						UFlibHotPatcherEditorHelper::CreateSaveFileNotify(Msg, SavePakFilesPath);
					}
				}
			}
		}
		return true;
	};
	
	// setup 14 serialize patch config
	bool SavePatchConfigWorker(FHotPatcherPatchContext& Context)
	{
		if(Context.GetPakFileNum())
		{
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

			FString SaveConfigPath = FPaths::Combine(
				Context.GetSettingObject()->GetCurrentVersionSavePath(),
				FString::Printf(TEXT("%s_PatchConfig.json"),*Context.CurrentVersion.VersionId)
			);

			if (Context.GetSettingObject()->IsSaveConfig())
			{
				FString SerializedJsonStr;
				UFlibPatchParserHelper::TSerializeStructAsJsonString(*Context.GetSettingObject(),SerializedJsonStr);
				if (FFileHelper::SaveStringToFile(SerializedJsonStr, *SaveConfigPath))
				{
					if(::IsRunningCommandlet())
					{
						FString Msg = FString::Printf(TEXT("Successed to Export the Patch Config to %s."),*SaveConfigPath);
						Context.OnPaking.Broadcast(TEXT("SavedPatchConfig"),Msg);
					}else
					{
						FText Msg = LOCTEXT("SavedPatchConfig", "Successed to Export the Patch Config.");
						UFlibHotPatcherEditorHelper::CreateSaveFileNotify(Msg, SaveConfigPath);
					}
				}
			}
		}
		return true;
	};
	
	// setup 15
	bool BackupMetadataWorker(FHotPatcherPatchContext& Context)
	{
		// backup Metadata
		if(Context.GetPakFileNum())
		{
			TimeRecorder TR(FString::Printf(TEXT("Backup Metadata")));
			FText DiaLogMsg = FText::Format(NSLOCTEXT("BackupMetadata", "BackupMetadata", "Backup Release {0} Metadatas."), FText::FromString(Context.GetSettingObject()->GetVersionId()));
			Context.UnrealPakSlowTask->EnterProgressFrame(1.0, DiaLogMsg);
			if(Context.GetSettingObject()->IsBackupMetadata())
			{
				UFlibHotPatcherEditorHelper::BackupMetadataDir(
					FPaths::ProjectDir(),
					FApp::GetProjectName(),
					Context.GetSettingObject()->GetPakTargetPlatforms(),
					FPaths::Combine(Context.GetSettingObject()->GetSaveAbsPath(),
					Context.GetSettingObject()->GetVersionId())
				);
			}
		}
		return true;
	};

	// setup 16
	bool ShowSummaryWorker(FHotPatcherPatchContext& Context)
	{
		// show summary infomation
		if(Context.GetPakFileNum())
		{
			Context.OnShowMsg.Broadcast(UFlibHotPatcherEditorHelper::PatchSummary(Context.VersionDiff));
			Context.OnShowMsg.Broadcast(UFlibHotPatcherEditorHelper::ReleaseSummary(Context.NewReleaseVersion));
		}
		return true;
	};

	// setup 17
	bool OnFaildDispatchWorker(FHotPatcherPatchContext& Context)
	{
		if (!Context.GetPakFileNum())
		{
			UE_LOG(LogHotPatcher, Error, TEXT("The Patch not contain any invalie file!"));
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
