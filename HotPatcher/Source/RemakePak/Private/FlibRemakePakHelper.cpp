#include "FlibRemakePakHelper.h"
#include "FlibAssetManageHelper.h"
#include "FlibPakHelper.h"
#include "HotPatcherTemplateHelper.hpp"

FAES::FAESKey CachedAESKey;
#define AES_KEY_SIZE 32

DEFINE_LOG_CATEGORY(LogRemakePak);


FSHA1 UFlibRemakePakHelper::GetPakEntryHASH(FPakFile* InPakFile,const FPakEntry& PakEntry)
{
	FSHA1 Sha1;
	FArchive* Reader = InPakFile->GetSharedReader(nullptr);
	Reader->Seek(PakEntry.Offset);
	FPakEntry SerializedEntry;
	SerializedEntry.Serialize(*Reader, InPakFile->GetInfo().Version);
	FMemory::Memcpy(Sha1.m_digest, &SerializedEntry.Hash, sizeof(SerializedEntry.Hash));
	return Sha1;
}

void UFlibRemakePakHelper::DumpPakEntrys(const FString& InPak, const FString& AESKey, const FString& SaveTo)
{
	auto PakFilePtr = UFlibPakHelper::GetPakFileIns(InPak,AESKey);;
#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION > 26
	TRefCountPtr<FPakFile> PakFile = PakFilePtr;
#else
	TSharedPtr<FPakFile> PakFile = MakeShareable(PakFilePtr);
#endif
	
	TMap<FString,FPakEntry> Records = UFlibPakHelper::GetPakEntrys(PakFilePtr,AESKey);
	FString FileName = FPaths::GetBaseFilename(InPak,true);
	FPakDumper Results;
	Results.PakName = FileName;
	Results.MountPoint = PakFilePtr->GetMountPoint();
	
	for(const auto& Pair:Records)
	{
		FDumpPakEntry Entry;
		FString PakFilename = Pair.Key;
		FString PackageName;
		FString RecordName;
		bool bIsAsset = false;
		if(FPackageName::TryConvertFilenameToLongPackageName(PakFilename,PackageName))
		{
			bIsAsset = true;
			RecordName = PackageName;
		}
		else
		{
			RecordName = PakFilename;
		}
		
		PakFilename = FPaths::GetBaseFilename(PakFilename,true) + FPaths::GetExtension(PakFilename,true);
		Entry.ContentSize = Pair.Value.Size;
		Entry.Offset = Pair.Value.Offset;
		FDumpPakAsset& RecordItem = Results.PakEntrys.FindOrAdd(RecordName);
		Entry.PakEntrySize = Pair.Value.GetSerializedSize(FPakInfo::PakFile_Version_Latest);
		if(bIsAsset)
		{
			UFlibAssetManageHelper::GetAssetPackageGUID(PackageName,RecordItem.GUID);
		}
		else
		{
			FSHAHash Hash;
			FMemory::Memcpy(Hash.Hash,Pair.Value.Hash,20);
			RecordItem.GUID = *Hash.ToString();
		}
		RecordItem.PackageName = PackageName;
		
		RecordItem.AssetEntrys.Add(PakFilename,Entry);
	}
	FString OutString;
	THotPatcherTemplateHelper::TSerializeStructAsJsonString(Results,OutString);
	FFileHelper::SaveStringToFile(OutString,*SaveTo);
}

#include "FRemakePak.h"
void UFlibRemakePakHelper::TestRemakePak(const FString& OldPak, const FString& NewPak)
{
	FString AESKey = TEXT("");
	auto PakFilePtr = UFlibPakHelper::GetPakFileIns(OldPak,AESKey);;
#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION > 26
	TRefCountPtr<FPakFile> PakFile = PakFilePtr;
#else
	TSharedPtr<FPakFile> PakFile = MakeShareable(PakFilePtr);
#endif
	
	TMap<FString,FPakEntry> Records = UFlibPakHelper::GetPakEntrys(PakFilePtr,AESKey);

	TArray<PakEntryWrapper> PakEntryWrappers;

	IPlatformFile* PlatformIns = &FPlatformFileManager::Get().GetPlatformFile();
	IFileHandle* OldPakFileHandle = PlatformIns->OpenRead(*OldPak);
	
	for(const auto& PakEntry:Records)
	{
		PakEntryWrapper CurrentPakEntryWrapper;
		CurrentPakEntryWrapper.MountPath = PakEntry.Key;
		CurrentPakEntryWrapper.PakEntry = PakEntry.Value;
		OldPakFileHandle->Seek(PakEntry.Value.Offset+PakEntry.Value.GetSerializedSize(FPakInfo::PakFile_Version_Latest));
		CurrentPakEntryWrapper.Data.SetNumUninitialized(PakEntry.Value.Size);
		OldPakFileHandle->Read(CurrentPakEntryWrapper.Data.GetData(),PakEntry.Value.Size);
		PakEntryWrappers.Emplace(std::move(CurrentPakEntryWrapper));
	}

	FRemakePak RemakePak;
	
	for(auto& PakEntryWrapper:PakEntryWrappers)
	{
		RemakePak.ResetPakEntryOffset(PakEntryWrapper.PakEntry);
		RemakePak.AddPakEntry(PakEntryWrapper);
	}

	RemakePak.SerializeToDisk(NewPak);
}
