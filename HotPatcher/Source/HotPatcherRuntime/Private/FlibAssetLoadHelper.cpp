// Fill out your copyright notice in the Description page of Project Settings.


#include "FlibAssetLoadHelper.h"
#include "AssetRegistryModule.h"
#include "ConstructorHelpers.h"

UObject* UFlibAssetLoadHelper::LoadAssetByPackageName(const FString& InPackageName, FString& OutAssetType)
{
	UObject* result = NULL;

	if (FPackageName::DoesPackageExist(InPackageName))
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		TArray<FAssetData> AssetDataList;
		bool bResault = AssetRegistryModule.Get().GetAssetsByPackageName(FName(*InPackageName), AssetDataList);
		if (bResault && !!AssetDataList.Num())
		{	
			result = StaticLoadObject(UObject::StaticClass(),nullptr,*InPackageName);
			OutAssetType = AssetDataList[0].AssetClass.ToString();
		}
	}
	return result;
}

FString UFlibAssetLoadHelper::GetObjectResource(UObject* Obj)
{
	FSoftObjectPath SoftRef(Obj);
	return SoftRef.ToString();
}
