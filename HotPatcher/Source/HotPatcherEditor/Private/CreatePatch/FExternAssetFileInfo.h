#pragma once

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BaseVersion", meta = (RelativeToGameContentDir))
		FFilePath FileAbsPath;
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
		FString MountPath = TEXT("../../../");


	bool operator==(const FExternAssetFileInfo& Right)const
	{
		bool bIsSamePath = (FileAbsPath.FilePath == Right.FileAbsPath.FilePath);
		bool bIsSameMountPath = (MountPath == Right.MountPath);
		return bIsSamePath && bIsSameMountPath;
	}

};
