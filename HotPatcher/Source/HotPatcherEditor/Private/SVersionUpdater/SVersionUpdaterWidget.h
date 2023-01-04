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
	_ModName(),
	_CurrentVersion(),
	_RemoteVersion(),
	_Description(),
	_URL(),
	_UpdateURL(),
	_bIsBuiltInMod()
	{}
	SLATE_ATTRIBUTE( FString, ModName )
	SLATE_ATTRIBUTE( float, CurrentVersion )
	SLATE_ATTRIBUTE( float, RemoteVersion )
	SLATE_ATTRIBUTE( FString, Description )
	SLATE_ATTRIBUTE( FString, URL )
	SLATE_ATTRIBUTE( FString, UpdateURL )
	SLATE_ATTRIBUTE( bool, bIsBuiltInMod )
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

	float GetCurrentVersion()const { return CurrentVersion; }
	float GetRemoteVersion()const { return RemoteVersion; }
	FString GetModName()const { return ModName; };
	
private:
	float CurrentVersion = 0;
	FString ModName;
	float RemoteVersion = 0;
	FString Description;
	FString URL;
	FString UpdateURL;
	bool bIsBuiltInMod = false;
	TSharedPtr<SHorizontalBox> HorizontalBox;
};


class SVersionUpdaterWidget : public SCompoundWidget
{

public:
	SLATE_BEGIN_ARGS(SVersionUpdaterWidget):
	_CurrentVersion(),
	_ToolName(),
	_DeveloperName(),
	_DeveloperWebsite(),
	_UpdateWebsite()
	{}
	SLATE_ATTRIBUTE( int32, CurrentVersion )
	SLATE_ATTRIBUTE( int32, PatchVersion )
	SLATE_ATTRIBUTE( FText, ToolName )
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
		return FText::FromString(
			FString::Printf(TEXT("Current Version v%d.%d"),GetCurrentVersion(),GetPatchVersion())
			);
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
	bool AddChildMod(const FChildModDesc& ModDesc);
	
private:
	void OnRemoveVersionFinished();
	
	int32 CurrentVersion = 0;
	int32 PatchVersion = 0;
	FString ToolName;
	FString UpdateWebsite;
	FString DeveloperWebsite;
	FString DeveloperName;
	TSharedPtr<SHorizontalBox> UpdateInfoWidget;
	FRemteVersionDescrible RemoteVersion;

	// child mod
	TSharedPtr<SButton> ExpanderButton;
	TSharedPtr<SBorder> ChildModBorder;
	TSharedPtr<SVerticalBox> ChildModBox;
	TSharedPtr<SVerticalBox> PayBox;
};

