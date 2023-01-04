// Fill out your copyright notice in the Description page of Project Settings.


#include "ShaderPatch/FlibShaderPatchHelper.h"
#include "ETargetPlatform.h"
#include "FlibHotPatcherCoreHelper.h"
#include "Misc/Paths.h"
#include "ShaderCodeLibrary.h"
#include "Settings/ProjectPackagingSettings.h"


bool UFlibShaderPatchHelper::CreateShaderCodePatch(TArray<FString> const& OldMetaDataDirs, FString const& NewMetaDataDir, FString const& OutDir, bool bNativeFormat,bool bDeterministicShaderCodeOrder)
{
#if	ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION > 26
	return FShaderLibraryCooker::CreatePatchLibrary(OldMetaDataDirs,NewMetaDataDir,OutDir,bNativeFormat,bDeterministicShaderCodeOrder);
#else
	#if ENGINE_MINOR_VERSION > 25
		return FShaderCodeLibrary::CreatePatchLibrary(OldMetaDataDirs,NewMetaDataDir,OutDir,bNativeFormat,bDeterministicShaderCodeOrder);
	#else
		#if ENGINE_MINOR_VERSION > 23
			return FShaderCodeLibrary::CreatePatchLibrary(OldMetaDataDirs,NewMetaDataDir,OutDir,bNativeFormat);
		#else
			return false;
		#endif
	#endif
#endif
}

TArray<FString> UFlibShaderPatchHelper::ConvDirectoryPathToStr(const TArray<FDirectoryPath>& Dirs)
{
	TArray<FString> result;
	for(const auto& Dir:Dirs)
	{
		FString Path = FPaths::ConvertRelativePathToFull(Dir.Path);
		if(FPaths::DirectoryExists(Path))
		{
			result.AddUnique(Path);
		}
	}
	return result;
}

FString UFlibShaderPatchHelper::GetShaderStableInfoFileNameByShaderArchiveFileName(const FString& ShaderArchiveFileName)
{
	FString ShaderInfoFileName = ShaderArchiveFileName;
	ShaderInfoFileName.RemoveFromStart(TEXT("ShaderArchive-"));
	ShaderInfoFileName.RemoveFromEnd(TEXT(".ushaderbytecode"));
	ShaderInfoFileName = FString::Printf(TEXT("ShaderStableInfo-%s.scl.csv"),*ShaderInfoFileName);
	return ShaderInfoFileName;
}

FString UFlibShaderPatchHelper::GetShaderInfoFileNameByShaderArchiveFileName(const FString& ShaderArchiveFileName)
{
	FString ShaderInfoFileName = ShaderArchiveFileName;
	ShaderInfoFileName.RemoveFromStart(TEXT("ShaderArchive-"));
	ShaderInfoFileName.RemoveFromEnd(TEXT(".ushaderbytecode"));
	ShaderInfoFileName = FString::Printf(TEXT("ShaderAssetInfo-%s.assetinfo.json"),*ShaderInfoFileName);
	return ShaderInfoFileName;
}



