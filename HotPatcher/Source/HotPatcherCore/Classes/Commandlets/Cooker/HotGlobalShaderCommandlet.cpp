#include "HotGlobalShaderCommandlet.h"
#include "CommandletHelper.h"
// engine header
#include "CoreMinimal.h"
#include "FlibHotPatcherCoreHelper.h"
#include "Async/ParallelFor.h"
#include "Cooker/MultiCooker/FlibHotCookerHelper.h"
#include "Kismet/KismetStringLibrary.h"
#include "Misc/FileHelper.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "ShaderLibUtils/FlibShaderCodeLibraryHelper.h"

#define PLATFORMS_PARAM_NAME TEXT("-platforms=")
#define SAVETO_PARAM_NAME TEXT("-saveto=")

DEFINE_LOG_CATEGORY(LogHotGlobalShaderCommandlet);

int32 UHotGlobalShaderCommandlet::Main(const FString& Params)
{
	SCOPED_NAMED_EVENT_TEXT("UHotGlobalShaderCommandlet::Main",FColor::Red);
	Super::Main(Params);

	UE_LOG(LogHotGlobalShaderCommandlet, Display, TEXT("UHotGlobalShaderCommandlet::Main"));

	FString PlatformNameStr;
	bool bStatus = FParse::Value(*Params, *FString(PLATFORMS_PARAM_NAME).ToLower(), PlatformNameStr);
	if (!bStatus)
	{
		UE_LOG(LogHotGlobalShaderCommandlet, Warning, TEXT("not -platforms=Android_ASTC+IOS params."));
		return -1;
	}

	FString SaveShaderToDir;
	if (!FParse::Value(*Params, *FString(SAVETO_PARAM_NAME).ToLower(), SaveShaderToDir))
	{
		SaveShaderToDir = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Cooked")));
	}
	
	TArray<FString> PlatformNames = UKismetStringLibrary::ParseIntoArray(PlatformNameStr,TEXT("+"),false);
	TArray<ETargetPlatform> Platforms;
	for(const auto& PlatformName:PlatformNames)
	{
		ETargetPlatform Platform;
		if(THotPatcherTemplateHelper::GetEnumValueByName(PlatformName,Platform))
		{
			Platforms.AddUnique(Platform);
		}
	}

	UE_LOG(LogHotGlobalShaderCommandlet,Display,TEXT("Compile Global Shader for %s, save to %s"),*PlatformNameStr,*SaveShaderToDir);
	
	{
		SCOPED_NAMED_EVENT_TEXT("Compile Global Shader",FColor::Red);
		FString Name = TEXT("Global");
		auto GlobalShaderCollectionProxy = UFlibHotCookerHelper::CreateCookShaderCollectionProxyByPlatform(
			Name,
			Platforms,
			true,
			true,
			true,
			SaveShaderToDir,
			true
			);
		if(GlobalShaderCollectionProxy.IsValid())
		{
			GlobalShaderCollectionProxy->Init();
		}
		// compile Global Shader
		UFlibHotPatcherCoreHelper::SaveGlobalShaderMapFiles(UFlibHotPatcherCoreHelper::GetTargetPlatformsByNames(Platforms),SaveShaderToDir);

		UFlibShaderCodeLibraryHelper::WaitShaderCompilingComplete();
		UFlibHotPatcherCoreHelper::WaitForAsyncFileWrites();
		if(GlobalShaderCollectionProxy.IsValid())
		{
			GlobalShaderCollectionProxy->Shutdown();
		}
	}
	
	UE_LOG(LogHotGlobalShaderCommandlet,Display,TEXT("HotGlobalShader Misstion is Finished!"));
	
	if(FParse::Param(FCommandLine::Get(), TEXT("wait")))
	{
		system("pause");
	}
	
	return 0;
}
