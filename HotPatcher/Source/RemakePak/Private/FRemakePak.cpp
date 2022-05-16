#include "FRemakePak.h"

/* Helper for index creation in CreatePakFile.  Used to (conditionally) write Bytes into a SecondaryIndex and serialize into the PrimaryIndex the offset of the SecondaryIndex */
struct FSecondaryIndexWriter
{
private:
	TArray<uint8>& SecondaryIndexData;
	FMemoryWriter SecondaryWriter;
	TArray<uint8>& PrimaryIndexData;
	FMemoryWriter& PrimaryIndexWriter;
	FSHAHash SecondaryHash;
	int64 OffsetToDataInPrimaryIndex = INDEX_NONE;
	int64 OffsetToSecondaryInPakFile = INDEX_NONE;
	int64 SecondarySize = 0;
	bool bShouldWrite;

public:
	FSecondaryIndexWriter(TArray<uint8>& InSecondaryIndexData, bool bInShouldWrite, TArray<uint8>& InPrimaryIndexData, FMemoryWriter& InPrimaryIndexWriter)
		: SecondaryIndexData(InSecondaryIndexData)
		, SecondaryWriter(SecondaryIndexData)
		, PrimaryIndexData(InPrimaryIndexData)
		, PrimaryIndexWriter(InPrimaryIndexWriter)
		, bShouldWrite(bInShouldWrite)
	{
		if (bShouldWrite)
		{
			SecondaryWriter.SetByteSwapping(PrimaryIndexWriter.ForceByteSwapping());
		}
	}

	/**
	 * Write the condition flag and the Offset,Size,Hash into the primary index.  Offset of the Secondary index cannot be calculated until the PrimaryIndex is done writing later,
	 * so placeholders are left instead, with a marker to rewrite them later.
	 */
	void WritePlaceholderToPrimary()
	{
		PrimaryIndexWriter << bShouldWrite;
		if (bShouldWrite)
		{
			OffsetToDataInPrimaryIndex = PrimaryIndexData.Num();
			PrimaryIndexWriter << OffsetToSecondaryInPakFile;
			PrimaryIndexWriter << SecondarySize;
			PrimaryIndexWriter << SecondaryHash;
		}
	}

	FMemoryWriter& GetSecondaryWriter()
	{
		return SecondaryWriter;
	}

	/** The caller has populated this->SecondaryIndexData using GetSecondaryWriter().  We now calculate the size, encrypt, and hash, and store the Offset,Size,Hash in the PrimaryIndex */
	void FinalizeAndRecordOffset(int64 OffsetInPakFile, const TFunction<void(TArray<uint8> & IndexData, FSHAHash & OutHash)>& FinalizeIndexBlock)
	{
		if (!bShouldWrite)
		{
			return;
		}

		FinalizeIndexBlock(SecondaryIndexData, SecondaryHash);
		SecondarySize = SecondaryIndexData.Num();
		OffsetToSecondaryInPakFile = OffsetInPakFile;

		PrimaryIndexWriter.Seek(OffsetToDataInPrimaryIndex);
		PrimaryIndexWriter << OffsetToSecondaryInPakFile;
		PrimaryIndexWriter << SecondarySize;
		PrimaryIndexWriter << SecondaryHash;
	}
};

FString GetLongestPath(const TArray<FString>& FileMountPaths)
{
	FString LongestPath;
	int32 MaxNumDirectories = 0;

	for (int32 FileIndex = 0; FileIndex < FileMountPaths.Num(); FileIndex++)
	{
		const FString& Filename = FileMountPaths[FileIndex];
		int32 NumDirectories = 0;
		for (int32 Index = 0; Index < Filename.Len(); Index++)
		{
			if (Filename[Index] == '/')
			{
				NumDirectories++;
			}
		}
		if (NumDirectories > MaxNumDirectories)
		{
			LongestPath = Filename;
			MaxNumDirectories = NumDirectories;
		}
	}
	return FPaths::GetPath(LongestPath) + TEXT("/");
}

FString GetCommonRootPath(const TArray<FString>& FileMountPaths)
{
	FString Root = GetLongestPath(FileMountPaths);
	for (int32 FileIndex = 0; FileIndex < FileMountPaths.Num() && Root.Len(); FileIndex++)
	{
		FString Filename(FileMountPaths[FileIndex]);
		FString Path = FPaths::GetPath(Filename) + TEXT("/");
		int32 CommonSeparatorIndex = -1;
		int32 SeparatorIndex = Path.Find(TEXT("/"), ESearchCase::CaseSensitive);
		while (SeparatorIndex >= 0)
		{
			if (FCString::Strnicmp(*Root, *Path, SeparatorIndex + 1) != 0)
			{
				break;
			}
			CommonSeparatorIndex = SeparatorIndex;
			if (CommonSeparatorIndex + 1 < Path.Len())
			{
				SeparatorIndex = Path.Find(TEXT("/"), ESearchCase::CaseSensitive, ESearchDir::FromStart, CommonSeparatorIndex + 1);
			}
			else
			{
				break;
			}
		}
		if ((CommonSeparatorIndex + 1) < Root.Len())
		{
			Root.MidInline(0, CommonSeparatorIndex + 1, false);
		}
	}
	return Root;
}

