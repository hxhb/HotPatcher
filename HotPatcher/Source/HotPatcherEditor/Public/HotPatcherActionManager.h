#pragma once
// engine header
#include "CoreMinimal.h"
#include "SHotPatcherWidgetBase.h"
#include "Widgets/SCompoundWidget.h"
#include "Model/FHotPatcherContextBase.h"


struct HOTPATCHEREDITOR_API FHotPatcherAction
{
	FHotPatcherAction()=default;
	FHotPatcherAction(
		FName InCategory,
		const TAttribute<FText>& InActionName,
		const TAttribute<FText>& InActionToolTip,
		const FSlateIcon& InIcon,
		TFunction<void(void)> InCallback,
		TFunction<TSharedRef<SHotPatcherWidgetInterface>(TSharedPtr<FHotPatcherContextBase>)> InRequestWidget,
		int32 InPriority
		)
	:Category(InCategory),ActionName(InActionName),ActionToolTip(InActionToolTip),Icon(InIcon),ActionCallback(InCallback),RequestWidget(InRequestWidget),Priority(InPriority){}
	FName Category;
	TAttribute<FText> ActionName;
	TAttribute<FText> ActionToolTip;
	FSlateIcon Icon;
	TFunction<void(void)> ActionCallback;
	TFunction<TSharedRef<SHotPatcherWidgetInterface>(TSharedPtr<FHotPatcherContextBase>)> RequestWidget;
	int32 Priority = 0;
};

struct FHotPatcherCategory
{
	FHotPatcherCategory()=default;
	FHotPatcherCategory(const FHotPatcherCategory&)=default;
	FHotPatcherCategory(const FString InCategoryName,TSharedRef<SCompoundWidget> InWidget):CategoryName(InCategoryName),Widget(InWidget){}
	
	FString CategoryName;
	TSharedRef<SCompoundWidget> Widget;
};

DECLARE_MULTICAST_DELEGATE_ThreeParams(FHotPatcherActionDelegate,const FString&,const FString&,const FHotPatcherAction&);

struct HOTPATCHEREDITOR_API FHotPatcherActionManager
{
	static FHotPatcherActionManager& Get()
	{
		static FHotPatcherActionManager Manager;
		return Manager;
	}
	virtual void Init();
	virtual void Shutdown();

	void RegisterCategory(const FHotPatcherCategory& Category);
	
	void RegisteHotPatcherAction(const FString& Category,const FString& ActionName,const FHotPatcherAction& Action);
	void UnRegisteHotPatcherAction(const FString& Category, const FString& ActionName);

	FHotPatcherActionDelegate OnHotPatcherActionRegisted;
	FHotPatcherActionDelegate OnHotPatcherActionUnRegisted;

	typedef TMap<FString,TMap<FString,FHotPatcherAction>> FHotPatcherActionsType;
	typedef TMap<FString,FHotPatcherCategory> FHotPatcherCategoryType;
	
	FHotPatcherCategoryType& GetHotPatcherCategorys(){ return HotPatcherCategorys; }
	FHotPatcherActionsType& GetHotPatcherActions() { return HotPatcherActions; }
	
	FHotPatcherAction* GetTopActionByCategory(const FString CategoryName);
	FORCEINLINE bool IsActiveAction(FString ActionName);

private:
	virtual ~FHotPatcherActionManager(){}
protected:
	void SetupDefaultActions();
	FHotPatcherActionsType HotPatcherActions;
	FHotPatcherCategoryType HotPatcherCategorys;
	
};