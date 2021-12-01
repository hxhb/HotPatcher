#include "Cooker/MultiCooker/FCookShaderCollectionProxy.h"
#include "FlibHotPatcherEditorHelper.h"
#include "ShaderPatch/FlibShaderCodeLibraryHelper.h"

FCookShaderCollectionProxy::FCookShaderCollectionProxy(const TArray<FString>& InPlatformNames,const FString& InLibraryName,bool bInShareShader,bool InIsNative,bool bInMaster,const FString& InSaveBaseDir)
:PlatformNames(InPlatformNames),LibraryName(InLibraryName),bShareShader(bInShareShader),bIsNative(InIsNative),bMaster(bInMaster),SaveBaseDir(InSaveBaseDir){}

FCookShaderCollectionProxy::~FCookShaderCollectionProxy(){}

void FCookShaderCollectionProxy::Init()
{
	if(bShareShader)
	{
		SHADER_COOKER_CLASS::InitForCooking(bIsNative);
		for(const auto& PlatformName:PlatformNames)
		{
			ITargetPlatform* TargetPlatform = UFlibHotPatcherEditorHelper::GetPlatformByName(PlatformName);
			TargetPlatforms.AddUnique(TargetPlatform);
			TArray<FName> ShaderFormats = UFlibShaderCodeLibraryHelper::GetShaderFormatsByTargetPlatform(TargetPlatform);
			if (ShaderFormats.Num() > 0)
			{
	#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION > 25
				TArray<SHADER_COOKER_CLASS::FShaderFormatDescriptor> ShaderFormatsWithStableKeys = UFlibShaderCodeLibraryHelper::GetShaderFormatsWithStableKeys(ShaderFormats);
	#else
				TArray<TPair<FName, bool>> ShaderFormatsWithStableKeys;
				for (FName& Format : ShaderFormats)
				{
					ShaderFormatsWithStableKeys.Push(MakeTuple(Format, true));
				}
	#endif
				SHADER_COOKER_CLASS::CookShaderFormats(ShaderFormatsWithStableKeys);
			}
		}
		FShaderCodeLibrary::OpenLibrary(LibraryName,TEXT(""));
	}
}

bool IsAppleMetalPlatform(ITargetPlatform* TargetPlatform)
{
	bool bIsMatched = false;
	TArray<FString> ApplePlatforms = {TEXT("IOS"),TEXT("Mac"),TEXT("TVOS")};
	for(const auto& Platform:ApplePlatforms)
	{
		if(TargetPlatform->PlatformName().StartsWith(Platform,ESearchCase::IgnoreCase))
		{
			bIsMatched = true;
			break;
		}
	}
	return bIsMatched;
}

void FCookShaderCollectionProxy::Shutdown()
{
	if(bShareShader)
	{
		for(const auto& TargetPlatform:TargetPlatforms)
		{
			FString PlatformName = TargetPlatform->PlatformName();
			bSuccessed = UFlibShaderCodeLibraryHelper::SaveShaderLibrary(TargetPlatform, NULL, LibraryName,SaveBaseDir,bMaster);
#if ENGINE_MAJOR_VERSION < 5 && ENGINE_MINOR_VERSION <= 26
			if(bIsNative)
			{
				FString ShaderCodeDir = FPaths::Combine(SaveBaseDir,PlatformName);
				bSuccessed = bSuccessed && FShaderCodeLibrary::PackageNativeShaderLibrary(ShaderCodeDir,UFlibShaderCodeLibraryHelper::GetShaderFormatsByTargetPlatform(TargetPlatform));
			}
#endif
			// rename StarterContent_SF_METAL.0.metallib to startercontent_sf_metal.0.metallib
			if(bSuccessed && bIsNative && IsAppleMetalPlatform(TargetPlatform))
			{
				TArray<FString> FoundShaderLibs = UFlibShaderCodeLibraryHelper::FindCookedShaderLibByPlatform(PlatformName,FPaths::Combine(SaveBaseDir,TargetPlatform->PlatformName()));
				for(const auto& Shaderlib:FoundShaderLibs)
				{
					if(Shaderlib.EndsWith(TEXT("metallib"),ESearchCase::IgnoreCase) || Shaderlib.EndsWith(TEXT("metalmap"),ESearchCase::IgnoreCase))
					{
						FString Path = FPaths::GetPath(Shaderlib);
						FString Name = FPaths::GetBaseFilename(Shaderlib,true);
						FString Extersion = FPaths::GetExtension(Shaderlib,true);
						Name = FString::Printf(TEXT("%s%s"),*Name,*Extersion);
						Name.ToLowerInline();
						if(!Shaderlib.EndsWith(Name,ESearchCase::CaseSensitive))
						{
							IFileManager::Get().Move(*FPaths::Combine(Path,Name),*Shaderlib,false);
						}
					}
				}
			}
		}
		FShaderCodeLibrary::CloseLibrary(LibraryName);
		FShaderCodeLibrary::Shutdown();
	}
}
