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

private:
	TArray<FString> mSelectedPlatform;
	TArray<FString> mSelectedCookMaps;
};