// void UFlibShaderPatchHelper::InitShaderCodeLibrary(const TArray<ETargetPlatform>& Platforms)
// {
//     const UProjectPackagingSettings* const PackagingSettings = GetDefault<UProjectPackagingSettings>();
// 	// IsUsingShaderCodeLibrary();
// 	// CurrentCookMode == ECookMode::CookByTheBookFromTheEditor || CurrentCookMode == ECookMode::CookByTheBook;
// 	bool const bCacheShaderLibraries = true;
//     if (bCacheShaderLibraries && PackagingSettings->bShareMaterialShaderCode)
//     {
//         FShaderCodeLibrary::InitForCooking(PackagingSettings->bSharedMaterialNativeLibraries);
//         
// 		bool bAllPlatformsNeedStableKeys = false;
// 		// support setting without Hungarian prefix for the compatibility, but allow newer one to override
// 		GConfig->GetBool(TEXT("DevOptions.Shaders"), TEXT("NeedsShaderStableKeys"), bAllPlatformsNeedStableKeys, GEngineIni);
// 		GConfig->GetBool(TEXT("DevOptions.Shaders"), TEXT("bNeedsShaderStableKeys"), bAllPlatformsNeedStableKeys, GEngineIni);
//
//         for (const ITargetPlatform* TargetPlatform : UFlibHotPatcherCoreHelper::GetTargetPlatformsByNames(Platforms))
//         {
// 			// Find out if this platform requires stable shader keys, by reading the platform setting file.
// 			bool bNeedShaderStableKeys = bAllPlatformsNeedStableKeys;
// 			FConfigFile PlatformIniFile;
// 			FConfigCacheIni::LoadLocalIniFile(PlatformIniFile, TEXT("Engine"), true, *TargetPlatform->IniPlatformName());
// 			PlatformIniFile.GetBool(TEXT("DevOptions.Shaders"), TEXT("NeedsShaderStableKeys"), bNeedShaderStableKeys);
// 			PlatformIniFile.GetBool(TEXT("DevOptions.Shaders"), TEXT("bNeedsShaderStableKeys"), bNeedShaderStableKeys);
//
// 			bool bNeedsDeterministicOrder = PackagingSettings->bDeterministicShaderCodeOrder;
// 			FConfigFile PlatformGameIniFile;
// 			FConfigCacheIni::LoadLocalIniFile(PlatformGameIniFile, TEXT("Game"), true, *TargetPlatform->IniPlatformName());
// 			PlatformGameIniFile.GetBool(TEXT("/Script/UnrealEd.ProjectPackagingSettings"), TEXT("bDeterministicShaderCodeOrder"), bNeedsDeterministicOrder);
//
//             TArray<FName> ShaderFormats;
//             TargetPlatform->GetAllTargetedShaderFormats(ShaderFormats);
// 			TArray<FShaderCodeLibrary::FShaderFormatDescriptor> ShaderFormatsWithStableKeys;
// 			for (FName& Format : ShaderFormats)
// 			{
// 				FShaderCodeLibrary::FShaderFormatDescriptor NewDesc;
// 				NewDesc.ShaderFormat = Format;
// 				NewDesc.bNeedsStableKeys = bNeedShaderStableKeys;
// 				NewDesc.bNeedsDeterministicOrder = bNeedsDeterministicOrder;
// 				ShaderFormatsWithStableKeys.Push(NewDesc);
// 			}
//
//             if (ShaderFormats.Num() > 0)
// 			{
// 				FShaderCodeLibrary::CookShaderFormats(ShaderFormatsWithStableKeys);
// 			}
//         }
//     }
// }


// void UFlibShaderPatchHelper::CleanShaderCodeLibraries(const TArray<ETargetPlatform>& Platforms)
// {
// 	const UProjectPackagingSettings* const PackagingSettings = GetDefault<UProjectPackagingSettings>();
// 	bool const bCacheShaderLibraries = true;// IsUsingShaderCodeLibrary();
// 	ITargetPlatformManagerModule& TPM = GetTargetPlatformManagerRef();
// 	bool bIterativeCook = true;// IsCookFlagSet(ECookInitializationFlags::Iterative) ||	PackageDatas->GetNumCooked() != 0;
//
// 	// If not iterative then clean up our temporary files
// 	if (bCacheShaderLibraries && PackagingSettings->bShareMaterialShaderCode && !bIterativeCook)
// 	{
// 		for (const ITargetPlatform* TargetPlatform : UFlibHotPatcherCoreHelper::GetTargetPlatformsByNames(Platforms))
// 		{
// 			TArray<FName> ShaderFormats;
// 			TargetPlatform->GetAllTargetedShaderFormats(ShaderFormats);
// 			if (ShaderFormats.Num() > 0)
// 			{
// 				FShaderCodeLibrary::CleanDirectories(ShaderFormats);
// 			}
// 		}
// 	}
// }
//
// FString ConvertToFullSandboxPath( const FString &FileName, bool bForWrite, const FString& PlatformName )
// {
// 	FString Result = FPaths::ConvertRelativePathToFull(FileName);//ConvertToFullSandboxPath( FileName, bForWrite );
// 	Result.ReplaceInline(TEXT("[Platform]"), *PlatformName);
// 	return Result;
// }
//
// static FString GenerateShaderCodeLibraryName(FString const& Name, bool bIsIterateSharedBuild)
// {
// 	FString ActualName = (!bIsIterateSharedBuild) ? Name : Name + TEXT("_SC");
// 	return ActualName;
// }

