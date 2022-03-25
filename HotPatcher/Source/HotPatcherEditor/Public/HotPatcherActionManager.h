
// engine header
#include "CoreMinimal.h"
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
		TFunction<TSharedRef<SCompoundWidget>(TSharedPtr<FHotPatcherContextBase>)> InRequestWidget
		)
	:Category(InCategory),ActionName(InActionName),ActionToolTip(InActionToolTip),Icon(InIcon),ActionCallback(InCallback),RequestWidget(InRequestWidget){}
	FName Category;
	TAttribute<FText> ActionName;
	TAttribute<FText> ActionToolTip;
	FSlateIcon Icon;
	TFunction<void(void)> ActionCallback;
	TFunction<TSharedRef<SCompoundWidget>(TSharedPtr<FHotPatcherContextBase>)> RequestWidget;
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

	void RegisteHotPatcherAction(const FString& Category,const FString& ActionName,const FHotPatcherAction& Action);
	void UnRegisteHotPatcherAction(const FString& Category, const FString& ActionName);

	FHotPatcherActionDelegate OnHotPatcherActionRegisted;
	FHotPatcherActionDelegate OnHotPatcherActionUnRegisted;

	typedef TMap<FString,TMap<FString,FHotPatcherAction>> FHotPatcherActionsType;
	FHotPatcherActionsType& GetHotPatcherActions() { return HotPatcherActions; }
private:
	virtual ~FHotPatcherActionManager(){}
protected:
	void SetupDefaultActions();
	FHotPatcherActionsType HotPatcherActions;

};