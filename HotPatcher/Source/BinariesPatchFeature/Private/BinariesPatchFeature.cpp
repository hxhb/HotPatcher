// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "BinariesPatchFeature.h"
#include "HotPatcherTemplateHelper.hpp"

#include "Resources/Version.h"
#include "Features/IModularFeatures.h"
#include "Misc/EnumRange.h"
#include "Modules/ModuleManager.h"
#include "UObject/Class.h"

template<typename ENUM_TYPE>
	static UEnum* GetUEnum()
{
#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION > 21
	UEnum* FoundEnum = StaticEnum<ENUM_TYPE>();
#else
	FString EnumTypeName = ANSI_TO_TCHAR(THotPatcherTemplateHelper::GetCPPTypeName<ENUM_TYPE>().c_str());
	UEnum* FoundEnum = FindObject<UEnum>(ANY_PACKAGE, *EnumTypeName, true); 
#endif
	return FoundEnum;
}

template<typename T>
void AppendEnumValue(FName Name,int32 NativeExistedNum = 0)
{
	UEnum* GenericUEnum = GetUEnum<T>();
	if(!IsValid(GenericUEnum))
		return;
	uint64 MaxEnumValue = GenericUEnum->GetMaxEnumValue() - NativeExistedNum;
	FString EnumName = GenericUEnum->GetName();
	TArray<TPair<FName, int64>> EnumNames;
	TArray<FString> AppendPlatformEnums;
	
#if WITH_EDITOR
	AppendPlatformEnums.AddUnique(Name.ToString());
#endif
	for (T Enum:TEnumRange<T>())
	{
		FName EnumtorName = GenericUEnum->GetNameByValue((int64)Enum);
		EnumNames.Emplace(EnumtorName,(int64)Enum);
		AppendPlatformEnums.Remove(EnumtorName.ToString());
	}
	
	for(const auto& AppendEnumItem:AppendPlatformEnums)
	{
		++MaxEnumValue;
		EnumNames.Emplace(
			FName(*FString::Printf(TEXT("%s::%s"),*EnumName,*AppendEnumItem)),
			MaxEnumValue
		);
	}
#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION > 25
	GenericUEnum->SetEnums(EnumNames,UEnum::ECppForm::EnumClass,EEnumFlags::None,true);
#else
	GenericUEnum->SetEnums(EnumNames,UEnum::ECppForm::EnumClass,true);
#endif
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
		
}

void OnBinariesModularFeatureRegistered(const FName& Type, IModularFeature* ModularFeature)
{
	if(!Type.ToString().Equals(BINARIES_DIFF_PATCH_FEATURE_NAME,ESearchCase::IgnoreCase))
		return;
	IBinariesDiffPatchFeature* Feature = static_cast<IBinariesDiffPatchFeature*>(ModularFeature);
	AppendEnumValue<EBinariesPatchFeature>(*Feature->GetFeatureName());
}
void OnBinariesModularFeatureUnRegistered(const FName& Type, IModularFeature* ModularFeature)
{
	
}

void FBinariesPatchFeatureModule::StartupModule()
{
	TArray<IBinariesDiffPatchFeature*> RegistedFeatures = IModularFeatures::Get().GetModularFeatureImplementations<IBinariesDiffPatchFeature>(BINARIES_DIFF_PATCH_FEATURE_NAME);
	for(const auto& Featue:RegistedFeatures)
	{
		AppendEnumValue<EBinariesPatchFeature>(*Featue->GetFeatureName());
	}
	IModularFeatures::Get().OnModularFeatureRegistered().AddStatic(&OnBinariesModularFeatureRegistered);
	IModularFeatures::Get().OnModularFeatureUnregistered().AddStatic(&OnBinariesModularFeatureUnRegistered);
}

void FBinariesPatchFeatureModule::ShutdownModule()
{
	
}

IMPLEMENT_MODULE( FBinariesPatchFeatureModule, BinariesPatchFeature );