FPakEntryPair PakEntryWrapper::AsPakEntryPair(const FString& MountPoint)const
{
	FPakEntryPair PakEntryPair;
	PakEntryPair.Filename = MountPath.Mid(MountPoint.Len());
	PakEntryPair.Info = PakEntry;
	return PakEntryPair;
}

FRemakePak::FRemakePak():PakWriter(PakWriterData)
{
	
}

bool FRemakePak::ResetPakEntryOffset(FPakEntry& PakEntry)
{
	PakEntry.Offset = PakWriter.TotalSize();
	return true;
}

void FRemakePak::AddPakEntry(const PakEntryWrapper& PakEntryWrapper)
{
	FPakEntry PakEntry = PakEntryWrapper.PakEntry;
	PakEntry.Offset = 0;
	PakEntry.Serialize(PakWriter,FPakInfo::PakFile_Version_Latest);
	PakWriter.Serialize(const_cast<uint8*>(PakEntryWrapper.Data.GetData()),PakEntryWrapper.PakEntry.Size);
	PakEntries.Add(PakEntryWrapper);
}

TArray<FString> FRemakePak::GetPakEntryMountPaths() const
{
	TArray<FString> result;
	for(const auto& PakEntryWrapper:PakEntries)
	{
		result.AddUnique(PakEntryWrapper.MountPath);
	}
	return result;
}

FString FRemakePak::GetFinalMountPoint() const
{
	return GetCommonRootPath(GetPakEntryMountPaths());
}

