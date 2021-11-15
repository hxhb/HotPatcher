#include "CreatePatch/HotPatcherContext.h"



FPatchVersionExternDiff* FHotPatcherPatchContext::GetPatcherDiffInfoByName(const FString& PlatformName)
{
	ETargetPlatform Platform;
	UFlibPatchParserHelper::GetEnumValueByName(PlatformName,Platform);
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
	ETargetPlatform Platform;
	UFlibPatchParserHelper::GetEnumValueByName(PlatformName,Platform);
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
 