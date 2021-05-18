// Fill out your copyright notice in the Description page of Project Settings.
#include "ShaderPatch/ShaderPatchProxy.h"
#include "FlibHotPatcherEditorHelper.h"
#include "Commandlets/HotShaderPatchCommandlet.h"
#include "ShaderPatch/FlibShaderPatchHelper.h"

#define LOCTEXT_NAMESPACE "HotPatcherShaderPatchProxy"

static FString ShaderExtension = TEXT(".ushaderbytecode");
static FString GetCodeArchiveFilename(const FString& BaseDir, const FString& LibraryName, FName Platform)
{
	return BaseDir / FString::Printf(TEXT("ShaderArchive-%s-"), *LibraryName) + Platform.ToString() + ShaderExtension;
}

bool UShaderPatchProxy::DoExport()
{
	bool bStatus = false;
	for(const auto& PlatformConfig:GetSettingObject()->ShaderPatchConfigs)
	{
		UE_LOG(LogHotShaderPatchCommandlet,Display,TEXT("Generating Shader Patch for %s"),*UFlibPatchParserHelper::GetEnumNameByValue(PlatformConfig.Platform));
		
		FString SaveToPath = FPaths::Combine(GetSettingObject()->GetSaveAbsPath(),GetSettingObject()->VersionID,UFlibPatchParserHelper::GetEnumNameByValue(PlatformConfig.Platform));
		bool bCreateStatus = UFlibShaderPatchHelper::CreateShaderCodePatch(
        UFlibShaderPatchHelper::ConvDirectoryPathToStr(PlatformConfig.OldMetadataDir),
        FPaths::ConvertRelativePathToFull(PlatformConfig.NewMetadataDir.Path),
        SaveToPath,
        PlatformConfig.bNativeFormat,
        PlatformConfig.bDeterministicShaderCodeOrder
        );

		auto GetShaderPatchFormatLambda = [](const FString& ShaderPatchDir)->TMap<FName, TSet<FString>>
		{
			TMap<FName, TSet<FString>> FormatLibraryMap;
			TArray<FString> LibraryFiles;
			IFileManager::Get().FindFiles(LibraryFiles, *(ShaderPatchDir), *ShaderExtension);
	
			for (FString const& Path : LibraryFiles)
			{
				FString Name = FPaths::GetBaseFilename(Path);
				if (Name.RemoveFromStart(TEXT("ShaderArchive-")))
				{
					TArray<FString> Components;
					if (Name.ParseIntoArray(Components, TEXT("-")) == 2)
					{
						FName Format(*Components[1]);
						TSet<FString>& Libraries = FormatLibraryMap.FindOrAdd(Format);
						Libraries.Add(Components[0]);
					}
				}
			}
			return FormatLibraryMap;
		};
		
		if(bCreateStatus)
		{
			TMap<FName,TSet<FString>> ShaderFormatLibraryMap  = GetShaderPatchFormatLambda(SaveToPath);
			FText Msg = LOCTEXT("GeneratedShaderPatch", "Successd to Generated the Shader Patch.");
			TArray<FName> FormatNames;
			ShaderFormatLibraryMap.GetKeys(FormatNames);
			for(const auto& FormatName:FormatNames)
			{
				TArray<FString> LibraryNames= ShaderFormatLibraryMap[FormatName].Array();
				for(const auto& LibrartName:LibraryNames)
				{
					FString OutputFilePath = GetCodeArchiveFilename(SaveToPath, LibrartName, FormatName);
					if(FPaths::FileExists(OutputFilePath))
					{
						bStatus = true;
						if(!IsRunningCommandlet())
						{
							UFlibHotPatcherEditorHelper::CreateSaveFileNotify(Msg, OutputFilePath);
						}
						else
						{
							UE_LOG(LogHotShaderPatchCommandlet,Display,TEXT("%s"),*Msg.ToString());
						}
					}
					else
					{
						UE_LOG(LogHotShaderPatchCommandlet,Display,TEXT("ERROR: %s not found!"),*OutputFilePath);
					}
				}
			}
		}
	}

	if(GetSettingObject()->bSaveConfig)
	{
		FString SerializedJsonStr;
		UFlibPatchParserHelper::TSerializeStructAsJsonString(*GetSettingObject(),SerializedJsonStr);

		FString SaveToPath = FPaths::Combine(GetSettingObject()->GetSaveAbsPath(),GetSettingObject()->VersionID,GetSettingObject()->VersionID).Append(TEXT(".json"));
		
		if (FFileHelper::SaveStringToFile(SerializedJsonStr, *SaveToPath))
		{
			FText Msg = LOCTEXT("SavedShaderPatchConfigMas", "Successd to Export the Shader Patch Config.");
			if(!IsRunningCommandlet())
			{
				UFlibHotPatcherEditorHelper::CreateSaveFileNotify(Msg, SaveToPath);
			}
			else
			{
				UE_LOG(LogHotShaderPatchCommandlet,Display,TEXT("%s"),*Msg.ToString());
			}
		}
	}
	return bStatus;
}

#undef LOCTEXT_NAMESPACE