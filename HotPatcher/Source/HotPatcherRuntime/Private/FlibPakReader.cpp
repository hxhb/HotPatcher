// Fill out your copyright notice in the Description page of Project Settings.


#include "FlibPakReader.h"

#include "Misc/FileHelper.h"
#include "IPlatformFilePak.h"
#include "HAL/PlatformFilemanager.h"
#include "Resources/Version.h"

/**
 * Thread local class to manage working buffers for file compression
 */
class FCompressionScratchBuffers : public TThreadSingleton<FCompressionScratchBuffers>
{
public:
	FCompressionScratchBuffers()
		: TempBufferSize(0)
		, ScratchBufferSize(0)
		, LastReader(nullptr)
		, LastDecompressedBlock(0xFFFFFFFF)
	{}

	int64				TempBufferSize;
	TUniquePtr<uint8[]>	TempBuffer;
	int64				ScratchBufferSize;
	TUniquePtr<uint8[]>	ScratchBuffer;

	void* LastReader;
	uint32 LastDecompressedBlock;

	void EnsureBufferSpace(int64 CompressionBlockSize, int64 ScrachSize)
	{
		if (TempBufferSize < CompressionBlockSize)
		{
			TempBufferSize = CompressionBlockSize;
			TempBuffer = MakeUnique<uint8[]>(TempBufferSize);
		}
		if (ScratchBufferSize < ScrachSize)
		{
			ScratchBufferSize = ScrachSize;
			ScratchBuffer = MakeUnique<uint8[]>(ScratchBufferSize);
		}
	}
	
};
/**
 * Class to handle correctly reading from a compressed file within a pak
 */
template< typename EncryptionPolicy = FPakNoEncryption >
class FPakCompressedReaderPolicy
{
public:
	class FPakUncompressTask : public FNonAbandonableTask
	{
	public:
		uint8*				UncompressedBuffer;
		int32				UncompressedSize;
		uint8*				CompressedBuffer;
		int32				CompressedSize;
		FName				CompressionFormat;
		void*				CopyOut;
		int64				CopyOffset;
		int64				CopyLength;
		FGuid				EncryptionKeyGuid;

		void DoWork()
		{
			// Decrypt and Uncompress from memory to memory.
			int64 EncryptionSize = EncryptionPolicy::AlignReadRequest(CompressedSize);
			EncryptionPolicy::DecryptBlock(CompressedBuffer, EncryptionSize, EncryptionKeyGuid);
			FCompression::UncompressMemory(CompressionFormat, UncompressedBuffer, UncompressedSize, CompressedBuffer, CompressedSize);
			if (CopyOut)
			{
				FMemory::Memcpy(CopyOut, UncompressedBuffer + CopyOffset, CopyLength);
			}
		}

		FORCEINLINE TStatId GetStatId() const
		{
			// TODO: This is called too early in engine startup.
			return TStatId();
			//RETURN_QUICK_DECLARE_CYCLE_STAT(FPakUncompressTask, STATGROUP_ThreadPoolAsyncTasks);
		}
	};

	FPakCompressedReaderPolicy(const FPakFile& InPakFile, const FPakEntry& InPakEntry, TAcquirePakReaderFunction& InAcquirePakReader)
		: PakFile(InPakFile)
		, PakEntry(InPakEntry)
		, AcquirePakReader(InAcquirePakReader)
	{
	}

	~FPakCompressedReaderPolicy()
	{
		FCompressionScratchBuffers& ScratchSpace = FCompressionScratchBuffers::Get();
		if(ScratchSpace.LastReader == this)
		{
			ScratchSpace.LastDecompressedBlock = 0xFFFFFFFF;
			ScratchSpace.LastReader = nullptr;
		}
	}

	/** Pak file that own this file data */
	const FPakFile&		PakFile;
	/** Pak file entry for this file. */
	FPakEntry			PakEntry;
	/** Function that gives us an FArchive to read from. The result should never be cached, but acquired and used within the function doing the serialization operation */
	TAcquirePakReaderFunction AcquirePakReader;

	FORCEINLINE int64 FileSize() const
	{
		return PakEntry.UncompressedSize;
	}

