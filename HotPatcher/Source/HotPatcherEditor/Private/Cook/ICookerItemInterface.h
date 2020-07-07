#pragma once
#include "Dom/JsonObject.h"
#include "Templates/SharedPointer.h"

struct ICookerItemInterface
{
	virtual TSharedPtr<FJsonObject> SerializeAsJson()const =0;
	virtual void DeSerializeFromJsonObj(TSharedPtr<FJsonObject>const & InJsonObject)=0;
	virtual FString GetSerializeName()const = 0;
	virtual void Reset() = 0;
};