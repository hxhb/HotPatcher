#include "BaseTypes/FExternFileInfo.h"

#include "FlibPatchParserHelper.h"

FString FExternFileInfo::GenerateFileHash(EHashCalculator HashCalculator)
{
	FileHash = GetFileHash(HashCalculator);
	return FileHash;
}

FString FExternFileInfo::GetFileHash(EHashCalculator HashCalculator)const
{
	return UFlibPatchParserHelper::FileHash(GetReplaceMarkdFilePath(),HashCalculator);
}

FString FExternFileInfo::GetReplaceMarkdFilePath() const
{
	return UFlibPatchParserHelper::ReplaceMark(FilePath.FilePath);
}


void FExternFileInfo::SetFilePath(const FString& InFilePath)
{
	FilePath.FilePath = UFlibPatchParserHelper::MakeMark(InFilePath);
}