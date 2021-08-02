// Fill out your copyright notice in the Description page of Project Settings.


#include "FlibPakHelper.h"
#include "IPlatformFilePak.h"
#include "HAL/PlatformFilemanager.h"
#include "AssetManager/FFileArrayDirectoryVisitor.hpp"
#include "HotPatcherLog.h"

// Engine Header
#include "Resources/Version.h"
#include "Serialization/ArrayReader.h"
#include "AssetRegistryModule.h"
#include "IAssetRegistry.h"
#include "Misc/ScopeExit.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Misc/FileHelper.h"
#include "IPlatformFilePak.h"
#include "ShaderPipelineCache.h"
#include "RHI.h"
#include "Misc/Base64.h"
#include "Resources/Version.h"

void UFlibPakHelper::ExecMountPak(FString InPakPath, int32 InPakOrder, FString InMountPoint)
{
	UFlibPakHelper::MountPak(InPakPath, InPakOrder, InMountPoint);
}

bool UFlibPakHelper::MountPak(const FString& PakPath, int32 PakOrder, const FString& InMountPoint)
{
	bool bMounted = false;

	FPakPlatformFile* PakFileMgr = (FPakPlatformFile*)FPlatformFileManager::Get().GetPlatformFile(FPakPlatformFile::GetTypeName());
	if (!PakFileMgr)
	{
		UE_LOG(LogHotPatcher, Log, TEXT("GetPlatformFile(TEXT(\"PakFile\") is NULL"));
		return false;
	}

	PakOrder = FMath::Max(0, PakOrder);

	if (FPaths::FileExists(PakPath) && FPaths::GetExtension(PakPath) == TEXT("pak"))
	{
		bool bIsEmptyMountPoint = InMountPoint.IsEmpty();
		const TCHAR* MountPoint = bIsEmptyMountPoint ? NULL : InMountPoint.GetCharArray().GetData();

#if !WITH_EDITOR
		
		if (PakFileMgr->Mount(*PakPath, PakOrder, MountPoint))
		{
			UE_LOG(LogHotPatcher, Log, TEXT("Mounted = %s, Order = %d, MountPoint = %s"), *PakPath, PakOrder, !MountPoint ? TEXT("(NULL)") : MountPoint);
			bMounted = true;
		}
		else {
			UE_LOG(LogHotPatcher, Error, TEXT("Faild to mount pak = %s"), *PakPath);
			bMounted = false;
		}

#endif
	}

	return bMounted;
}

bool UFlibPakHelper::UnMountPak(const FString& PakPath)
{
	bool bMounted = false;

	FPakPlatformFile* PakFileMgr = (FPakPlatformFile*)FPlatformFileManager::Get().GetPlatformFile(FPakPlatformFile::GetTypeName());
	if (!PakFileMgr)
	{
		UE_LOG(LogHotPatcher, Log, TEXT("GetPlatformFile(TEXT(\"PakFile\") is NULL"));
		return false;
	}

	if (!FPaths::FileExists(PakPath))
		return false;
	return PakFileMgr->Unmount(*PakPath);
}

bool UFlibPakHelper::CreateFileByBytes(const FString& InFile, const TArray<uint8>& InBytes, int32 InWriteFlag /*= 0*/)
{
	if (InFile.IsEmpty()/* && FPaths::FileExists(InFile)*/)
	{
		UE_LOG(LogHotPatcher, Log, TEXT("CreateFileByBytes InFile is Empty!"));
		// UE_LOG(LogHotPatcher, Log, TEXT("CreateFileByBytes file %s existing!"), *InFile);
		return false;
	}
	FArchive* SaveToFile = NULL;
	SaveToFile = IFileManager::Get().CreateFileWriter(*InFile, InWriteFlag);
	check(SaveToFile != nullptr);
	SaveToFile->Serialize((void*)InBytes.GetData(), InBytes.Num());
	delete SaveToFile;
	return true;
}


