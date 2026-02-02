// Fill out your copyright notice in the Description page of Project Settings.


#include "FlibReflectionHelper.h"
#include "Misc/EngineVersionComparison.h"
FProperty* UFlibReflectionHelper::GetPropertyByName(UClass* Class, FName PropertyName)
{
	FProperty* Property = nullptr;
	for(TFieldIterator<FProperty> PropertyIter(Class);PropertyIter;++PropertyIter)
	{
		FProperty* PropertyIns = *PropertyIter;
		if(PropertyIns->GetName().Equals(PropertyName.ToString()))
		{
			Property = PropertyIns;
		}
	}
	return Property;
}

FString UFlibReflectionHelper::ExportPropertyToText(UObject* Object, FName PropertyName)
{
	FString Value;
	FProperty* Property = GetPropertyByName(Object->GetClass(),PropertyName);
	if(Property)
	{
#if UE_VERSION_OLDER_THAN(5,5,0)
		Property->ExportTextItem(Value,Property->ContainerPtrToValuePtr<uint8>(Object),nullptr,Object,0);

#else
		Property->ExportText_Direct(Value,Property->ContainerPtrToValuePtr<uint8>(Object),nullptr,Object,0);

#endif
		
	}
	return Value;
}

bool UFlibReflectionHelper::ImportPropertyValueFromText(UObject* Object, FName PropertyName,const FString& Text)
{
	FProperty* Property = GetPropertyByName(Object->GetClass(),PropertyName);
	if(Property)
	{
#if UE_VERSION_OLDER_THAN(5,5,0)
		Property->ImportText(*Text,Property->ContainerPtrToValuePtr<uint8>(Object),0,Object);
#else
		Property->ImportText_InContainer(*Text,Property->ContainerPtrToValuePtr<uint8>(Object),Object,0);
#endif
		
	}
	return true;
}
