#pragma once

#include "CoreMinimal.h"


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
	const TArray<FString>& GetAllSelectedSettings()
	{
		return mOptionSettings;
	}

	bool CombineAllCookParams(FString& OutCommand,FString& OutFaildReson)
	{
		FString result;
		if (!(mSelectedPlatform.Num() > 0))
		{
			OutFaildReson = TEXT("Not Selected any Platform.");
			return false;
		}
		if (!(mSelectedCookMaps.Num() > 0)&& !mOptionSettings.Contains(TEXT("CookAll")))
		{
			OutFaildReson = TEXT("Not Selected any Map.");
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
		if (mExternCookSettingTextBox.Num())
		{
			for (const auto& ExOption : mExternCookSettingTextBox)
			{
				result.Append(ExOption->GetText().ToString());
			}
		}

		OutCommand = result;
		return true;
	}
	void AddExCookSettingTextBox(const TSharedPtr<SEditableTextBox>& InEditableTextBox)
	{
		mExternCookSettingTextBox.AddUnique(InEditableTextBox);
	}
private:
	TArray<FString> mSelectedPlatform;
	TArray<FString> mSelectedCookMaps;
	TArray<FString> mOptionSettings;
	TArray<TSharedPtr<SEditableTextBox>> mExternCookSettingTextBox;
};