bool UFlibPakHelper::ScanPlatformDirectory(const FString& InRelativePath, bool bIncludeFile, bool bIncludeDir, bool bRecursively, TArray<FString>& OutResault)
{
	bool bRunStatus = false;
	OutResault.Reset();

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if(PlatformFile.DirectoryExists(*InRelativePath))
	{
		TArray<FString> Files;
		TArray<FString> Dirs;

		FFileArrayDirectoryVisitor FallArrayDirVisitor;
		if (bRecursively)
		{
			PlatformFile.IterateDirectoryRecursively(*InRelativePath, FallArrayDirVisitor);
		}
		else 
		{
			PlatformFile.IterateDirectory(*InRelativePath, FallArrayDirVisitor);
		}
		Files = FallArrayDirVisitor.Files;
		Dirs = FallArrayDirVisitor.Directories;

		if (bIncludeFile)
		{
			OutResault = Files;
		}
		if (bIncludeDir)
		{
			OutResault.Append(Dirs);
		}

		bRunStatus = true;
	}
	else 
	{
		UE_LOG(LogHotPatcher, Warning, TEXT("Directory %s is not found."), *InRelativePath);
	}

	return bRunStatus;
}

bool UFlibPakHelper::SerializePakVersionToString(const FPakVersion& InPakVersion, FString& OutString)
{
	bool bRunStatus = false;
	OutString = TEXT("");

	TSharedPtr<FJsonObject> RootJsonObject = MakeShareable(new FJsonObject);
	if (UFlibPakHelper::SerializePakVersionToJsonObject(InPakVersion, RootJsonObject))
	{
		auto JsonWriter = TJsonWriterFactory<TCHAR>::Create(&OutString);
		if (FJsonSerializer::Serialize(RootJsonObject.ToSharedRef(), JsonWriter))
		{
			bRunStatus = true;
		}
	}
	return bRunStatus;
}

bool UFlibPakHelper::SerializePakVersionToJsonObject(const FPakVersion& InPakVersion, TSharedPtr<FJsonObject>& OutJsonObject)
{
	bool bRunStatus = false;

	if(OutJsonObject.IsValid())
	{
		OutJsonObject = MakeShareable(new FJsonObject);
	}

	OutJsonObject->SetStringField(TEXT("VersionId"), InPakVersion.VersionId);
	OutJsonObject->SetStringField(TEXT("BaseVersionId"), InPakVersion.BaseVersionId);
	OutJsonObject->SetStringField(TEXT("Date"), InPakVersion.Date);
	OutJsonObject->SetStringField(TEXT("CheckCode"), InPakVersion.CheckCode);
	bRunStatus = true;

	return bRunStatus;
}

bool UFlibPakHelper::DeserializeStringToPakVersion(const FString& InString, FPakVersion& OutPakVersion)
{
	bool bRunStatus = false;
	if (!InString.IsEmpty())
	{
		TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(InString);
		TSharedPtr<FJsonObject> DeserializeJsonObject;
		if (FJsonSerializer::Deserialize(JsonReader, DeserializeJsonObject))
		{
			bRunStatus = DeserializeJsonObjectToPakVersion(DeserializeJsonObject, OutPakVersion);
		}
	}
	return bRunStatus;
}

bool UFlibPakHelper::DeserializeJsonObjectToPakVersion(const TSharedPtr<FJsonObject>& InJsonObject, FPakVersion& OutPakVersion)
{
	bool bRunStatus = false;

	if (!InJsonObject.IsValid())
	{
		return false;
	}
	{
		bool bVersionId = InJsonObject->TryGetStringField(TEXT("VersionId"), OutPakVersion.VersionId);
		bool bBaseVersionId = InJsonObject->TryGetStringField(TEXT("BaseVersionId"), OutPakVersion.BaseVersionId);
		bool bDate = InJsonObject->TryGetStringField(TEXT("Date"), OutPakVersion.Date);
		bool bCheckCode = InJsonObject->TryGetStringField(TEXT("CheckCode"), OutPakVersion.CheckCode);
		bRunStatus = bVersionId && bBaseVersionId && bDate && bCheckCode;
	}

	return bRunStatus;
}