void FRemakePak::SerializeToDisk(const FString& Filename)
{
	SerializePak = IFileManager::Get().CreateFileWriter(*Filename);
	// serialize PakEntry + Content
	SerializePak->Serialize(PakWriterData.GetData(),PakWriterData.Num());

	FString MountPoint = GetFinalMountPoint();

	TArray<uint8> EncodedPakEntries;
	int32 NumEncodedEntries = 0;
	int32 NumDeletedEntries = 0;
	TArray<FPakEntry> NonEncodableEntries;


	int32 NextIndex = 0;
	TArray<FPakEntryPair> PakEntryPairs;
	for(const auto& PakEntry:PakEntries)
	{
		PakEntryPairs.Add(PakEntry.AsPakEntryPair(MountPoint));
	}
	auto ReadNextEntry = [&]() -> FPakEntryPair&
	{
		return PakEntryPairs[NextIndex++];
	};

	FPakInfo Info;
	Info.bEncryptedIndex = false;
	Info.EncryptionKeyGuid = FGuid();

#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION > 25
	FPakFile::FDirectoryIndex DirectoryIndex;
	FPakFile::FPathHashIndex PathHashIndex;
	TMap<uint64, FString> CollisionDetection; // Currently detecting Collisions only within the files stored into a single Pak.  TODO: Create separate job to detect collisions over all files in the export.
	uint64 PathHashSeed;
	FPakFile::EncodePakEntriesIntoIndex(PakEntries.Num(), ReadNextEntry, *Filename, Info, MountPoint, NumEncodedEntries, NumDeletedEntries, &PathHashSeed,
		&DirectoryIndex, &PathHashIndex, EncodedPakEntries, NonEncodableEntries, &CollisionDetection, FPakInfo::PakFile_Version_Latest);

		// We write one PrimaryIndex and two SecondaryIndexes to the Pak File
	// PrimaryIndex
	//		Common scalar data such as MountPoint
	//		PresenceBit and Offset,Size,Hash for the SecondaryIndexes
	//		PakEntries (Encoded and NonEncoded)
	// SecondaryIndex PathHashIndex: used by default in shipped versions of games.  Uses less memory, but does not provide access to all filenames.
	//		TMap from hash of FilePath to FPakEntryLocation
	//		Pruned DirectoryIndex, containing only the FilePaths that were requested kept by whitelist config variables
	// SecondaryIndex FullDirectoryIndex: used for developer tools and for titles that opt out of PathHashIndex because they need access to all filenames.
	//		TMap from DirectoryPath to FDirectory, which itself is a TMap from CleanFileName to FPakEntryLocation
	// Each Index is separately encrypted and hashed.  Runtime consumer such as the tools or the client game will only load one of these off of disk (unless runtime verification is turned on).

	// Create the pruned DirectoryIndex for use in the Primary Index
	TMap<FString, FPakDirectory> PrunedDirectoryIndex;
	FPakFile::PruneDirectoryIndex(DirectoryIndex, &PrunedDirectoryIndex, MountPoint);

	bool bWritePathHashIndex = FPakFile::IsPakWritePathHashIndex();
	bool bWriteFullDirectoryIndex = FPakFile::IsPakWriteFullDirectoryIndex();
	checkf(bWritePathHashIndex || bWriteFullDirectoryIndex, TEXT("At least one of Engine:[Pak]:WritePathHashIndex and Engine:[Pak]:WriteFullDirectoryIndex must be true"));

	TArray<uint8> PrimaryIndexData;
	TArray<uint8> PathHashIndexData;
	TArray<uint8> FullDirectoryIndexData;
	Info.IndexOffset = SerializePak->Tell();


	auto FinalizeIndexBlockSize = [&Info](TArray<uint8>& IndexData)
	{
		if (Info.bEncryptedIndex)
		{
			int32 OriginalSize = IndexData.Num();
			int32 AlignedSize = Align(OriginalSize, FAES::AESBlockSize);

			for (int32 PaddingIndex = IndexData.Num(); PaddingIndex < AlignedSize; ++PaddingIndex)
			{
				uint8 Byte = IndexData[PaddingIndex % OriginalSize];
				IndexData.Add(Byte);
			}
		}
	};
	auto FinalizeIndexBlock = [&] (TArray<uint8>& IndexData, FSHAHash& OutHash)
	{
		FinalizeIndexBlockSize(IndexData);

		FSHA1::HashBuffer(IndexData.GetData(), IndexData.Num(), OutHash.Hash);

		if (Info.bEncryptedIndex)
		{
			// check(InKeyChain.MasterEncryptionKey);
			// FAES::EncryptData(IndexData.GetData(), IndexData.Num(), InKeyChain.MasterEncryptionKey->Key);
			// TotalEncryptedDataSize += IndexData.Num();
		}
	};
	
	// Write PrimaryIndex bytes
	{
		FMemoryWriter PrimaryIndexWriter(PrimaryIndexData);
		PrimaryIndexWriter.SetByteSwapping(SerializePak->ForceByteSwapping());

		PrimaryIndexWriter << MountPoint;
		int32 NumEntries = PakEntries.Num();
		PrimaryIndexWriter << NumEntries;
		PrimaryIndexWriter << PathHashSeed;

		FSecondaryIndexWriter PathHashIndexWriter(PathHashIndexData, bWritePathHashIndex, PrimaryIndexData, PrimaryIndexWriter);
		PathHashIndexWriter.WritePlaceholderToPrimary();

		FSecondaryIndexWriter FullDirectoryIndexWriter(FullDirectoryIndexData, bWriteFullDirectoryIndex, PrimaryIndexData, PrimaryIndexWriter);
		FullDirectoryIndexWriter.WritePlaceholderToPrimary();

		PrimaryIndexWriter << EncodedPakEntries;
		int32 NonEncodableEntriesNum = NonEncodableEntries.Num();
		PrimaryIndexWriter << NonEncodableEntriesNum;
		for (FPakEntry& PakEntry : NonEncodableEntries)
		{
			PakEntry.Serialize(PrimaryIndexWriter, FPakInfo::PakFile_Version_Latest);
		}

		// Finalize the size of the PrimaryIndex (it may change due to alignment padding) because we need the size to know the offset of the SecondaryIndexes which come after it in the PakFile.
		// Do not encrypt and hash it yet, because we still need to replace placeholder data in it for the Offset,Size,Hash of each SecondaryIndex
		FinalizeIndexBlockSize(PrimaryIndexData);

		// Write PathHashIndex bytes
		if (bWritePathHashIndex)
		{
			{
				FMemoryWriter& SecondaryWriter = PathHashIndexWriter.GetSecondaryWriter();
				SecondaryWriter << PathHashIndex;
				SecondaryWriter << PrunedDirectoryIndex;
			}
			PathHashIndexWriter.FinalizeAndRecordOffset(Info.IndexOffset + PrimaryIndexData.Num(), FinalizeIndexBlock);
		}

		// Write FullDirectoryIndex bytes
		if (bWriteFullDirectoryIndex)
		{
			{
				FMemoryWriter& SecondaryWriter = FullDirectoryIndexWriter.GetSecondaryWriter();
				SecondaryWriter << DirectoryIndex;
			}
			FullDirectoryIndexWriter.FinalizeAndRecordOffset(Info.IndexOffset + PrimaryIndexData.Num() + PathHashIndexData.Num(), FinalizeIndexBlock);
		}

		// Encrypt and Hash the PrimaryIndex now that we have filled in the SecondaryIndex information
		FinalizeIndexBlock(PrimaryIndexData, Info.IndexHash);
	}

	// Write the bytes for each Index into the PakFile
	Info.IndexSize = PrimaryIndexData.Num();
	SerializePak->Serialize(PrimaryIndexData.GetData(), PrimaryIndexData.Num());
	if (bWritePathHashIndex)
	{
		SerializePak->Serialize(PathHashIndexData.GetData(), PathHashIndexData.Num());
	}
	if (bWriteFullDirectoryIndex)
	{
		SerializePak->Serialize(FullDirectoryIndexData.GetData(), FullDirectoryIndexData.Num());
	}

	// Save the FPakInfo, which has offset, size, and hash value for the PrimaryIndex, at the end of the PakFile
	Info.Serialize(*SerializePak, FPakInfo::PakFile_Version_Latest);
#endif
	SerializePak->Close();
}
