#include "FBinariesPatchConfig.h"
#include "FlibPatchParserHelper.h"

FString FBinariesPatchConfig::GetBinariesPatchFeatureName() const
{
	return THotPatcherTemplateHelper::GetEnumNameByValue(BinariesPatchType);
}

FString FBinariesPatchConfig::GetOldCookedDir() const
{
	return UFlibPatchParserHelper::ReplaceMarkPath(OldCookedDir.Path);
}

FString FBinariesPatchConfig::GetBasePakExtractCryptoJson() const
{
	FString JsonFile;
	if(EncryptSettings.bUseDefaultCryptoIni)
	{
		JsonFile = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("HotPatcher/Crypto.json")));
		FPakEncryptionKeys PakEncryptionKeys = UFlibPatchParserHelper::GetCryptoByProjectSettings();
		UFlibPatchParserHelper::SerializePakEncryptionKeyToFile(PakEncryptionKeys,JsonFile);
	}
	else
	{
		JsonFile = UFlibPatchParserHelper::ReplaceMarkPath(EncryptSettings.CryptoKeys.FilePath);
	}
	return JsonFile;
}

bool FBinariesPatchConfig::IsMatchIgnoreRules(const FPakCommandItem& File)
{
	bool bIsIgnore = false;
	if(!FPaths::FileExists(File.AssetAbsPath))
		return false;
	uint64 FileSize = IFileManager::Get().FileSize(*File.AssetAbsPath);

	auto FormatMatcher = [](const FString& File,const TArray<FString>& Formaters)->bool
	{
		bool bFormatMatch = !Formaters.Num();
		for(const auto& FileFormat:Formaters)
		{
			if(!FileFormat.IsEmpty() && File.EndsWith(FileFormat,ESearchCase::CaseSensitive))
			{
				bFormatMatch = true;
				break;
			}
		}
		return bFormatMatch;
	};
		
	auto SizeMatcher = [](uint64 FileSize,const FMatchRule& MatchRule)->bool
	{
		bool bMatch = false;
		uint64 RuleSize = MatchRule.Size * 1024; // kb to byte
		switch (MatchRule.Operator)
		{
		case EMatchOperator::GREAT_THAN:
			{
				bMatch = FileSize > RuleSize;
				break;
			}
		case EMatchOperator::LESS_THAN:
			{
				bMatch = FileSize < RuleSize;
				break;
			}
		case EMatchOperator::EQUAL:
			{
				bMatch = FileSize == RuleSize;
				break;
			}
		case EMatchOperator::None:
			{
				bMatch = true;
				break;
			}
		}
		return bMatch;
	};
		
	for(const auto& Rule:GetMatchRules())
	{
		bool bSizeMatch = SizeMatcher(FileSize,Rule);
		bool bFormatMatch = FormatMatcher(File.AssetAbsPath,Rule.Formaters);
		bool bMatched = bSizeMatch && bFormatMatch;
		bIsIgnore = Rule.Rule == EMatchRule::MATCH ? !bMatched : bMatched;
	}
	return bIsIgnore;
}


TArray<FString> FBinariesPatchConfig::GetBaseVersionPakByPlatform(ETargetPlatform Platform)
{
	TArray<FString> result;
	for(const auto& BaseVersionPak:GetBaseVersionPaks())
	{
		if(BaseVersionPak.Platform == Platform)
		{
			for(const auto& Path:BaseVersionPak.Paks)
			{
				result.Emplace(UFlibPatchParserHelper::ReplaceMarkPath(Path.FilePath));
			}
		}
	}
	return result;
}