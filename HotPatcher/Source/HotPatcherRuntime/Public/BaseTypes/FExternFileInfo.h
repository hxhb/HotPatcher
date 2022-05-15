#pragma once
#include "HotPatcherBaseTypes.h"
// engine header
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "FExternFileInfo.generated.h"


UENUM()
enum class EHashCalculator
{
	NoHash,
	MD5,
	SHA1
};

USTRUCT(BlueprintType)
struct FExternFileInfo
{
	GENERATED_USTRUCT_BODY()

public:
	FORCEINLINE FExternFileInfo():MountPath(FPaths::Combine(TEXT("../../.."),FApp::GetProjectName())){}
	FExternFileInfo(const FExternFileInfo&) = default;
	FExternFileInfo& operator=(const FExternFileInfo&) = default;

	FString GenerateFileHash(EHashCalculator HashCalculator = EHashCalculator::MD5);
	FString GetFileHash(EHashCalculator HashCalculator = EHashCalculator::MD5)const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BaseVersion", meta = (RelativeToGameContentDir))
		FFilePath FilePath;
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
		FString MountPath = TEXT("../../../");
	UPROPERTY()
		FString FileHash;

		EPatchAssetType Type = EPatchAssetType::None;

	bool operator==(const FExternFileInfo& Right)const
	{
		return MountPath == Right.MountPath;
	}

	// ignore FilePath abs path (only mount path and filehash)
	bool IsSameMount(const FExternFileInfo& Right)const
	{
		bool IsSameHash = (FileHash == Right.FileHash);
		return (*this == Right) &&  IsSameHash;
	}
	
	bool IsAbsSame(const FExternFileInfo& Right)const
	{
		bool bIsSamePath = (FilePath.FilePath == Right.FilePath.FilePath);
		bool IsSameHash = (FileHash == Right.FileHash);
		return (*this == Right) && bIsSamePath &&  IsSameHash;
	}

};
