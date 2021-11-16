#pragma once

#include "CoreMinimal.h"
#include "IDetailsView.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Input/Reply.h"
#include "Interfaces/IHttpRequest.h"
#include "Widgets/SCompoundWidget.h"

// #define CURRENT_VERSION_ID 68
// #define REMOTE_VERSION_FILE TEXT("https://imzlp.com/opensource/version.json")

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

	FText GetCurrentVersionText() const {return FText::FromString(FString::Printf(TEXT("Current Version v%d."),GetCurrentVersion()));};
	FText GetToolName() const {return FText::FromString(ToolName);};
	FText GetDeveloperName() const {return FText::FromString(DeveloperName);};
	FText GetUpdateWebsite() const {return FText::FromString(UpdateWebsite);};
	FText GetDeveloperWebsite() const {return FText::FromString(DeveloperWebsite);};
	FText GetDeveloperDescrible() const {return FText::FromString(FString::Printf(TEXT("Developed by %s"),*GetDeveloperName().ToString()));};
	FText GetLatstVersionText() const {return FText::FromString(FString::Printf(TEXT("A new version v%d is avaliable"),LatstVersion));};
	virtual void SetToolUpdateInfo(const FString& ToolName,const FString& DeveloperName,const FString& DeveloperWebsite,const FString& UpdateWebsite);
	int32 GetCurrentVersion()const { return CurrentVersion; }

	void OnRequestComplete(FHttpRequestPtr RequestPtr, FHttpResponsePtr ResponsePtr, bool bConnectedSuccessfully);
	void RequestVersion(const FString& URL);
private:
	int32 CurrentVersion = 0;
	FString ToolName;
	FString UpdateWebsite;
	FString DeveloperWebsite;
	FString DeveloperName;
	TSharedPtr<SHorizontalBox> UpdateInfoWidget;
	int32 LatstVersion = 0;
	FHttpRequestPtr HttpHeadRequest;
};

