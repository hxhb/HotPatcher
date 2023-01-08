#include "Cooker/MultiCooker/FCookShaderCollectionProxy.h"
#include "FlibHotPatcherCoreHelper.h"
#include "Cooker/MultiCooker/FlibHotCookerHelper.h"
#include "ShaderLibUtils//FlibShaderCodeLibraryHelper.h"
#include "Interfaces/ITargetPlatform.h"
#include "Resources/Version.h"

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
			ITargetPlatform* TargetPlatform = UFlibHotPatcherCoreHelper::GetPlatformByName(PlatformName);
			TargetPlatforms.AddUnique(TargetPlatform);
			TArray<FName> ShaderFormats = UFlibShaderCodeLibraryHelper::GetShaderFormatsByTargetPlatform(TargetPlatform);
			if (ShaderFormats.Num() > 0)
			{
	#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION > 25
				TArray<SHADER_COOKER_CLASS::FShaderFormatDescriptor> ShaderFormatsWithStableKeys = UFlibShaderCodeLibraryHelper::GetShaderFormatsWithStableKeys(ShaderFormats);
	#else
		#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION > 22
						TArray<TPair<FName, bool>> ShaderFormatsWithStableKeys;
						for (FName& Format : ShaderFormats)
						{
							ShaderFormatsWithStableKeys.Push(MakeTuple(Format, true));
						}
		#else
						TArray<FName> ShaderFormatsWithStableKeys =  ShaderFormats;
		#endif
	#endif
				SHADER_COOKER_CLASS::CookShaderFormats(ShaderFormatsWithStableKeys);
			}
		}
		FShaderCodeLibrary::OpenLibrary(LibraryName,TEXT(""));
	}
}



void FCookShaderCollectionProxy::Shutdown()
{
	SCOPED_NAMED_EVENT_TEXT("FCookShaderCollectionProxy::Shutdown",FColor::Red);
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
			if(bSuccessed && bIsNative && UFlibHotCookerHelper::IsAppleMetalPlatform(TargetPlatform))
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
