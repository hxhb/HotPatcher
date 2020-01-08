#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "FExternDirectoryInfo.generated.h"

USTRUCT(BlueprintType)
struct FExternDirectoryInfo
{
	GENERATED_USTRUCT_BODY()

public:
	FORCEINLINE FExternDirectoryInfo():MountPoint(FPaths::Combine(TEXT("../../.."),FApp::GetProjectName())){}
	FExternDirectoryInfo(const FExternDirectoryInfo&) = default;
	FExternDirectoryInfo& operator=(const FExternDirectoryInfo&) = default;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BaseVersion")
		FDirectoryPath DirectoryPath;
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
		FString MountPoint = TEXT("../../../");


	bool operator==(const FExternDirectoryInfo& Right)const
	{
		bool bIsSamePath = (DirectoryPath.Path == Right.DirectoryPath.Path);
		bool bIsSameMountPath = (MountPoint == Right.MountPoint);
		return bIsSamePath && bIsSameMountPath;
	}
};
