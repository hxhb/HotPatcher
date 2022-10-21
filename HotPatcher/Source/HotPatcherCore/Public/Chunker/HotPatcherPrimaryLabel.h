// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "FChunkInfo.h"
#include "CoreMinimal.h"
#include "Engine/AssetManagerTypes.h"
#include "Engine/DataAsset.h"
#include "HotPatcherPrimaryLabel.generated.h"

/**
 * 
 */
UCLASS()
class HOTPATCHERCORE_API UHotPatcherPrimaryLabel : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	static const FName HotPatherBundleName;
	/** Management rules for this specific asset, if set it will override the type rules */
	// UPROPERTY(EditAnywhere, Category = Rules, meta = (ShowOnlyInnerProperties))
	FPrimaryAssetRules Rules;

	/** Primary Assets with a higher priority will take precedence over lower priorities when assigning management for referenced assets. If priorities match, both will manage the same Secondary Asset. */
	// UPROPERTY(EditAnywhere, Category = Rules)
	// int32 Priority = -1;
	//
	// /** Set to true if the label asset itself should be cooked and available at runtime. This does not affect the assets that are labeled, they are set with cook rule */
	// UPROPERTY(EditAnywhere, Category = HotPatcherAssetLabel)
	// uint32 bIsRuntimeLabel : 1;

	UPROPERTY(EditAnywhere,Category = HotPatcherAssetLabel)
	FChunkInfo LabelRules;
	
	UHotPatcherPrimaryLabel()
	{
		Rules.Priority = -1;
		Rules.bApplyRecursively = false;
		Rules.CookRule = EPrimaryAssetCookRule::Unknown;
	}

	/** Set to editor only if this is not available in a cooked build */
	virtual bool IsEditorOnly() const
	{
		return true;
	}

#if WITH_EDITORONLY_DATA
	/** This scans the class for AssetBundles metadata on asset properties and initializes the AssetBundleData with InitializeAssetBundlesFromMetadata */
	virtual void UpdateAssetBundleData();
#endif


private:
	void SetPriority(int32 NewPriority);
	
	
};
