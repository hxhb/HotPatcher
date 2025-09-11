// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "BinariesPatchFeature.h"
#include "HotPatcherTemplateHelper.hpp"

#include "Resources/Version.h"
#include "Features/IModularFeatures.h"
#include "Misc/EnumRange.h"
#include "Modules/ModuleManager.h"
#include "UObject/Class.h"

DECAL_GETCPPTYPENAME_SPECIAL(EBinariesPatchFeature)

void OnBinariesModularFeatureRegistered(const FName& Type, IModularFeature* ModularFeature)
{
	if(!Type.ToString().Equals(BINARIES_DIFF_PATCH_FEATURE_NAME,ESearchCase::IgnoreCase))
		return;
	IBinariesDiffPatchFeature* Feature = static_cast<IBinariesDiffPatchFeature*>(ModularFeature);
	THotPatcherTemplateHelper::AppendEnumeraters<EBinariesPatchFeature>(TArray<FString>{Feature->GetFeatureName()});
}
void OnBinariesModularFeatureUnRegistered(const FName& Type, IModularFeature* ModularFeature)
{
	
}

void FBinariesPatchFeatureModule::StartupModule()
{
	TArray<IBinariesDiffPatchFeature*> RegistedFeatures = IModularFeatures::Get().GetModularFeatureImplementations<IBinariesDiffPatchFeature>(BINARIES_DIFF_PATCH_FEATURE_NAME);
	for(const auto& Featue:RegistedFeatures)
	{
		THotPatcherTemplateHelper::AppendEnumeraters<EBinariesPatchFeature>(TArray<FString>{Featue->GetFeatureName()});
	}
	IModularFeatures::Get().OnModularFeatureRegistered().AddStatic(&OnBinariesModularFeatureRegistered);
	IModularFeatures::Get().OnModularFeatureUnregistered().AddStatic(&OnBinariesModularFeatureUnRegistered);
}

void FBinariesPatchFeatureModule::ShutdownModule()
{
	
}

IMPLEMENT_MODULE( FBinariesPatchFeatureModule, BinariesPatchFeature );