bool UFlibPakHelper::ScanExtenFilesInDirectory(const FString& InRelativePath, const FString& InExtenPostfix, bool InRecursively, TArray<FString>& OutFiles)
{
	OutFiles.Reset();
	TArray<FString> ReceiveScanResult;
	if (!UFlibPakHelper::ScanPlatformDirectory(InRelativePath, true, false, InRecursively, ReceiveScanResult))
	{
		UE_LOG(LogHotPatcher, Warning, TEXT("UFlibPakHelper::ScanPlatformDirectory return false."));
		return false;
	}
	TArray<FString> FinalResult;
	for (const auto& File : ReceiveScanResult)
	{
		if (File.EndsWith(*InExtenPostfix,ESearchCase::IgnoreCase))
		{
			FinalResult.AddUnique(File);
		}
	}
	OutFiles = FinalResult;
	return true;
}

TArray<FString> UFlibPakHelper::ScanAllVersionDescribleFiles()
{
	TArray<FString> FinalResult;

	FString VersionDescribleDir = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Extension/Versions"));
	UFlibPakHelper::ScanExtenFilesInDirectory(VersionDescribleDir, TEXT(".json"), true, FinalResult);

	return FinalResult;
}

TArray<FString> UFlibPakHelper::ScanExtenPakFiles()
{
	TArray<FString> FinalResult;

	FString ExtenPakDir = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("ExtenPak"));
	UFlibPakHelper::ScanExtenFilesInDirectory(ExtenPakDir, TEXT(".pak"), true, FinalResult);

	return FinalResult;
}

