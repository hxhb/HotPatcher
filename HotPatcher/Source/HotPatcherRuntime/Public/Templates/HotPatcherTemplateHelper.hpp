#pragma once

#include "CoreMinimal.h"
#include "JsonObjectConverter.h"
#include "HAL/PlatformMisc.h"
#include "Misc/EnumRange.h"
#include "Resources/Version.h"
#include "Misc/CommandLine.h"

// cpp standard
#include <typeinfo>
#include <cctype>
#include <algorithm>
#include <string>
#include <vector>
#include "Resources/Version.h"

#if  ENGINE_MAJOR_VERSION <=4 && ENGINE_MINOR_VERSION < 25
#define FProperty UProperty
#endif


#define DECAL_GETCPPTYPENAME_SPECIAL(TypeName) \
namespace THotPatcherTemplateHelper \
{\
	template<>\
	std::string GetCPPTypeName<TypeName>()\
	{\
	return std::string(#TypeName);\
	};\
};


namespace THotPatcherTemplateHelper
{
	FORCEINLINE std::vector<std::string> split(const std::string& s, char seperator)
	{
		std::vector<std::string> output;
		std::string::size_type prev_pos = 0, pos = 0;
		while((pos = s.find(seperator, pos)) != std::string::npos)
		{
			std::string substring( s.substr(prev_pos, pos-prev_pos) );
			output.push_back(substring);
			prev_pos = ++pos;
		}
		output.push_back(s.substr(prev_pos, pos-prev_pos)); // Last word
		return output;
	}

	template<typename T>
	static std::string GetCPPTypeName()
	{
		// std::string type_name = typeid(T).name();
		// std::vector<std::string> split_results = THotPatcherTemplateHelper::split(type_name,' ');
		// std::string result = split_results.at(split_results.size()-1);
		std::string result;
		// std::for_each(type_name.begin(),type_name.end(),[&result](const char& character){if(!std::isdigit(character)) result.push_back(character);});
		return result;
	}
	
	template<typename ENUM_TYPE>
	static UEnum* GetUEnum()
	{
		SCOPED_NAMED_EVENT_TEXT("GetUEnum",FColor::Red);
#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION > 21
		UEnum* FoundEnum = StaticEnum<ENUM_TYPE>();
#else
		FString EnumTypeName = ANSI_TO_TCHAR(THotPatcherTemplateHelper::GetCPPTypeName<ENUM_TYPE>().c_str());
		UEnum* FoundEnum = FindObject<UEnum>(ANY_PACKAGE, *EnumTypeName, true); 
#endif
		return FoundEnum;
	}
	
	template<typename ENUM_TYPE>
	FString GetEnumNameByValue(ENUM_TYPE InEnumValue, bool bFullName = false)
	{
		SCOPED_NAMED_EVENT_TEXT("GetEnumNameByValue",FColor::Red);
		FString result;
		{
			FString TypeName;
			FString ValueName;
			UEnum* FoundEnum = nullptr;

#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION > 21
			FoundEnum = StaticEnum<ENUM_TYPE>();
#else
			FString EnumTypeName = ANSI_TO_TCHAR(THotPatcherTemplateHelper::GetCPPTypeName<ENUM_TYPE>().c_str());
			FoundEnum = FindObject<UEnum>(ANY_PACKAGE, *EnumTypeName, true); 
#endif
			if (FoundEnum)
			{
				result = FoundEnum->GetNameByValue((int64)InEnumValue).ToString();
				result.Split(TEXT("::"), &TypeName, &ValueName, ESearchCase::CaseSensitive, ESearchDir::FromEnd);
				if (!bFullName)
				{
					result = ValueName;
				}
			}
		}
		return result;
	}
	

	template<typename ENUM_TYPE>
	bool GetEnumValueByName(const FString& InEnumValueName, ENUM_TYPE& OutEnumValue)
	{
		SCOPED_NAMED_EVENT_TEXT("GetEnumValueByName",FColor::Red);
		bool bStatus = false;
		UEnum* FoundEnum = nullptr;
		FString EnumTypeName;

#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION > 21
		FoundEnum = StaticEnum<ENUM_TYPE>();
		EnumTypeName = FoundEnum->CppType;
#else
		EnumTypeName = ANSI_TO_TCHAR(THotPatcherTemplateHelper::GetCPPTypeName<ENUM_TYPE>().c_str());
		FoundEnum = FindObject<UEnum>(ANY_PACKAGE, *EnumTypeName, true); 
#endif
		
		if (FoundEnum)
		{
			FString EnumValueFullName = EnumTypeName + TEXT("::") + InEnumValueName;
			int32 EnumIndex = FoundEnum->GetIndexByName(FName(*EnumValueFullName));
			if (EnumIndex != INDEX_NONE)
			{
				int32 EnumValue = FoundEnum->GetValueByIndex(EnumIndex);
				ENUM_TYPE ResultEnumValue = (ENUM_TYPE)EnumValue;
				OutEnumValue = ResultEnumValue;
				bStatus = true;
			}
		}
		return bStatus;
	}