	void Serialize(int64 DesiredPosition, void* V, int64 Length)
	{
		const int32 CompressionBlockSize = PakEntry.CompressionBlockSize;
		uint32 CompressionBlockIndex = DesiredPosition / CompressionBlockSize;
		uint8* WorkingBuffers[2];
		int64 DirectCopyStart = DesiredPosition % PakEntry.CompressionBlockSize;
		FAsyncTask<FPakUncompressTask> UncompressTask;
		FCompressionScratchBuffers& ScratchSpace = FCompressionScratchBuffers::Get();
		bool bStartedUncompress = false;

		FName CompressionMethod = PakFile.GetInfo().GetCompressionMethod(PakEntry.CompressionMethodIndex);
		checkf(FCompression::IsFormatValid(CompressionMethod), 
			TEXT("Attempting to use compression format %s when loading a file from a .pak, but that compression format is not available.\n")
			TEXT("If you are running a program (like UnrealPak) you may need to pass the .uproject on the commandline so the plugin can be found.\n"),
			TEXT("It's also possible that a necessary compression plugin has not been loaded yet, and this file needs to be forced to use zlib compression.\n")
			TEXT("Unfortunately, the code that can check this does not have the context of the filename that is being read. You will need to look in the callstack in a debugger.\n")
			TEXT("See ExtensionsToNotUsePluginCompression in [Pak] section of Engine.ini to add more extensions."),
			*CompressionMethod.ToString(), TEXT("Unknown"));

		// an amount to extra allocate, in case one block's compressed size is bigger than CompressMemoryBound
		float SlopMultiplier = 1.1f;
		int64 WorkingBufferRequiredSize = FCompression::CompressMemoryBound(CompressionMethod, CompressionBlockSize) * SlopMultiplier;
		WorkingBufferRequiredSize = EncryptionPolicy::AlignReadRequest(WorkingBufferRequiredSize);
		const bool bExistingScratchBufferValid = ScratchSpace.TempBufferSize >= CompressionBlockSize;
		ScratchSpace.EnsureBufferSpace(CompressionBlockSize, WorkingBufferRequiredSize * 2);
		WorkingBuffers[0] = ScratchSpace.ScratchBuffer.Get();
		WorkingBuffers[1] = ScratchSpace.ScratchBuffer.Get() + WorkingBufferRequiredSize;

		FArchive* PakReader = AcquirePakReader();

		while (Length > 0)
		{
			const FPakCompressedBlock& Block = PakEntry.CompressionBlocks[CompressionBlockIndex];
			int64 Pos = CompressionBlockIndex * CompressionBlockSize;
			int64 CompressedBlockSize = Block.CompressedEnd - Block.CompressedStart;
			int64 UncompressedBlockSize = FMath::Min<int64>(PakEntry.UncompressedSize - Pos, PakEntry.CompressionBlockSize);

			if (CompressedBlockSize > UncompressedBlockSize)
			{
				UE_LOG(LogPakFile, Display, TEXT("Bigger compressed? Block[%d]: %d -> %d > %d [%d min %d]"), CompressionBlockIndex, Block.CompressedStart, Block.CompressedEnd, UncompressedBlockSize, PakEntry.UncompressedSize - Pos, PakEntry.CompressionBlockSize);
			}


			int64 ReadSize = EncryptionPolicy::AlignReadRequest(CompressedBlockSize);
			int64 WriteSize = FMath::Min<int64>(UncompressedBlockSize - DirectCopyStart, Length);

			const bool bCurrentScratchTempBufferValid = 
				bExistingScratchBufferValid && !bStartedUncompress
				// ensure this object was the last reader from the scratch buffer and the last thing it decompressed was this block.
				&& ScratchSpace.LastReader == this && ScratchSpace.LastDecompressedBlock == CompressionBlockIndex 
				// ensure the previous decompression destination was the scratch buffer.
				&& !(DirectCopyStart == 0 && Length >= CompressionBlockSize); 

			if (bCurrentScratchTempBufferValid)
			{
				// Reuse the existing scratch buffer to avoid repeatedly deserializing and decompressing the same block.
				FMemory::Memcpy(V, ScratchSpace.TempBuffer.Get() + DirectCopyStart, WriteSize);
			}
			else
			{
			PakReader->Seek(Block.CompressedStart + (PakFile.GetInfo().HasRelativeCompressedChunkOffsets() ? PakEntry.Offset : 0));
			PakReader->Serialize(WorkingBuffers[CompressionBlockIndex & 1], ReadSize);
			if (bStartedUncompress)
			{
				UncompressTask.EnsureCompletion();
				bStartedUncompress = false;
			}

			FPakUncompressTask& TaskDetails = UncompressTask.GetTask();
			TaskDetails.EncryptionKeyGuid = PakFile.GetInfo().EncryptionKeyGuid;

			if (DirectCopyStart == 0 && Length >= CompressionBlockSize)
			{
				// Block can be decompressed directly into output buffer
				TaskDetails.CompressionFormat = CompressionMethod;
				TaskDetails.UncompressedBuffer = (uint8*)V;
				TaskDetails.UncompressedSize = UncompressedBlockSize;
				TaskDetails.CompressedBuffer = WorkingBuffers[CompressionBlockIndex & 1];
				TaskDetails.CompressedSize = CompressedBlockSize;
				TaskDetails.CopyOut = nullptr;
					ScratchSpace.LastDecompressedBlock = 0xFFFFFFFF;
					ScratchSpace.LastReader = nullptr;
			}
			else
			{
				// Block needs to be copied from a working buffer
				TaskDetails.CompressionFormat = CompressionMethod;
				TaskDetails.UncompressedBuffer = ScratchSpace.TempBuffer.Get();
				TaskDetails.UncompressedSize = UncompressedBlockSize;
				TaskDetails.CompressedBuffer = WorkingBuffers[CompressionBlockIndex & 1];
				TaskDetails.CompressedSize = CompressedBlockSize;
				TaskDetails.CopyOut = V;
				TaskDetails.CopyOffset = DirectCopyStart;
				TaskDetails.CopyLength = WriteSize;

					ScratchSpace.LastDecompressedBlock = CompressionBlockIndex;
					ScratchSpace.LastReader = this;
			}

			if (Length == WriteSize)
			{
				UncompressTask.StartSynchronousTask();
			}
			else
			{
				UncompressTask.StartBackgroundTask();
			}
			bStartedUncompress = true;
			}
		
			V = (void*)((uint8*)V + WriteSize);
			Length -= WriteSize;
			DirectCopyStart = 0;
			++CompressionBlockIndex;
		}

		if (bStartedUncompress)
		{
			UncompressTask.EnsureCompletion();
		}
	}
};