// bool UFlibPakHelper::LoadPakDoSomething(const FString& InPakFile, TFunction<bool(const FPakFile*)> InDoSomething)
// {
// 	bool bRunStatus = false;
// 	bool bMounted = false;
// 	FPakPlatformFile* PakPlatform = (FPakPlatformFile*)&FPlatformFileManager::Get().GetPlatformFile();
//
// 	FString StandardFileName = InPakFile;
// 	FPaths::MakeStandardFilename(StandardFileName);
//
// 	TSharedPtr<FPakFile> PakFile = MakeShareable(new FPakFile(PakPlatform->GetLowerLevel(), *StandardFileName, false));
//
// 	if (PakPlatform->FileExists(*StandardFileName))
// 	{
// 		FString MountPoint = PakFile->GetMountPoint();
// 		UE_LOG(LogHotPatcher, Log, TEXT("Pak Mount Point %s"),*MountPoint);
//
// 		if (UFlibPakHelper::MountPak(*StandardFileName,10, MountPoint))
// 		{
// 			TArray<FString> MountedPakList = UFlibPakHelper::GetAllMountedPaks();
// 			UE_LOG(LogHotPatcher, Log, TEXT("Mounted Paks:"), *MountPoint);
// 			
// 			if (PakFile)
// 			{
// 				InDoSomething(PakFile.Get());;
// 			}
// 			bMounted = true;
// 		}
// 		else 
// 		{
// 			UE_LOG(LogHotPatcher, Log, TEXT("Mount Faild."));
// 		}
// 	}
// 	else
// 	{
// 		UE_LOG(LogHotPatcher, Log, TEXT("file %s is not found."),*StandardFileName);
// 	}
//
// 	if (bMounted)
// 	{
// 		bRunStatus = UFlibPakHelper::UnMountPak(*StandardFileName);
// 	}
//
// 	return bRunStatus;
// }
//
// bool UFlibPakHelper::LoadFilesByPak(const FString& InPakFile, TArray<FString>& OutFiles)
// {
// 	bool bRunStatus = false;
// 	TArray<FString> AllFiles;
// 	auto ScanAllFilesLambda = [&AllFiles](const FPakFile* InPakFileIns)->bool
// 	{
// 		bool bLambdaRunStatus = false;
// 		if (InPakFileIns)
// 		{
// 			InPakFileIns->FindFilesAtPath(AllFiles, *InPakFileIns->GetMountPoint(), true, false, true);
// 			bLambdaRunStatus = true;
// 		}
// 		return bLambdaRunStatus;
// 	};
// 	bRunStatus = UFlibPakHelper::LoadPakDoSomething(InPakFile, ScanAllFilesLambda);
// 	OutFiles = AllFiles;
// 	return bRunStatus;
// }
//
// bool UFlibPakHelper::LoadVersionInfoByPak(const FString& InPakFile, FPakVersion& OutVersion)
// {
// 	bool bRunStatus = false;
// 	
// 	TArray<FPakVersion> AllVersionInfo;
// 	auto ScanAllFilesLambda = [&AllVersionInfo](const FPakFile* InPakFile)->bool
// 	{
// 		bool bLambdaRunStatus = false;
// 		if (InPakFile)
// 		{
// 			// UE_LOG(LogHotPatcher, Log, TEXT("Scan All Files Lambda: InPakFile is not null."));
// 			TArray<FString> AllVersionDescribleFiles;
// 			FString PakMountPoint = InPakFile->GetMountPoint();
//
// 			InPakFile->FindFilesAtPath(AllVersionDescribleFiles, *InPakFile->GetMountPoint(), true, false, true);
//
// 			// UE_LOG(LogHotPatcher, Log, TEXT("Scan All Files Lambda:  FindFilesAtPath num is %d."),AllVersionDescribleFiles.Num());
//
// 			
// 			for (const auto& VersionDescribleFile : AllVersionDescribleFiles)
// 			{
// 				// UE_LOG(LogHotPatcher, Log, TEXT("Scan All Files Lambda: VersionDescrible File %s."), *VersionDescribleFile);
// 				if (!VersionDescribleFile.EndsWith(".json"))
// 					continue;
// 				FString CurrentVersionStr;
// 				if (FFileHelper::LoadFileToString(CurrentVersionStr,*VersionDescribleFile))
// 				{
// 					// UE_LOG(LogHotPatcher, Log, TEXT("Scan All Files Lambda: Load File Success."));
// 					FPakVersion CurrentPakVersionInfo;
// 					if (UFlibPakHelper::DeserializeStringToPakVersion(CurrentVersionStr, CurrentPakVersionInfo))
// 					{
// 						// UE_LOG(LogHotPatcher, Log, TEXT("Scan All Files Lambda: Deserialize File Success."));
// 						AllVersionInfo.Add(CurrentPakVersionInfo);
// 						bLambdaRunStatus = true;
// 					}
// 				}
// 			}
// 			
// 		}
// 		return bLambdaRunStatus;
// 	};
// 	bRunStatus = UFlibPakHelper::LoadPakDoSomething(InPakFile, ScanAllFilesLambda);
//
// 	if (bRunStatus && !!AllVersionInfo.Num())
// 	{
// 		OutVersion = AllVersionInfo[0];
// 	}
// 	return bRunStatus;
// }

int32 UFlibPakHelper::GetPakOrderByPakPath(const FString& PakFile)
{
	int32 PakOrder = 0;
    if (!PakFile.IsEmpty())
    {
    	FString PakFilename = PakFile;
    	if (PakFilename.EndsWith(TEXT("_P.pak")))
    	{
			// Prioritize based on the chunk version number
    		// Default to version 1 for single patch system
    		uint32 ChunkVersionNumber = 1;
    		FString StrippedPakFilename = PakFilename.LeftChop(6);
    		int32 VersionEndIndex = PakFilename.Find("_", ESearchCase::CaseSensitive, ESearchDir::FromEnd);
    		if (VersionEndIndex != INDEX_NONE && VersionEndIndex > 0)
    		{
    			int32 VersionStartIndex = PakFilename.Find("_", ESearchCase::CaseSensitive, ESearchDir::FromEnd, VersionEndIndex - 1);
    			if (VersionStartIndex != INDEX_NONE)
    			{
    				VersionStartIndex++;
    				FString VersionString = PakFilename.Mid(VersionStartIndex, VersionEndIndex - VersionStartIndex);
    				if (VersionString.IsNumeric())
    				{
    					int32 ChunkVersionSigned = FCString::Atoi(*VersionString);
    					if (ChunkVersionSigned >= 1)
    					{
    						// Increment by one so that the first patch file still gets more priority than the base pak file
    						ChunkVersionNumber = (uint32)ChunkVersionSigned + 1;
    					}
    				}
    			}
    		}
    		PakOrder += 100 * ChunkVersionNumber;
		}
    }
	return PakOrder;
}