	template<typename TStructType>
	static bool TSerializeStructAsJsonObject(const TStructType& InStruct,TSharedPtr<FJsonObject>& OutJsonObject)
	{
		SCOPED_NAMED_EVENT_TEXT("TSerializeStructAsJsonObject",FColor::Red);
		if(!OutJsonObject.IsValid())
		{
			OutJsonObject = MakeShareable(new FJsonObject);
		}
		bool bStatus = FJsonObjectConverter::UStructToJsonObject(TStructType::StaticStruct(),&InStruct,OutJsonObject.ToSharedRef(),0,0);
		return bStatus;
	}

	template<typename TStructType>
    static bool TDeserializeJsonObjectAsStruct(const TSharedPtr<FJsonObject>& OutJsonObject,TStructType& InStruct)
	{
		SCOPED_NAMED_EVENT_TEXT("TDeserializeJsonObjectAsStruct",FColor::Red);
		bool bStatus = false;
		if(OutJsonObject.IsValid())
		{
			bStatus = FJsonObjectConverter::JsonObjectToUStruct(OutJsonObject.ToSharedRef(),TStructType::StaticStruct(),&InStruct,0,0);
		}
		return bStatus;
	}
	
	static bool SerializeJsonObjAsJsonString(TSharedPtr<FJsonObject> JsonObject,FString& OutJsonString)
	{
		SCOPED_NAMED_EVENT_TEXT("SerializeJsonObjAsJsonString",FColor::Red);
		bool bRunStatus = false;
		if(JsonObject.IsValid())
		{
			auto JsonWriter = TJsonWriterFactory<>::Create(&OutJsonString);
			FJsonSerializer::Serialize(JsonObject.ToSharedRef(), JsonWriter);
			bRunStatus = true;
		}
		return bRunStatus;
	}
	
	template<typename TStructType>
    static bool TSerializeStructAsJsonString(const TStructType& InStruct,FString& OutJsonString)
	{
		SCOPED_NAMED_EVENT_TEXT("TSerializeStructAsJsonString",FColor::Red);
		bool bRunStatus = false;

		{
			TSharedPtr<FJsonObject> JsonObject;
			if (THotPatcherTemplateHelper::TSerializeStructAsJsonObject<TStructType>(InStruct,JsonObject) && JsonObject.IsValid())
			{
				auto JsonWriter = TJsonWriterFactory<>::Create(&OutJsonString);
				FJsonSerializer::Serialize(JsonObject.ToSharedRef(), JsonWriter);
				bRunStatus = true;
			}
		}
		return bRunStatus;
	}

	template<typename TStructType>
    static bool TDeserializeJsonStringAsStruct(const FString& InJsonString,TStructType& OutStruct)
	{
		SCOPED_NAMED_EVENT_TEXT("TDeserializeJsonStringAsStruct",FColor::Red);
		bool bRunStatus = false;
		TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(InJsonString);
		TSharedPtr<FJsonObject> DeserializeJsonObject;
		if (FJsonSerializer::Deserialize(JsonReader, DeserializeJsonObject))
		{
			bRunStatus = THotPatcherTemplateHelper::TDeserializeJsonObjectAsStruct<TStructType>(DeserializeJsonObject,OutStruct);
		}
		return bRunStatus;
	}

	static TMap<FString, FString> GetCommandLineParamsMap(const FString& CommandLine)
	{
		TMap<FString, FString> resault;
		TArray<FString> ParamsSwitchs, ParamsTokens;
		FCommandLine::Parse(*CommandLine, ParamsTokens, ParamsSwitchs);

		for (const auto& SwitchItem : ParamsSwitchs)
		{
			TArray<FString> SwitchArray;
			SwitchItem.ParseIntoArray(SwitchArray,TEXT("="),true);
			if(SwitchArray.Num()>1)
			{
				resault.Add(SwitchArray[0],SwitchArray[1].TrimQuotes());
			}
			else
			{
				resault.Add(SwitchArray[0],TEXT(""));
			}
		}
		return resault;
	}

	static bool HasPrroperty(UStruct* Field,const FString& FieldName)
	{
		SCOPED_NAMED_EVENT_TEXT("HasPrroperty",FColor::Red);
		for(TFieldIterator<FProperty> PropertyIter(Field);PropertyIter;++PropertyIter)
		{
			if(PropertyIter->GetName().Equals(FieldName,ESearchCase::IgnoreCase))
			{
				return true;
			}
		}
		return false;
	}

