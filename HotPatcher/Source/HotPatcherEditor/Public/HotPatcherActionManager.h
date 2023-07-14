#pragma once
// engine header
#include "CoreMinimal.h"
#include "SHotPatcherPageBase.h"
#include "SHotPatcherWidgetBase.h"
#include "Widgets/SCompoundWidget.h"
#include "Model/FHotPatcherContextBase.h"
#include "Kismet/KismetTextLibrary.h"

using FRequestWidgetPtr = TFunction<TSharedRef<SHotPatcherWidgetInterface>(TSharedPtr<FHotPatcherContextBase>)>;

struct HOTPATCHEREDITOR_API FHotPatcherActionDesc
{
	FHotPatcherActionDesc()=default;
	FHotPatcherActionDesc(FString InCategory,FString InModName,FString InActionName,FString InToolTip,FRequestWidgetPtr InRequestWidgetPtr,int32 InPriority = 0):
	Category(InCategory),ModName(InModName),ActionName(InActionName),ToolTip(InToolTip),RequestWidgetPtr(InRequestWidgetPtr),Priority(InPriority)
	{}
	
	FString Category;
	FString ModName;
	FString ActionName;
	FString ToolTip;
	FRequestWidgetPtr RequestWidgetPtr;
	int32 Priority = 0;
};

struct HOTPATCHEREDITOR_API FHotPatcherAction
{
	FHotPatcherAction()=default;
	FHotPatcherAction(
		FName InCategory,
		FName InModName,
		const TAttribute<FText>& InActionName,
		const TAttribute<FText>& InActionToolTip,
		const FSlateIcon& InIcon,
		TFunction<void(void)> InCallback,
		FRequestWidgetPtr InRequestWidget,
		int32 InPriority
		)
	:Category(InCategory),ModName(InModName),ActionName(InActionName),ActionToolTip(InActionToolTip),Icon(InIcon),ActionCallback(InCallback),RequestWidget(InRequestWidget),Priority(InPriority){}
	
	FName Category;
	FName ModName;
	TAttribute<FText> ActionName;
	TAttribute<FText> ActionToolTip;
	FSlateIcon Icon;
	TFunction<void(void)> ActionCallback;
	FRequestWidgetPtr RequestWidget;
	int32 Priority = 0;
};

struct HOTPATCHEREDITOR_API FHotPatcherModDesc
{
	FString ModName;
	bool bIsBuiltInMod = false;
	float CurrentVersion = 0.f;
	FString Description;
	FString URL;
	FString UpdateURL;
	float MinHotPatcherVersion = 0.0f;
	TArray<FHotPatcherActionDesc> ModActions;
};

struct FHotPatcherCategory
{
	FHotPatcherCategory()=default;
	FHotPatcherCategory(const FHotPatcherCategory&)=default;
	FHotPatcherCategory(const FString InCategoryName,TSharedRef<SHotPatcherPageBase> InWidget):CategoryName(InCategoryName),Widget(InWidget){}
	
	FString CategoryName;
	TSharedRef<SHotPatcherPageBase> Widget;
};

DECLARE_MULTICAST_DELEGATE_ThreeParams(FHotPatcherActionDelegate,const FString&,const FString&,const FHotPatcherAction&);

struct HOTPATCHEREDITOR_API FHotPatcherActionManager
{
    static FHotPatcherActionManager Manager;
	static FHotPatcherActionManager& Get()
	{
		return Manager;
	}
	virtual void Init();
	virtual void Shutdown();

	void RegisterCategory(const FHotPatcherCategory& Category);
	
	void RegisteHotPatcherAction(const FString& Category,const FString& ActionName,const FHotPatcherAction& Action);
	void RegisteHotPatcherAction(const FHotPatcherActionDesc& NewAction);
	void RegisteHotPatcherMod(const FHotPatcherModDesc& ModDesc);
	void UnRegisteHotPatcherMod(const FHotPatcherModDesc& ModDesc);
	void UnRegisteHotPatcherAction(const FString& Category, const FString& ActionName);

	float GetHotPatcherVersion()const;
	
	FHotPatcherActionDelegate OnHotPatcherActionRegisted;
	FHotPatcherActionDelegate OnHotPatcherActionUnRegisted;

	typedef TMap<FString,TMap<FString,FHotPatcherAction>> FHotPatcherActionsType;
	typedef TMap<FString,FHotPatcherCategory> FHotPatcherCategoryType;
	
	FHotPatcherCategoryType& GetHotPatcherCategorys(){ return HotPatcherCategorys; }
	FHotPatcherActionsType& GetHotPatcherActions() { return HotPatcherActions; }
	
	FHotPatcherAction* GetTopActionByCategory(const FString CategoryName);
	bool IsContainAction(const FString& CategoryName,const FString& ActionName);
	bool IsActiviteMod(const FString& ModName);
	
	bool IsSupportEditorAction(FString ActionName);
	bool IsActiveAction(FString ActionName);

	bool GetActionDescByName(const FString& ActionName,FHotPatcherActionDesc& Desc);
	bool GetModDescByName(const FString& ModName,FHotPatcherModDesc& ModDesc);
private:
	virtual ~FHotPatcherActionManager(){}
protected:
	void SetupDefaultActions();
	FHotPatcherActionsType HotPatcherActions;
	FHotPatcherCategoryType HotPatcherCategorys;
	TMap<FString,FHotPatcherActionDesc> ActionsDesc;
	TMap<FName,FHotPatcherModDesc> ModsDesc;
	TMap<FName,FHotPatcherModDesc> UnSupportModsDesc;
};