bool UFlibPakHelper::LoadAssetRegistry(const FString& InAssetRegistryBin)
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
	bool bSuccess = false;
	FArrayReader SerializedAssetData;
	FString AssetRegistryBinPath = InAssetRegistryBin;
	UE_LOG(LogHotPatcher,Log,TEXT("Load AssetRegistry %s"),*AssetRegistryBinPath);
	if (IFileManager::Get().FileExists(*AssetRegistryBinPath) && FFileHelper::LoadFileToArray(SerializedAssetData, *AssetRegistryBinPath))
	{
		SerializedAssetData.Seek(0);
		AssetRegistry.Serialize(SerializedAssetData);
		bSuccess = true;
	}
	return bSuccess;
}

bool UFlibPakHelper::OpenPSO(const FString& Name)
{
	return FShaderPipelineCache::OpenPipelineFileCache(Name,GMaxRHIShaderPlatform);
}

FAES::FAESKey CachedAESKey;
#define AES_KEY_SIZE 32
bool ValidateEncryptionKey(TArray<uint8>& IndexData, const FSHAHash& InExpectedHash, const FAES::FAESKey& InAESKey)
{
	FAES::DecryptData(IndexData.GetData(), IndexData.Num(), InAESKey);

	// Check SHA1 value.
	FSHAHash ActualHash;
	FSHA1::HashBuffer(IndexData.GetData(), IndexData.Num(), ActualHash.Hash);
	return InExpectedHash == ActualHash;
}

