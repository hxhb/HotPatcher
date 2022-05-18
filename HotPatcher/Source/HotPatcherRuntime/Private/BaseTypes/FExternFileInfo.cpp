#include "BaseTypes/FExternFileInfo.h"

#include "FlibPatchParserHelper.h"

FString FExternFileInfo::GenerateFileHash(EHashCalculator HashCalculator)
{
	FileHash = GetFileHash(HashCalculator);
	return FileHash;
}

FString FExternFileInfo::GetFileHash(EHashCalculator HashCalculator)const
{
	return UFlibPatchParserHelper::FileHash(FilePath.FilePath,HashCalculator);
}