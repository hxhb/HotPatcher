#include "BaseTypes/FChunkInfo.h"

#include "FlibPatchParserHelper.h"
#include "Engine/AssetManager.h"


FString FChunkInfo::GetShaderLibraryName() const
{
	FString ShaderLibraryName;
	switch (GetCookShaderOptions().ShaderNameRule)
	{
	case EShaderLibNameRule::CHUNK_NAME:
		{
			ShaderLibraryName = ChunkName;
			break;
		}
	case EShaderLibNameRule::PROJECT_NAME:
		{
			ShaderLibraryName = FApp::GetProjectName();
			break;
		}
	case EShaderLibNameRule::CUSTOM:
		{
			ShaderLibraryName = GetCookShaderOptions().CustomShaderName;
			break;
		}
	}
	return ShaderLibraryName;
}


TArray<FSoftObjectPath> FChunkInfo::GetManagedAssets() const
{
	FScopedNamedEventStatic GetManagedAssetsTag(FColor::Red,*FString::Printf(TEXT("GetManagedAssets_%s"),*ChunkName));
	TArray<FSoftObjectPath> NewPaths;
	
	UAssetManager& Manager = UAssetManager::Get();
	IAssetRegistry& AssetRegistry = Manager.GetAssetRegistry();
	
	FHotPatcherVersion ChunkManageAssetsVersion = UFlibPatchParserHelper::ExportReleaseVersionInfoByChunk(
		TEXT(""),
		TEXT(""),
		TEXT(""),
		*this,
		false,
		bAnalysisFilterDependencies
	);
	const TArray<FAssetDetail>& LabelManagedAssets = ChunkManageAssetsVersion.AssetInfo.GetAssetDetails();
	if (LabelManagedAssets.Num())
	{
		for (const auto& AssetDetail : LabelManagedAssets)
		{
			FAssetData AssetData;
			bool bAssetDataStatus = UFlibAssetManageHelper::GetAssetsDataByPackageName(AssetDetail.PackagePath.ToString(),AssetData);
			if(bAssetDataStatus && AssetData.IsValid())
			{
				FSoftObjectPath AssetRef = Manager.GetAssetPathForData(AssetData);
				if (!AssetRef.IsNull())
				{
					NewPaths.Add(AssetRef);
				}
			}
		}
	}
	return NewPaths;
}



bool FChunkAssetDescribe::HasValidAssets()const
{
	bool bHasValidExFiles = false;
	for(const auto& Item:AllPlatformExFiles)
	{
		if(!!Item.Value.ExternFiles.Num())
		{
			bHasValidExFiles = true;
			break;
		}
	}
		
	return !!GetAssetsDetail().Num() || InternalFiles.HasValidAssets() || bHasValidExFiles;
}
	
TArray<FAssetDetail> FChunkAssetDescribe::GetAssetsDetail()const
{
	return Assets.GetAssetDetails();
}
	
	
TArray<FName> FChunkAssetDescribe::GetAssetsStrings()const
{
	TArray<FName> AllUnselectedAssets;
	const TArray<FAssetDetail>& OutAssetDetails = GetAssetsDetail();

	for (const auto& AssetDetail : OutAssetDetails)
	{
		AllUnselectedAssets.AddUnique(AssetDetail.PackagePath);
	}
	return AllUnselectedAssets;
}

TArray<FExternFileInfo> FChunkAssetDescribe::GetExFilesByPlatform(ETargetPlatform Platform)const
{
	TArray<FExternFileInfo> result;
	if(AllPlatformExFiles.Contains(Platform))
	{
		result = AllPlatformExFiles.Find(Platform)->ExternFiles;
	}
	return result;
}
TArray<FName> FChunkAssetDescribe::GetExternalFileNames(ETargetPlatform Platform)const
{
	TArray<FName> ExFilesResult;
	auto CollectExFilesStrings = [](const TArray<FExternFileInfo>& InFiles)->TArray<FName>
	{
		TArray<FName> result;
		for (const auto& File : InFiles)
		{
			result.AddUnique(FName(*File.GetFilePath()));
		}
		return result;
	};
	if(AllPlatformExFiles.Contains(Platform))
	{
		ExFilesResult = CollectExFilesStrings(GetExFilesByPlatform(Platform));
	}
	return ExFilesResult;
}


TArray<FName> FChunkAssetDescribe::GetInternalFileNames()const
{
	TArray<FName> result;
	{
		if (InternalFiles.bIncludeAssetRegistry) { result.Add(TEXT("bIncludeAssetRegistry")); };
		if (InternalFiles.bIncludeGlobalShaderCache) { result.Add(TEXT("bIncludeGlobalShaderCache")); };
		if (InternalFiles.bIncludeShaderBytecode) { result.Add(TEXT("bIncludeShaderBytecode")); };
		if (InternalFiles.bIncludeEngineIni) { result.Add(TEXT("bIncludeEngineIni")); };
		if (InternalFiles.bIncludePluginIni) { result.Add(TEXT("bIncludePluginIni")); };
		if (InternalFiles.bIncludeProjectIni) { result.Add(TEXT("bIncludeProjectIni")); };
	}
	return result;
}

FChunkInfo FChunkAssetDescribe::AsChunkInfo(const FString& ChunkName)
{
	FChunkInfo DefaultChunk;

	DefaultChunk.ChunkName = ChunkName;
	DefaultChunk.bMonolithic = false;
	DefaultChunk.InternalFiles = GetInternalInfo();
	DefaultChunk.bOutputDebugInfo = false;
	DefaultChunk.bStorageUnrealPakList = true;
	DefaultChunk.bStorageIoStorePakList = true;
	for(const auto& AssetDetail:GetAssetsDetail())
	{
		FPatcherSpecifyAsset Asset;
		Asset.Asset.SetPath(AssetDetail.PackagePath.ToString());
		DefaultChunk.IncludeSpecifyAssets.AddUnique(Asset);
	}
	for(const auto& ExFiles:AllPlatformExFiles)
	{
		FPlatformExternAssets PlatformFiles;
		PlatformFiles.TargetPlatform = ExFiles.Key;
		PlatformFiles.AddExternFileToPak = ExFiles.Value.ExternFiles;
	}
	return DefaultChunk;
}