bool PreLoadPak(const FString& InPakPath,const FString& AesKey)
{
	UE_LOG(LogHotPatcher, Log, TEXT("Pre load pak file: %s and check file hash."), *InPakPath);

	FArchive* Reader = IFileManager::Get().CreateFileReader(*InPakPath);
	if (!Reader)
	{
		return false;
	}

	FPakInfo Info;
	const int64 CachedTotalSize = Reader->TotalSize();
	bool bShouldLoad = false;
	int32 CompatibleVersion = FPakInfo::PakFile_Version_Latest;

	// Serialize trailer and check if everything is as expected.
	// start up one to offset the -- below
	CompatibleVersion++;
	int64 FileInfoPos = -1;
	do
	{
		// try the next version down
		CompatibleVersion--;

		FileInfoPos = CachedTotalSize - Info.GetSerializedSize(CompatibleVersion);
		if (FileInfoPos >= 0)
		{
			Reader->Seek(FileInfoPos);

			// Serialize trailer and check if everything is as expected.
			Info.Serialize(*Reader, CompatibleVersion);
			if (Info.Magic == FPakInfo::PakFile_Magic)
			{
				bShouldLoad = true;
			}
		}
	} while (!bShouldLoad && CompatibleVersion >= FPakInfo::PakFile_Version_Initial);

	if (!bShouldLoad)
	{
		Reader->Close();
		delete Reader;
		return false;
	}

	if (Info.EncryptionKeyGuid.IsValid() || Info.bEncryptedIndex)
	{
		const FString KeyString = AesKey;
		
		FAES::FAESKey AESKey;

		TArray<uint8> DecodedBuffer;
		if (!FBase64::Decode(KeyString, DecodedBuffer))
		{
			UE_LOG(LogHotPatcher, Error, TEXT("AES encryption key base64[%s] is not base64 format!"), *KeyString);
			bShouldLoad = false;
		}

		// Error checking
		if (bShouldLoad && DecodedBuffer.Num() != AES_KEY_SIZE)
		{
			UE_LOG(LogHotPatcher, Error, TEXT("AES encryption key base64[%s] can not decode to %d bytes long!"), *KeyString, AES_KEY_SIZE);
			bShouldLoad = false;
		}

		if (bShouldLoad)
		{
			FMemory::Memcpy(AESKey.Key, DecodedBuffer.GetData(), AES_KEY_SIZE);

			TArray<uint8> PrimaryIndexData;
			Reader->Seek(Info.IndexOffset);
			PrimaryIndexData.SetNum(Info.IndexSize);
			Reader->Serialize(PrimaryIndexData.GetData(), Info.IndexSize);

			if (!ValidateEncryptionKey(PrimaryIndexData, Info.IndexHash, AESKey))
			{
				UE_LOG(LogHotPatcher, Error, TEXT("AES encryption key base64[%s] is not correct!"), *KeyString);
				bShouldLoad = false;
			}
			else
			{
 				CachedAESKey = AESKey;

				UE_LOG(LogHotPatcher, Log, TEXT("Use AES encryption key base64[%s] for %s."), *KeyString, *InPakPath);
				FCoreDelegates::GetPakEncryptionKeyDelegate().BindLambda(
					[AESKey](uint8 OutKey[32])
					{
						FMemory::Memcpy(OutKey, AESKey.Key, 32);
					});

				if (Info.EncryptionKeyGuid.IsValid())
				{
#if ENGINE_MINOR_VERSION >= 26
					FCoreDelegates::GetRegisterEncryptionKeyMulticastDelegate().Broadcast(Info.EncryptionKeyGuid, AESKey);
#else
					FCoreDelegates::GetRegisterEncryptionKeyDelegate().ExecuteIfBound(Info.EncryptionKeyGuid, AESKey);
#endif
				}
			}
		}
	}

	Reader->Close();
	delete Reader;
	return bShouldLoad;
}

TArray<FString> UFlibPakHelper::GetPakFileList(const FString& InPak, const FString& AESKey)
{
	IPlatformFile* PlatformIns = &FPlatformFileManager::Get().GetPlatformFile();
	
	FString StandardFileName = InPak;
	FPaths::MakeStandardFilename(StandardFileName);
	TArray<FString> Records;
	if(PreLoadPak(StandardFileName,AESKey))
	{
#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION > 26
		TRefCountPtr<FPakFile> PakFile = new FPakFile(&PlatformIns->GetPlatformPhysical(), *StandardFileName, false);
#else
		TSharedPtr<FPakFile> PakFile = MakeShareable(new FPakFile(&PlatformIns->GetPlatformPhysical(), *StandardFileName, false));
#endif
		FString MountPoint = PakFile->GetMountPoint();
		
		for (FPakFile::FFileIterator It(*PakFile, true); It; ++It)
		{
#if ENGINE_MINOR_VERSION >= 26
			const FString& Filename = *It.TryGetFilename();
#else
			const FString& Filename = It.Filename();
#endif

			Records.Emplace(MountPoint + Filename);
		}
	}

	return Records;
}

TArray<FString> UFlibPakHelper::GetAllMountedPaks()
{
	FPakPlatformFile* PakPlatformFile = (FPakPlatformFile*)(FPlatformFileManager::Get().FindPlatformFile(FPakPlatformFile::GetTypeName()));
	
	TArray<FString> Resault;
	if(PakPlatformFile)
		PakPlatformFile->GetMountedPakFilenames(Resault);
	else
		UE_LOG(LogHotPatcher,Warning,TEXT("PakPlatformFile is invalid!"));
	return Resault;
}

