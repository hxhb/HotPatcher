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
private:
	TArray<FString> mSelectedPlatform;
};