	static bool ConvStr2Bool(const FString& Str,bool& bOut)
	{
		bool bIsTrue = Str.Equals(TEXT("true"),ESearchCase::IgnoreCase);
		bool bIsFlase = Str.Equals(TEXT("false"),ESearchCase::IgnoreCase);
		if(bIsTrue) {	bOut = true;	}
		if(bIsFlase){	bOut = false;	}
		return bIsTrue || bIsFlase;
	}
	
	template<typename T>
    static void ReplaceProperty(T& Struct, const TMap<FString, FString>& ParamsMap)
	{
		SCOPED_NAMED_EVENT_TEXT("ReplaceProperty",FColor::Red);
		TSharedPtr<FJsonObject> DeserializeJsonObject;
		THotPatcherTemplateHelper::TSerializeStructAsJsonObject(Struct,DeserializeJsonObject);
		if (DeserializeJsonObject.IsValid())
		{
			TArray<FString> MapKeys;
			ParamsMap.GetKeys(MapKeys);

			for(const auto& key:MapKeys)
			{
				TArray<FString> BreakedDot;
				key.ParseIntoArray(BreakedDot,TEXT("."));
				if(BreakedDot.Num())
				{
					TSharedPtr<FJsonObject> JsonObject = DeserializeJsonObject;
					if(HasPrroperty(T::StaticStruct(),BreakedDot[0]))
					{
						int32 lastIndex = BreakedDot.Num()-1;
						for(int32 index=0;index<lastIndex;++index)
						{
							JsonObject = JsonObject->GetObjectField(BreakedDot[index]);
						}

						if(JsonObject)
						{
							FString Value = *ParamsMap.Find(key);
							FString From = JsonObject->GetStringField(BreakedDot[lastIndex]);
							JsonObject->SetStringField(BreakedDot[lastIndex],Value);
							UE_LOG(LogTemp,Display,TEXT("ReplaceProperty %s from %s to %s."),*BreakedDot[0],*From,*Value);
						}
					}
				}
			}
			FString JsonStr;
			THotPatcherTemplateHelper::SerializeJsonObjAsJsonString(DeserializeJsonObject,JsonStr);
			THotPatcherTemplateHelper::TDeserializeJsonObjectAsStruct(DeserializeJsonObject,Struct);
		}
	}
	
	template<typename T>
	static TArray<TArray<T>> SplitArray(const TArray<T>& Array,int32 SplitNum)
	{
		SCOPED_NAMED_EVENT_TEXT("SplitArray",FColor::Red);
		TArray<TArray<T>> result;
		result.AddDefaulted(SplitNum);

		for( int32 index=0; index<Array.Num(); ) 
		{
			for(auto& SplitItem:result)
			{
				SplitItem.Add(Array[index]);
				++index;
				if(index >= Array.Num())
				{
					break;
				}
			}
		}
		return result;
	}

	template <typename T>
	TArray<T> GetArrayBySrcWithCondition(TArray<T>& SrcArray, TFunction<bool(T)> Matcher, bool RemoveFromSrc)
	{
		SCOPED_NAMED_EVENT_TEXT("GetArrayBySrcWithCondition",FColor::Red);
		TArray<T> result;
		for(int32 Index = SrcArray.Num() - 1 ;Index >= 0;--Index)
		{
			if(Matcher(SrcArray[Index]))
			{
				result.Add(SrcArray[Index]);
				if(RemoveFromSrc)
				{
					SrcArray.RemoveAtSwap(Index,1,false);
				}
			}
		}
		return result;
	}

	typedef TArray<TPair<FName, int64>> FEnumeratorPair;
	template <typename T>
	FEnumeratorPair AppendEnumeraters(const TArray<FString>& Enmueraters)
	{
		UEnum* UEnumIns = THotPatcherTemplateHelper::GetUEnum<T>();
		uint64 MaxEnumValue = UEnumIns->GetMaxEnumValue()-2;
		FString EnumName = UEnumIns->GetName();
		FEnumeratorPair EnumNamePairs;
	
		TArray<FString> AppendEnumsCopy = Enmueraters;

		for (T EnumeraterName:TEnumRange<T>())
		{
			FName EnumtorName = UEnumIns->GetNameByValue((int64)EnumeraterName);
			EnumNamePairs.Emplace(EnumtorName,(int64)EnumeraterName);
			AppendEnumsCopy.Remove(EnumtorName.ToString());
		}
		for(const auto& AppendEnumItem:AppendEnumsCopy)
		{
			++MaxEnumValue;
			EnumNamePairs.Emplace(
				FName(*FString::Printf(TEXT("%s::%s"),*EnumName,*AppendEnumItem)),
				MaxEnumValue
			);
		}
#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION > 25
		UEnumIns->SetEnums(EnumNamePairs,UEnum::ECppForm::EnumClass,EEnumFlags::None,true);
#else
		UEnumIns->SetEnums(EnumNamePairs,UEnum::ECppForm::EnumClass,true);
#endif
		return EnumNamePairs;
	}
}
