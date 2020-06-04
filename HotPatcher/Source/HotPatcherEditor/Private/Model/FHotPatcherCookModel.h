#pragma once

#include "FLibAssetManageHelperEx.h"
#include "FCookerConfig.h"
#include "FlibPatchParserHelper.h"

// engine header
#include "Misc/App.h"
#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Delegates/DelegateCombinations.h"

DECLARE_DELEGATE_OneParam(FRequestExSettingsDlg, TArray<FString>&);
DECLARE_DELEGATE_OneParam(FRequestSpecifyCookFilterDlg, TArray<FDirectoryPath>&);

class FHotPatcherCookModel
{
public:

	void AddSelectedCookPlatform(const FString& InPlatfotm)
	{
		if (!mSelectedPlatform.Contains(InPlatfotm))
		{
			mSelectedPlatform.Add(InPlatfotm);
		}
	}

	void RemoveSelectedCookPlatform(const FString& InPlatfotm)
	{
		if (mSelectedPlatform.Contains(InPlatfotm))
		{
			mSelectedPlatform.Remove(InPlatfotm);
		}
	}

	const TArray<FString>& GetAllSelectedPlatform()
	{
		return mSelectedPlatform;
	}
	void ClearAllPlatform()
	{
		if(mSelectedPlatform.Num()>0)
			mSelectedPlatform.Reset();
	}

	void AddSelectedCookMap(const FString& InNewMap)
	{
		if (!mSelectedCookMaps.Contains(InNewMap))
		{
			mSelectedCookMaps.AddUnique(InNewMap);
		}
	}
	void RemoveSelectedCookMap(const FString& InMap)
	{
		if (mSelectedCookMaps.Contains(InMap))
		{
			mSelectedCookMaps.Remove(InMap);
		}
	}
	const TArray<FString>& GetAllSelectedCookMap()
	{
		return mSelectedCookMaps;
	}
	void ClearAllMap()
	{
		if (mSelectedCookMaps.Num() > 0)
			mSelectedCookMaps.Reset();
	}

	void AddSelectedSetting(const FString& InSetting)
	{
		if (!mOptionSettings.Contains(InSetting))
		{
			mOptionSettings.AddUnique(InSetting);
		}
	}
	void RemoveSelectedSetting(const FString& InSetting)
	{
		if (mOptionSettings.Contains(InSetting))
		{
			mOptionSettings.Remove(InSetting);
		}
	}

	void ClearAllSettings()
	{
		if (mOptionSettings.Num() > 0)
			mOptionSettings.Reset();
	}
	const TArray<FString>& GetAllSelectedSettings()
	{
		return mOptionSettings;
	}

	TArray<FDirectoryPath> GetAlwayCookFilters()
	{
		TArray<FDirectoryPath> CookFilters;
		OnRequestSpecifyCookFilter.ExecuteIfBound(CookFilters);
		return CookFilters;
	}

	TArray<FString> GetAlwayCookFiltersFullPath()
	{
		TArray<FString> result;
		for (const auto& CookFilter : GetAlwayCookFilters())
		{
			FString FilterFullPath;
			if (UFLibAssetManageHelperEx::ConvRelativeDirToAbsDir(CookFilter.Path, FilterFullPath))
			{
				if (FPaths::DirectoryExists(FilterFullPath))
				{
					result.AddUnique(FilterFullPath);
				}
			}
		}
		return result;

	}

	bool CombineAllCookParams(FString& OutCommand,FString& OutFaildReson)
	{
		FString result;
		if (!(mSelectedPlatform.Num() > 0))
		{
			OutFaildReson = TEXT("Not Selected any Platform.");
			return false;
		}
		if (!(mSelectedCookMaps.Num() > 0 || GetAlwayCookFilters().Num() > 0)&& !mOptionSettings.Contains(TEXT("CookAll")))
		{
			OutFaildReson = TEXT("Not Selected any Cookable things.");
			return false;
		}
		result.Append(TEXT("-targetplatform="));
		for (int32 Index = 0; Index < mSelectedPlatform.Num(); ++Index)
		{
			result.Append(mSelectedPlatform[Index]);
			if (!(Index == mSelectedPlatform.Num() - 1))
			{			
				result.Append(TEXT("+"));
			}
		}
		result.Append(TEXT(" "));

		if (GetAllSelectedCookMap().Num() > 0)
		{
			result.Append(TEXT("-Map="));
			for (int32 Index = 0; Index < mSelectedCookMaps.Num(); ++Index)
			{
				result.Append(mSelectedCookMaps[Index]);
				if (!(Index == mSelectedCookMaps.Num() - 1))
				{
					result.Append(TEXT("+"));
				}
			}
			result.Append(TEXT(" "));
		}

		for (const auto& Option : mOptionSettings)
		{
			result.Append(TEXT("-") + Option + TEXT(" "));
		}

		// request cook directory
		for (const auto& CookFilterFullPath : GetAlwayCookFiltersFullPath())
		{
			result.Append(FString::Printf(TEXT("-COOKDIR=\"%s\" "), *CookFilterFullPath));
		}

		// Request Ex Options
		for (const auto& ExOption : GetCookExSetting())
		{
			result.Append(ExOption);
		}

		OutCommand = result;
		return true;
	}

	TArray<FString> GetCookExSetting()
	{
		TArray<FString> ExOptions;
		OnRequestExSettings.ExecuteIfBound(ExOptions);
		return ExOptions;
	}

	FCookerConfig GetCookConfig()
	{
		FCookerConfig result;
		FString ProjectFilePath;
		{
			FString ProjectPath = UKismetSystemLibrary::GetProjectDirectory();
			FString ProjectName = FString(FApp::GetProjectName()).Append(TEXT(".uproject"));
			ProjectFilePath =  FPaths::Combine(ProjectPath, ProjectName);
		}
		FString EngineBin = UFlibPatchParserHelper::GetUE4CmdBinary();

		result.EngineBin = EngineBin;
		result.ProjectPath = ProjectFilePath;
		result.EngineParams = TEXT("-run=cook");

		result.CookPlatforms = mSelectedPlatform;
		result.CookMaps = mSelectedCookMaps;
		TArray<FString> AllGameMap = UFlibPatchParserHelper::GetAvailableMaps(UKismetSystemLibrary::GetProjectDirectory(), false, false, true);
		result.bCookAllMap = result.CookMaps.Num() == AllGameMap.Num();
		for (const auto& Filter : GetAlwayCookFilters())
		{
			result.CookFilter.AddUnique(Filter.Path);
		}
		result.CookSettings = mOptionSettings;
		result.Options = GetCookExSetting()[0];

		return result;
	}
public:
	FRequestExSettingsDlg OnRequestExSettings;
	FRequestSpecifyCookFilterDlg OnRequestSpecifyCookFilter;
private:
	TArray<FString> mSelectedPlatform;
	TArray<FString> mSelectedCookMaps;
	TArray<FString> mOptionSettings;
};