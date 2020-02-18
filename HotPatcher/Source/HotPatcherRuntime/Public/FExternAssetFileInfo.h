#pragma once

#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "FExternAssetFileInfo.generated.h"

USTRUCT(BlueprintType)
struct FExternAssetFileInfo
{
	GENERATED_USTRUCT_BODY()

public:
	FORCEINLINE FExternAssetFileInfo():MountPath(FPaths::Combine(TEXT("../../.."),FApp::GetProjectName())){}
	FExternAssetFileInfo(const FExternAssetFileInfo&) = default;
	FExternAssetFileInfo& operator=(const FExternAssetFileInfo&) = default;

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

	bool operator==(const FExternAssetFileInfo& Right)const
	{
		bool bIsSamePath = (FilePath.FilePath == Right.FilePath.FilePath);
		bool bIsSameMountPath = (MountPath == Right.MountPath);
		bool IsSameHash = (FileHash == Right.FileHash);
		return bIsSamePath && bIsSameMountPath && IsSameHash;
	}

};
