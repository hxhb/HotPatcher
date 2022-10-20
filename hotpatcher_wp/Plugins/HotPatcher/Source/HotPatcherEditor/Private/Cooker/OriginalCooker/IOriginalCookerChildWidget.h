#pragma once
#include "Dom/JsonObject.h"
#include "Templates/SharedPointer.h"
#include "Model/FHotPatcherContextBase.h"
#include "Model/FOriginalCookerContext.h"

struct IOriginalCookerChildWidget
{
	virtual TSharedPtr<FJsonObject> SerializeAsJson()const {return nullptr;};
	virtual void DeSerializeFromJsonObj(TSharedPtr<FJsonObject>const & InJsonObject){};
	virtual FString GetSerializeName()const { return TEXT(""); };
	virtual void Reset() {};
	virtual void SetContext(TSharedPtr<FHotPatcherContextBase> InContext)
	{
		mContext = InContext;
	}
	TSharedPtr<FHotPatcherContextBase> GetContext()const { return mContext; };
	virtual FOriginalCookerContext* GetCookerContextPtr()const
	{
		FOriginalCookerContext* Ptr = nullptr;
		if(GetContext().IsValid())
		{
			Ptr = (FOriginalCookerContext*)mContext.Get();
		}
		return Ptr;
	}
	virtual ~IOriginalCookerChildWidget(){}
protected:
	TSharedPtr<FHotPatcherContextBase> mContext;
};