// void UFlibShaderPatchHelper::SaveShaderLibrary(const ITargetPlatform* TargetPlatform, FString const& Name, const TArray<TSet<FName>>* ChunkAssignments)
// {
// 	bool bIsIterateSharedBuild = true; // IsCookFlagSet(ECookInitializationFlags::IterateSharedBuild)
// 	FString ActualName = GenerateShaderCodeLibraryName(Name, bIsIterateSharedBuild);
// 	FString BasePath = FPaths::ProjectContentDir();
//
// 	FString ShaderCodeDir = ConvertToFullSandboxPath(*BasePath, true, TargetPlatform->PlatformName());
//
// 	const FString RootMetaDataPath = FPaths::ProjectDir() / TEXT("Metadata") / TEXT("PipelineCaches");
// 	const FString MetaDataPathSB = FPaths::ConvertRelativePathToFull(*RootMetaDataPath);
// 	const FString MetaDataPath = MetaDataPathSB.Replace(TEXT("[Platform]"), *TargetPlatform->PlatformName());
//
// 	// note that shader formats can be shared across the target platforms
// 	TArray<FName> ShaderFormats;
// 	TargetPlatform->GetAllTargetedShaderFormats(ShaderFormats);
// 	if (ShaderFormats.Num() > 0)
// 	{
// 		FString TargetPlatformName = TargetPlatform->PlatformName();
// 		TMap<FName, TArray<FString>> OutSCLCSVPaths;
// 		TArray<FString>& PlatformSCLCSVPaths = OutSCLCSVPaths.FindOrAdd(FName(TargetPlatformName));
// 		bool bStoraged = FShaderCodeLibrary::SaveShaderCode(ShaderCodeDir, MetaDataPath, ShaderFormats, PlatformSCLCSVPaths, ChunkAssignments);
//
// 		if (UNLIKELY(!bStoraged))
// 		{
// 			UE_LOG(LogTemp, Error, TEXT("%s"),*FString::Printf(TEXT("Saving shared material shader code library failed for %s."), *TargetPlatformName));
// 		}
// 		else
// 		{
// 			const UProjectPackagingSettings* const PackagingSettings = GetDefault<UProjectPackagingSettings>();
// 			if (PackagingSettings->bSharedMaterialNativeLibraries)
// 			{
// 				bStoraged = FShaderCodeLibrary::PackageNativeShaderLibrary(ShaderCodeDir, ShaderFormats);
// 				if (!bStoraged)
// 				{
// 					// This is fatal - In this case we should cancel any launch on device operation or package write but we don't want to assert and crash the editor
// 					UE_LOG(LogTemp, Error, TEXT("%s"),*FString::Printf(TEXT("Package Native Shader Library failed for %s."), *TargetPlatformName));
// 				}
// 			}
// 			for (const FString& Item : PlatformSCLCSVPaths)
// 			{
// 				UE_LOG(LogTemp, Display, TEXT("Saved scl.csv %s for platform %s, %d bytes"), *Item, *TargetPlatformName,
// 					IFileManager::Get().FileSize(*Item));
// 			}
// 		}
// 	}
// }

// void UFlibShaderPatchHelper::SaveGlobalShaderLibrary(const TArray<ETargetPlatform>& Platforms)
// {
// 	const TCHAR* GlobalShaderLibName = TEXT("Global");
// 	bool bIsIterateSharedBuild = true; // IsCookFlagSet(ECookInitializationFlags::IterateSharedBuild)
// 	FString ActualName = GenerateShaderCodeLibraryName(GlobalShaderLibName, bIsIterateSharedBuild);
//
// 	const UProjectPackagingSettings* const PackagingSettings = GetDefault<UProjectPackagingSettings>();
// 	bool const bCacheShaderLibraries = true;//IsUsingShaderCodeLibrary();
// 	if (bCacheShaderLibraries && PackagingSettings->bShareMaterialShaderCode)
// 	{
// 		// Save shader code map - cleaning directories is deliberately a separate loop here as we open the cache once per shader platform and we don't assume that they can't be shared across target platforms.
// 		for (const ITargetPlatform* TargetPlatform : UFlibHotPatcherCoreHelper::GetTargetPlatformsByNames(Platforms))
// 		{
// 			SaveShaderLibrary(TargetPlatform, GlobalShaderLibName, nullptr);
// 		}
//
// 		FShaderCodeLibrary::CloseLibrary(ActualName);
// 	}
// }
