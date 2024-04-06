#include "CreatePatch/HotPatcherContext.h"

void FPackageTrackerByDiff::OnPackageCreated(UPackage* Package)
{
	FString PackageName = Package->GetName();
		
	// get asset detail by current version
	FAssetDetail CurrentVersionAssetDetail = UFlibAssetManageHelper::GetAssetDetailByPackageName(PackageName);

	bool bNeedAdd = true;

	auto HasAssstInDependenciesInfo = [&CurrentVersionAssetDetail](const FAssetDependenciesInfo& DependenciesInfo,const FString& PackageName)->bool
	{
		bool bHasInDependenciesInfo = false;
		if(DependenciesInfo.HasAsset(PackageName))
		{
			FAssetDetail BaseVersionAssetDetail;
			// get asset detail by base version
			if(DependenciesInfo.GetAssetDetailByPackageName(PackageName,BaseVersionAssetDetail))
			{
				if(BaseVersionAssetDetail == CurrentVersionAssetDetail)
				{
					bHasInDependenciesInfo = true;
				}
			}
		}
		return bHasInDependenciesInfo;
	};
	if(HasAssstInDependenciesInfo(Context.BaseVersion.AssetInfo,PackageName) ||
		HasAssstInDependenciesInfo(Context.VersionDiff.AssetDiffInfo.AddAssetDependInfo,PackageName) ||
		HasAssstInDependenciesInfo(Context.VersionDiff.AssetDiffInfo.ModifyAssetDependInfo,PackageName)
	)
	{
		bNeedAdd = false;
	}
	
	if(bNeedAdd)
	{
		TrackedAssets.Add(FName(*PackageName),CurrentVersionAssetDetail);
	}
}

void FHotPatcherPatchContext::Init()
{
	FHotPatcherContext::Init();
}

FPatchVersionExternDiff* FHotPatcherPatchContext::GetPatcherDiffInfoByName(const FString& PlatformName)
{
	ETargetPlatform Platform;
	THotPatcherTemplateHelper::GetEnumValueByName(PlatformName,Platform);
	// add new file to diff
	FPatchVersionExternDiff* PatchVersionExternDiff = NULL;
	{
		if(!VersionDiff.PlatformExternDiffInfo.Contains(Platform))
		{
			FPatchVersionExternDiff AdditionalFiles;
			VersionDiff.PlatformExternDiffInfo.Add(Platform,AdditionalFiles);
		}
		PatchVersionExternDiff = VersionDiff.PlatformExternDiffInfo.Find(Platform);
	}
	return PatchVersionExternDiff;
};
FPlatformExternAssets* FHotPatcherPatchContext::GetPatcherChunkInfoByName(const FString& PlatformName,const FString& ChunkName)
{
	SCOPED_NAMED_EVENT_TEXT("FHotPatcherPatchContext::GetPatcherChunkInfoByName",FColor::Red);
	ETargetPlatform Platform;
	THotPatcherTemplateHelper::GetEnumValueByName(PlatformName,Platform);
	FPlatformExternAssets* PlatformExternAssetsPtr = NULL;
	{
		auto PlatformExternAssetsPtrByChunk = [](FChunkInfo& ChunkInfo,ETargetPlatform Platform)
		{
			FPlatformExternAssets* PlatformExternAssetsPtr = NULL;
			for(auto& PlatfromExternAssetsRef:ChunkInfo.AddExternAssetsToPlatform)
			{
				if(PlatfromExternAssetsRef.TargetPlatform == Platform)
				{
					PlatformExternAssetsPtr = &PlatfromExternAssetsRef;
				}
			}
			if(!PlatformExternAssetsPtr)
			{
				FPlatformExternAssets PlatformExternAssets;
				PlatformExternAssets.TargetPlatform = Platform;
				int32 index = ChunkInfo.AddExternAssetsToPlatform.Add(PlatformExternAssets);
				PlatformExternAssetsPtr = &ChunkInfo.AddExternAssetsToPlatform[index];
			}
			return PlatformExternAssetsPtr;
		};
		
		for(auto& PakChunk:PakChunks)
		{
			if(PakChunk.ChunkName.Equals(ChunkName))
			{
				PlatformExternAssetsPtr = PlatformExternAssetsPtrByChunk(PakChunk,Platform);
			}
		}
		if(!PlatformExternAssetsPtr)
		{
			PlatformExternAssetsPtr = PlatformExternAssetsPtrByChunk(PakChunks[0],Platform);
		}
		return PlatformExternAssetsPtr;
	}
}

bool FHotPatcherPatchContext::IsContainInBase(const FString& PlatformName, const FExternFileInfo& AddFile)
{
	SCOPED_NAMED_EVENT_TEXT("IsContainInBase",FColor::Red);
	bool bContain = false;
	ETargetPlatform Platform;
	THotPatcherTemplateHelper::GetEnumValueByName(PlatformName,Platform);
	if(!BaseVersion.PlatformAssets.Contains(Platform))
	{
		return false;
	}
	const FPlatformExternFiles& BaseVersionExternFiles = UFlibPatchParserHelper::GetAllExFilesByPlatform(BaseVersion.PlatformAssets[Platform],false,GetSettingObject()->GetHashCalculator());
	for(const auto& BaseFile:BaseVersionExternFiles.ExternFiles)
	{
		if(BaseFile.IsSameMount(AddFile))
		{
			bContain = true;
			break;
		}
	}
	return bContain;
}

bool FHotPatcherPatchContext::AddAsset(const FString ChunkName, const FAssetDetail& AssetDetail)
{
	SCOPED_NAMED_EVENT_TEXT("FHotPatcherPatchContext::AddAsset",FColor::Red);
	bool bRet = false;

	if(!AssetDetail.IsValid())
		return false;
	
	FString PackageName = UFlibAssetManageHelper::PackagePathToLongPackageName(AssetDetail.PackagePath.ToString());

	bool bInBaseVersion = false;
	// has base version
	if(GetSettingObject()->bByBaseVersion)
	{
		bInBaseVersion = BaseVersion.AssetInfo.HasAsset(PackageName);
	}
	FAssetDependenciesInfo& AddtoDependencies = bInBaseVersion ? VersionDiff.AssetDiffInfo.ModifyAssetDependInfo : VersionDiff.AssetDiffInfo.AddAssetDependInfo;
	
	if(!AddtoDependencies.HasAsset(PackageName))
	{
		UE_LOG(LogHotPatcher,Display,TEXT("Add %s to Chunk %s"),*AssetDetail.PackagePath.ToString(),*ChunkName);
		AddtoDependencies.AddAssetsDetail(AssetDetail);
		for(auto& ChunkInfo:PakChunks)
		{
			if(ChunkInfo.ChunkName.Equals(ChunkName))
			{
				FPatcherSpecifyAsset CurrentAsset;
				CurrentAsset.Asset = AssetDetail.PackagePath.ToString();
				CurrentAsset.bAnalysisAssetDependencies = false;
				CurrentAsset.AssetRegistryDependencyTypes = {EAssetRegistryDependencyTypeEx::None};
				ChunkInfo.IncludeSpecifyAssets.AddUnique(CurrentAsset);
				bRet = true;
			}
		}
	}
	return bRet;
}
 