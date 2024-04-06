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
struct HOTPATCHERRUNTIME_API FExternFileInfo
{
	GENERATED_USTRUCT_BODY()

public:
	FORCEINLINE FExternFileInfo():MountPath(FPaths::Combine(TEXT("../../.."),FApp::GetProjectName())){}
	FExternFileInfo(const FExternFileInfo&) = default;
	FExternFileInfo& operator=(const FExternFileInfo&) = default;

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
		bool bIsSamePath = (GetFilePath() == Right.GetFilePath());
		bool IsSameHash = (FileHash == Right.FileHash);
		return (*this == Right) && bIsSamePath &&  IsSameHash;
	}
	
	FString GenerateFileHash(EHashCalculator HashCalculator = EHashCalculator::MD5);
	FString GetFileHash(EHashCalculator HashCalculator = EHashCalculator::MD5)const;
	FString GetReplaceMarkdFilePath()const;
	FString GetFilePath()const{ return FilePath.FilePath; }
	void SetFilePath(const FString& InFilePath);
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BaseVersion", meta = (RelativeToGameContentDir))
		FFilePath FilePath;
public:
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
		FString MountPath = TEXT("../../../");
	UPROPERTY()
		FString FileHash;
		EPatchAssetType Type = EPatchAssetType::None;


};

