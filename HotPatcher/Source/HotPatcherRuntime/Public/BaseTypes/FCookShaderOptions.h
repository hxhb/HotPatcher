#pragma once

#include "CoreMinimal.h"
#include "FCookShaderOptions.generated.h"


UENUM(BlueprintType)
enum class EShaderLibNameRule : uint8
{
	CHUNK_NAME,
	PROJECT_NAME,
	CUSTOM
};

#define AS_PROJECTDIR_MARK TEXT("[PROJECTDIR]")

USTRUCT(BlueprintType)
struct FCookShaderOptions
{
	GENERATED_BODY()
	FCookShaderOptions()
	{
		ShderLibMountPointRegular = FString::Printf(TEXT("%s/ShaderLibs"),AS_PROJECTDIR_MARK);
	}
	FString GetShaderLibMountPointRegular()const { return ShderLibMountPointRegular; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bSharedShaderLibrary = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bNativeShader = false;
	// metallib and metalmap to pak?
	bool bNativeShaderToPak = false;
	// if name is StartContent to ShaderArchive-StarterContent-PCD3D_SM5.ushaderbytecode
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EShaderLibNameRule ShaderNameRule = EShaderLibNameRule::CHUNK_NAME;
	UPROPERTY(EditAnywhere, BlueprintReadWrite,meta=(EditCondition="ShaderNameRule==EShaderLibNameRule::CUSTOM"))
	FString CustomShaderName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ShderLibMountPointRegular;
};