void DecryptDataEx(uint8* InData, uint32 InDataSize, FGuid InEncryptionKeyGuid)
{
#if ENGINE_MINOR_VERSION >22
	if (FPakPlatformFile::GetPakCustomEncryptionDelegate().IsBound())
	{
		FPakPlatformFile::GetPakCustomEncryptionDelegate().Execute(InData, InDataSize, InEncryptionKeyGuid);
	}
	else
#endif
	{
		// SCOPE_SECONDS_ACCUMULATOR(STAT_PakCache_DecryptTime);
		FAES::FAESKey Key;
		FPakPlatformFile::GetPakEncryptionKey(Key, InEncryptionKeyGuid);
		check(Key.IsValid());
		FAES::DecryptData(InData, InDataSize, Key);
	}
}

/**
* Class to handle correctly reading from a compressed file within a compressed package
*/
class FPakSimpleEncryption
{
public:
	enum
	{
		Alignment = FAES::AESBlockSize,
    };

	static FORCEINLINE int64 AlignReadRequest(int64 Size)
	{
		return Align(Size, Alignment);
	}

	static FORCEINLINE void DecryptBlock(void* Data, int64 Size, const FGuid& EncryptionKeyGuid)
	{
		// INC_DWORD_STAT(STAT_PakCache_SyncDecrypts);
		DecryptDataEx((uint8*)Data, Size, EncryptionKeyGuid);
	}
};


FPakFile* UFlibPakReader::GetPakFileInsByPath(const FString& PakPath)
{
	FPakFile* Pak = NULL;
#if !WITH_EDITOR
	FPakPlatformFile* PakFileMgr = (FPakPlatformFile*)FPlatformFileManager::Get().GetPlatformFile(FPakPlatformFile::GetTypeName());
	IPlatformFile* LowerLevel = PakFileMgr->GetLowerLevel();
	if(LowerLevel)
	{
#if ENGINE_MINOR_VERSION > 21
		Pak = new FPakFile(LowerLevel, *PakPath, false, true);
#else
		Pak = new FPakFile(LowerLevel, *PakPath, false);
#endif
	}
#endif
	return Pak;
}

TArray<FString> UFlibPakReader::GetPakFileList(const FString& PakFilePath)
{
	TArray<FString> Files;
	FPakFile* Pak = UFlibPakReader::GetPakFileInsByPath(PakFilePath);
	if(Pak)
	{
#if ENGINE_MINOR_VERSION >22
		Pak->GetFilenames(Files);
#endif
	}
	return Files;
}

bool UFlibPakReader::FindFileInPakFile(FPakFile* InPakFile, const FString& InFileName, FPakEntry* OutPakEntry)
{
	bool result = false;
	if(InPakFile)
	{
		FPakEntry FileEntry;
		FPakFile::EFindResult FindResult = InPakFile->Find(InFileName,&FileEntry);
		if(FPakFile::EFindResult::Found == FindResult)
		{
			*OutPakEntry = FileEntry;
			result = true;
		}
	}
	return result;
}

