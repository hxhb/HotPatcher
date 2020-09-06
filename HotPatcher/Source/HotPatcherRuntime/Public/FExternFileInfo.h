#pragma once

#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "FExternFileInfo.generated.h"

USTRUCT(BlueprintType)
struct FExternFileInfo
{
	GENERATED_USTRUCT_BODY()

public:
	FORCEINLINE FExternFileInfo():MountPath(FPaths::Combine(TEXT("../../.."),FApp::GetProjectName())){}
	FExternFileInfo(const FExternFileInfo&) = default;
	FExternFileInfo& operator=(const FExternFileInfo&) = default;

	FORCEINLINE FString GenerateFileHash()
	{
		FileHash = GetFileHash();
		return FileHash;
	}

	FORCEINLINE FString GetFileHash()const
	{
		FString HashValue;
		FString FileAbsPath = FPaths::ConvertRelativePathToFull(FilePath.FilePath);
		if (FPaths::FileExists(FileAbsPath))
		{
			FMD5Hash FileMD5Hash = FMD5Hash::HashFile(*FileAbsPath);
			HashValue = LexToString(FileMD5Hash);
		}
		return HashValue;
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BaseVersion", meta = (RelativeToGameContentDir))
		FFilePath FilePath;
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
		FString MountPath = TEXT("../../../");
	UPROPERTY()
		FString FileHash;

	bool operator==(const FExternFileInfo& Right)const
	{
		
		bool bIsSameMountPath = (MountPath == Right.MountPath);
		
		return bIsSameMountPath;
	}
	bool IsAbsSame(const FExternFileInfo& Right)const
	{
		bool bIsSamePath = (FilePath.FilePath == Right.FilePath.FilePath);
		bool IsSameHash = (FileHash == Right.FileHash);
		return (*this == Right) && bIsSamePath &&  IsSameHash;
	}

};
