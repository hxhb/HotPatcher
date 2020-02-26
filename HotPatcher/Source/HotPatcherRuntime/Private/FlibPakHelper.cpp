// Fill out your copyright notice in the Description page of Project Settings.


#include "FlibPakHelper.h"
#include "IPlatformFilePak.h"
#include "HAL/PlatformFilemanager.h"
#include "AssetManager/FFileArrayDirectoryVisitor.hpp"

// Engine Header
#include "Misc/ScopeExit.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Misc/FileHelper.h"

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
		UE_LOG(LogTemp, Log, TEXT("GetPlatformFile(TEXT(\"PakFile\") is NULL"));
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
			UE_LOG(LogTemp, Log, TEXT("Mounted = %s, Order = %d, MountPoint = %s"), *PakPath, PakOrder, !MountPoint ? TEXT("(NULL)") : MountPoint);
			bMounted = true;
		}
		else {
			UE_LOG(LogTemp, Error, TEXT("Faild to mount pak = %s"), *PakPath);
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
		UE_LOG(LogTemp, Log, TEXT("GetPlatformFile(TEXT(\"PakFile\") is NULL"));
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
		UE_LOG(LogTemp, Log, TEXT("CreateFileByBytes InFile is Empty!"));
		// UE_LOG(LogTemp, Log, TEXT("CreateFileByBytes file %s existing!"), *InFile);
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

		FFillArrayDirectoryVisitor FallArrayDirVisitor;
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
		UE_LOG(LogTemp, Warning, TEXT("Directory %s is not found."), *InRelativePath);
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
		UE_LOG(LogTemp, Warning, TEXT("UFlibPakHelper::ScanPlatformDirectory return false."));
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

bool UFlibPakHelper::LoadPakDoSomething(const FString& InPakFile, TFunction<bool(const FPakFile*)> InDoSomething)
{
	bool bRunStatus = false;
	bool bMounted = false;
	FPakPlatformFile* PakPlatform = (FPakPlatformFile*)&FPlatformFileManager::Get().GetPlatformFile();

	FString StandardFileName = InPakFile;
	FPaths::MakeStandardFilename(StandardFileName);

	TSharedPtr<FPakFile> PakFile = MakeShareable(new FPakFile(PakPlatform->GetLowerLevel(), *StandardFileName, false));

	if (PakPlatform->FileExists(*StandardFileName))
	{
		FString MountPoint = PakFile->GetMountPoint();
		UE_LOG(LogTemp, Log, TEXT("Pak Mount Point %s"),*MountPoint);

		if (UFlibPakHelper::MountPak(*StandardFileName,10, MountPoint))
		{
			TArray<FString> MountedPakList = UFlibPakHelper::GetAllMountedPaks();
			UE_LOG(LogTemp, Log, TEXT("Mounted Paks:"), *MountPoint);
			
			if (PakFile)
			{
				InDoSomething(PakFile.Get());;
			}
			bMounted = true;
		}
		else 
		{
			UE_LOG(LogTemp, Log, TEXT("Mount Faild."));
		}
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("file %s is not found."),*StandardFileName);
	}

	if (bMounted)
	{
		bRunStatus = UFlibPakHelper::UnMountPak(*StandardFileName);
	}

	return bRunStatus;
}

bool UFlibPakHelper::LoadFilesByPak(const FString& InPakFile, TArray<FString>& OutFiles)
{
	bool bRunStatus = false;
	TArray<FString> AllFiles;
	auto ScanAllFilesLambda = [&AllFiles](const FPakFile* InPakFileIns)->bool
	{
		bool bLambdaRunStatus = false;
		if (InPakFileIns)
		{
			InPakFileIns->FindFilesAtPath(AllFiles, *InPakFileIns->GetMountPoint(), true, false, true);
			bLambdaRunStatus = true;
		}
		return bLambdaRunStatus;
	};
	bRunStatus = UFlibPakHelper::LoadPakDoSomething(InPakFile, ScanAllFilesLambda);
	OutFiles = AllFiles;
	return bRunStatus;
}

bool UFlibPakHelper::LoadVersionInfoByPak(const FString& InPakFile, FPakVersion& OutVersion)
{
	bool bRunStatus = false;
	
	TArray<FPakVersion> AllVersionInfo;
	auto ScanAllFilesLambda = [&AllVersionInfo](const FPakFile* InPakFile)->bool
	{
		bool bLambdaRunStatus = false;
		if (InPakFile)
		{
			// UE_LOG(LogTemp, Log, TEXT("Scan All Files Lambda: InPakFile is not null."));
			TArray<FString> AllVersionDescribleFiles;
			FString PakMountPoint = InPakFile->GetMountPoint();

			InPakFile->FindFilesAtPath(AllVersionDescribleFiles, *InPakFile->GetMountPoint(), true, false, true);

			// UE_LOG(LogTemp, Log, TEXT("Scan All Files Lambda:  FindFilesAtPath num is %d."),AllVersionDescribleFiles.Num());

			
			for (const auto& VersionDescribleFile : AllVersionDescribleFiles)
			{
				// UE_LOG(LogTemp, Log, TEXT("Scan All Files Lambda: VersionDescrible File %s."), *VersionDescribleFile);
				if (!VersionDescribleFile.EndsWith(".json"))
					continue;
				FString CurrentVersionStr;
				if (FFileHelper::LoadFileToString(CurrentVersionStr,*VersionDescribleFile))
				{
					// UE_LOG(LogTemp, Log, TEXT("Scan All Files Lambda: Load File Success."));
					FPakVersion CurrentPakVersionInfo;
					if (UFlibPakHelper::DeserializeStringToPakVersion(CurrentVersionStr, CurrentPakVersionInfo))
					{
						// UE_LOG(LogTemp, Log, TEXT("Scan All Files Lambda: Deserialize File Success."));
						AllVersionInfo.Add(CurrentPakVersionInfo);
						bLambdaRunStatus = true;
					}
				}
			}
			
		}
		return bLambdaRunStatus;
	};
	bRunStatus = UFlibPakHelper::LoadPakDoSomething(InPakFile, ScanAllFilesLambda);

	if (bRunStatus && !!AllVersionInfo.Num())
	{
		OutVersion = AllVersionInfo[0];
	}
	return bRunStatus;
}

TArray<FString> UFlibPakHelper::GetAllMountedPaks()
{
	FPakPlatformFile* PakPlatformFile = (FPakPlatformFile*)&FPlatformFileManager::Get().GetPlatformFile();

	TArray<FString> Resault;
	PakPlatformFile->GetMountedPakFilenames(Resault);

	return Resault;
}

