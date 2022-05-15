#include "BaseTypes/FExternFileInfo.h"

FString FExternFileInfo::GenerateFileHash(EHashCalculator HashCalculator)
{
	FileHash = GetFileHash(HashCalculator);
	return FileHash;
}

FString FExternFileInfo::GetFileHash(EHashCalculator HashCalculator)const
{
	FString HashValue;
	FString FileAbsPath = FPaths::ConvertRelativePathToFull(FilePath.FilePath);
	if (FPaths::FileExists(FileAbsPath))
	{
		switch (HashCalculator)
		{
		case EHashCalculator::MD5:
			{
				FMD5Hash FileMD5Hash = FMD5Hash::HashFile(*FileAbsPath);
				HashValue = LexToString(FileMD5Hash);
				break;
			}
		case  EHashCalculator::SHA1:
			{
				FSHAHash Hash;
				FSHA1::GetFileSHAHash(*FileAbsPath,Hash.Hash,true);
				TArray<uint8> Data;
				FFileHelper::LoadFileToArray(Data,*FileAbsPath);
				FSHA1::HashBuffer(Data.GetData(),Data.Num(),Hash.Hash);
				HashValue = Hash.ToString();
				break;
			}
		};
	}
	return HashValue;
}