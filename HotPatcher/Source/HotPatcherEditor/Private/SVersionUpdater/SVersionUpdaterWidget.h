#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Input/Reply.h"
#include "Interfaces/IHttpRequest.h"
#include "Widgets/SCompoundWidget.h"

#include "FVersionUpdaterManager.h"

class SChildModWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SChildModWidget):
	_ToolName(),
	_ToolVersion(),
	_ModDesc()
	{}
	SLATE_ATTRIBUTE( FString, ToolName )
	SLATE_ATTRIBUTE( float, ToolVersion )
	SLATE_ATTRIBUTE( FChildModDesc, ModDesc )
	SLATE_END_ARGS()
	
	FText GetModDisplay() const
	{
		return FText::FromString(
			FString::Printf(TEXT("%s v%.1f"),*GetModName(),GetCurrentVersion())
			);
	};
	
public:
	/**
	* Construct the widget
	*
	* @param	InArgs			A declaration from which to construct the widget
	*/
	void Construct(const FArguments& InArgs);

	float GetCurrentVersion()const { return ModDesc.CurrentVersion; }
	float GetRemoteVersion()const { return ModDesc.RemoteVersion; }
	FString GetModName()const { return ModDesc.ModName; };
	
private:
	FString ToolName;
	float ToolVersion = 0.f;
	FChildModDesc ModDesc;
	TSharedPtr<SHorizontalBox> HorizontalBox;
};

class SChildModManageWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SChildModManageWidget):
	_ToolName(),
	_ToolVersion(),
	_bShowPayInfo()
	{}
	SLATE_ATTRIBUTE( FString, ToolName )
	SLATE_ATTRIBUTE( float, ToolVersion )
	SLATE_ATTRIBUTE( bool, bShowPayInfo )
	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs);
	bool AddModCategory(const FString& Category,const TArray<FChildModDesc>& ModsDesc);
	bool AddChildMod(const FChildModDesc& ModDesc);
	bool AddPayment(const FString& Name,const FString& ImageBrushName);
	void SetPaymentFocus(const FString& Name);
	
protected:
	FString ToolName;
	float ToolVersion = 0.f;
	bool bShowPayInfo = true;
	
	// child mod
	TSharedPtr<SButton> ExpanderButton;
	TSharedPtr<SBorder> ChildModBorder;
	TSharedPtr<SVerticalBox> ChildModBox;
	// payment
	TSharedPtr<SVerticalBox> PayBox;
	TSharedPtr<SHorizontalBox> PaymentButtonWrapper;
	TSharedPtr<SImage> PayImage;

	struct FPaymentInfo
	{
		FString Name;
		FString BrushName;
		TSharedPtr<SButton> Button;
		TSharedPtr<STextBlock> TextBlock;
	};
	TMap<FString,FPaymentInfo> PaymentInfoMap;
};

class SVersionUpdaterWidget : public SCompoundWidget
{

public:
	SLATE_BEGIN_ARGS(SVersionUpdaterWidget):
	_CurrentVersion(),
	_ToolName(),
	_ToolVersion(),
	_DeveloperName(),
	_DeveloperWebsite(),
	_UpdateWebsite()
	{}
	SLATE_ATTRIBUTE( int32, CurrentVersion )
	SLATE_ATTRIBUTE( int32, PatchVersion )
	SLATE_ATTRIBUTE( FText, ToolName )
	SLATE_ATTRIBUTE( float, ToolVersion )
	SLATE_ATTRIBUTE( FText, DeveloperName )
	SLATE_ATTRIBUTE( FText, DeveloperWebsite )
	SLATE_ATTRIBUTE( FText, UpdateWebsite )
	SLATE_END_ARGS()

public:
	/**
	* Construct the widget
	*
	* @param	InArgs			A declaration from which to construct the widget
	*/
	void Construct(const FArguments& InArgs);
	void HyLinkClickEventOpenUpdateWebsite();
	void HyLinkClickEventOpenDeveloperWebsite();

	FText GetCurrentVersionText() const
	{
		return FText::FromString(FString::Printf(TEXT("Current Version v%d.%d"),GetCurrentVersion(),GetPatchVersion()));
	};
	FText GetEngineVersionText() const
	{
		return FText::FromString(FString::Printf(TEXT("Engine Version %d.%d.%d"),ENGINE_MAJOR_VERSION,ENGINE_MINOR_VERSION,ENGINE_PATCH_VERSION));
	};
	FText GetToolName() const {return FText::FromString(ToolName);};
	FText GetDeveloperName() const {return FText::FromString(DeveloperName);};
	FText GetUpdateWebsite() const {return FText::FromString(UpdateWebsite);};
	FText GetDeveloperWebsite() const {return FText::FromString(DeveloperWebsite);};
	FText GetDeveloperDescrible() const {return FText::FromString(FString::Printf(TEXT("Developed by %s"),*GetDeveloperName().ToString()));};
	FText GetLatstVersionText() const {return FText::FromString(FString::Printf(TEXT("A new version v%d.%d is avaliable"),RemoteVersion.Version,RemoteVersion.PatchVersion));};
	virtual void SetToolUpdateInfo(const FString& ToolName,const FString& DeveloperName,const FString& DeveloperWebsite,const FString& UpdateWebsite);
	int32 GetCurrentVersion()const { return CurrentVersion; }
	int32 GetPatchVersion()const { return PatchVersion; }
	
	
private:
	void OnRemoveVersionFinished();
	
	int32 CurrentVersion = 0;
	int32 PatchVersion = 0;
	FString ToolName;
	float ToolVersion = 0.f;
	FString UpdateWebsite;
	FString DeveloperWebsite;
	FString DeveloperName;
	TSharedPtr<SHorizontalBox> UpdateInfoWidget;
	FRemteVersionDescrible RemoteVersion;
	TSharedPtr<SChildModManageWidget> ChildModManageWidget;
};