IFileHandle* UFlibPakReader::CreatePakFileHandle(IPlatformFile* InLowLevel, FPakFile* PakFile, const FPakEntry* FileEntry)
{
	IFileHandle* Result = NULL;
	bool bNeedsDelete = true;
	TFunction<FArchive*()> AcquirePakReader = [PakFile, LowerLevelPlatformFile = InLowLevel]() { return PakFile->GetSharedReader(LowerLevelPlatformFile); };

	// Create the handle.
	if (FileEntry->CompressionMethodIndex != 0 && PakFile->GetInfo().Version >= FPakInfo::PakFile_Version_CompressionEncryption)
	{
		if (FileEntry->IsEncrypted())
		{
			Result = new FPakFileHandle< FPakCompressedReaderPolicy<FPakSimpleEncryption> >(*PakFile, *FileEntry, AcquirePakReader, bNeedsDelete);
		}
		else
		{
			Result = new FPakFileHandle< FPakCompressedReaderPolicy<> >(*PakFile, *FileEntry, AcquirePakReader, bNeedsDelete);
		}
	}
	else if (FileEntry->IsEncrypted())
	{
		Result = new FPakFileHandle< FPakReaderPolicy<FPakSimpleEncryption> >(*PakFile, *FileEntry, AcquirePakReader, bNeedsDelete);
	}
	else
	{
		Result = new FPakFileHandle<>(*PakFile, *FileEntry, AcquirePakReader, bNeedsDelete);
	}

	return Result;
}

/**
 * Load a text file to an FString.
 * Supports all combination of ANSI/Unicode files and platforms.
 * @param Result string representation of the loaded file
 * @param Filename name of the file to load
 * @param VerifyFlags flags controlling the hash verification behavior ( see EHashOptions )
 */
bool UFlibPakReader::LoadFileToString(FString& Result, FArchive* InReader, const TCHAR* Filename, FFileHelper::EHashOptions VerifyFlags)
{
	TUniquePtr<FArchive> Reader( InReader);
	if( !Reader )
	{
		return false;
	}
	
	int32 Size = Reader->TotalSize();
	if( !Size )
	{
		Result.Empty();
		return true;
	}

	uint8* Ch = (uint8*)FMemory::Malloc(Size);
	Reader->Serialize( Ch, Size );
	bool Success = Reader->Close();
	Reader = nullptr;
	FFileHelper::BufferToString( Result, Ch, Size );

	// handle SHA verify of the file
	if( EnumHasAnyFlags(VerifyFlags, FFileHelper::EHashOptions::EnableVerify) && ( EnumHasAnyFlags(VerifyFlags, FFileHelper::EHashOptions::ErrorMissingHash) || FSHA1::GetFileSHAHash(Filename, NULL) ) )
	{
		// kick off SHA verify task. this frees the buffer on close
		FBufferReaderWithSHA Ar( Ch, Size, true, Filename, false, true );
	}
	else
	{
		// free manually since not running SHA task
		FMemory::Free(Ch);
	}

	return Success;
	
}

#if PLATFORM_WINDOWS

#include "HACK_PRIVATE_MEMBER_UTILS.hpp"
DECL_HACK_PRIVATE_NOCONST_FUNCTION(FPakFile,CreatePakReader,FArchive*,IFileHandle&,const TCHAR*);

FArchive* UFlibPakReader::CreatePakReader(FPakFile* InPakFile, IFileHandle& InHandle, const TCHAR* InFilename)
{
	auto FPakFile_CreatePakReader =GET_PRIVATE_MEMBER_FUNCTION(FPakFile, CreatePakReader);
	return CALL_MEMBER_FUNCTION(InPakFile, FPakFile_CreatePakReader, InHandle,InFilename);
}

FString UFlibPakReader::LoadPakFileToString(const FString& InPakFile, const FString& InFileName)
{
	FString ret;
	if(FPaths::FileExists(InPakFile))
	{
		FPakFile* PakFile = UFlibPakReader::GetPakFileInsByPath(InPakFile);
		if(!PakFile)
			return ret;
		FPakEntry PakEntry;
		if(UFlibPakReader::FindFileInPakFile(PakFile,InFileName,&PakEntry))
		{
			FPakPlatformFile* PakFileMgr = (FPakPlatformFile*)FPlatformFileManager::Get().GetPlatformFile(FPakPlatformFile::GetTypeName());
			IPlatformFile* LowerLevel = PakFileMgr->GetLowerLevel();
			if(LowerLevel)
			{
				IFileHandle* FileHandle = UFlibPakReader::CreatePakFileHandle(LowerLevel,PakFile,&PakEntry);
				if(FileHandle)
				{
					TUniquePtr<FArchive> Reader(UFlibPakReader::CreatePakReader(PakFile,*FileHandle, *InFileName));
#if ENGINE_MINOR_VERSION > 25
					FFileHelper::LoadFileToString(ret,*Reader.Get());
#else
					UFlibPakReader::LoadFileToString(ret,Reader.Get(),*InFileName);
#endif
				}
			}
		}
	}
	return ret;
}

#endif