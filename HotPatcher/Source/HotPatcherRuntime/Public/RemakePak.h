#pragma once

#include "CoreMinimal.h"
#include "IPlatformFilePak.h"

struct PakEntryWrapper
{
	FString MountPath;
	FPakEntry PakEntry;
	TArray<uint8> Data;
	FPakEntryPair AsPakEntryPair(const FString& MountPoint)const;
};

struct FRemakePak
{
	FRemakePak();
	bool ResetPakEntryOffset(FPakEntry& PakEntry);
	void AddPakEntry(const PakEntryWrapper& PakEntryWrapper);
	TArray<FString> GetPakEntryMountPaths()const;
	FString GetFinalMountPoint()const;
	void SerializeToDisk(const FString& Filename);
protected:
	TArray<PakEntryWrapper> PakEntries;
	FMemoryWriter PakWriter;
	TArray<uint8> PakWriterData;
	FPakInfo PakInfo;
	FArchive* SerializePak;
};
