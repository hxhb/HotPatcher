// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "IPlatformFilePak.h"
#include "CoreMinimal.h"

#include "Misc/FileHelper.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FlibPakReader.generated.h"

/**
 * 
 */
UCLASS()
class HOTPATCHERRUNTIME_API UFlibPakReader : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable)
	static TArray<FString> GetPakFileList(const FString& PakFilePath);
	static FPakFile* GetPakFileInsByPath(const FString& PakPath);
	static bool FindFileInPakFile(FPakFile* InPakFile, const FString& InFileName, FPakEntry* OutPakEntry);
	static IFileHandle* CreatePakFileHandle(IPlatformFile* InLowLevel, FPakFile* PakFile, const FPakEntry* FileEntry);
	static bool LoadFileToString(FString& Result, FArchive* InReader, const TCHAR* Filename, FFileHelper::EHashOptions VerifyFlags = FFileHelper::EHashOptions::None);

#if PLATFORM_WINDOWS
	static FArchive* CreatePakReader(FPakFile* InPakFile, IFileHandle& InHandle, const TCHAR* InFilename);
	static FString LoadPakFileToString(const FString& InPakFile,const FString& InFileName);
#endif
};
