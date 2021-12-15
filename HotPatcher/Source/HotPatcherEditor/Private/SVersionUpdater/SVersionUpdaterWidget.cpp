#include "SVersionUpdaterWidget.h"
// engine header
#include "Widgets/Input/SHyperlink.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "HttpModule.h"
#include "Misc/FileHelper.h"
#include "Json.h"
#include "VersionUpdaterStyle.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"

#define LOCTEXT_NAMESPACE "VersionUpdaterWidget"


void SVersionUpdaterWidget::Construct(const FArguments& InArgs)
{
	static bool GBrushInited = false;
	if(!GBrushInited)
	{
		FVersionUpdaterStyle::Initialize(FString::Printf(TEXT("%s_UpdaterStyle"),ANSI_TO_TCHAR(TOOL_NAME)));
		FVersionUpdaterStyle::ReloadTextures();
		GBrushInited = true;
	}
	
	SetToolUpdateInfo(
		InArgs._ToolName.Get().ToString(),
		InArgs._DeveloperName.Get().ToString(),
		InArgs._DeveloperWebsite.Get().ToString(),
		InArgs._UpdateWebsite.Get().ToString()
		);
	CurrentVersion = InArgs._CurrentVersion.Get();
	
	ChildSlot
	[
		SNew(SBorder)
		.Padding(2)
		.BorderImage(FVersionUpdaterStyle::GetBrush("Updater.GroupBorder"))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			.Padding(4.0f, 0.0f, 4.0f, 0.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SBox)
					.WidthOverride(40)
					.HeightOverride(40)
					[
						SNew(SImage)
						.Image(FVersionUpdaterStyle::GetBrush("Updater.QuickLaunch"))
					]
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(10,0,10,0)
				.VAlign(VAlign_Center)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(2, 4, 2, 4)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SHyperlink)
							.Text_Raw(this,&SVersionUpdaterWidget::GetToolName)
							.OnNavigate(this, &SVersionUpdaterWidget::HyLinkClickEventOpenUpdateWebsite)
						]
						+ SHorizontalBox::Slot()
						.FillWidth(1.0)
						[
							SNew(SOverlay)
						]
					]
					+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(2, 4, 2, 4)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.Padding(0,0,4,0)
							[
								SNew(STextBlock)
								.Text_Raw(this,&SVersionUpdaterWidget::GetCurrentVersionText)
							]
							+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								SAssignNew(UpdateInfoWidget,SHorizontalBox)
								.Visibility(EVisibility::Collapsed)
								+ SHorizontalBox::Slot()
								.AutoWidth()
								[
									SNew(SBox)
									.WidthOverride(18)
									.HeightOverride(18)
									[
										SNew(SImage)
										.Image(FVersionUpdaterStyle::GetBrush("Updater.SpawnableIconOverlay"))
									]
								]
								+ SHorizontalBox::Slot()
								.Padding(2,0,0,0)
								.AutoWidth()
								[
									SNew(SHyperlink)
									.Text_Raw(this,&SVersionUpdaterWidget::GetLatstVersionText)
									.OnNavigate(this, &SVersionUpdaterWidget::HyLinkClickEventOpenUpdateWebsite)
								]
							]
						]
				]
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			[
				SNew(SOverlay)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(10,0,10,0)
			.VAlign(VAlign_Center)
			[
				SNew(SHyperlink)
				.Text_Raw(this,&SVersionUpdaterWidget::GetDeveloperDescrible)
				.OnNavigate(this, &SVersionUpdaterWidget::HyLinkClickEventOpenDeveloperWebsite)
			]
		]
	];
	RequestVersion(REMOTE_VERSION_FILE);
}

void SVersionUpdaterWidget::HyLinkClickEventOpenUpdateWebsite()
{
	FPlatformProcess::LaunchURL(*UpdateWebsite, NULL, NULL);
}

void SVersionUpdaterWidget::HyLinkClickEventOpenDeveloperWebsite()
{
	FPlatformProcess::LaunchURL(*DeveloperWebsite, NULL, NULL);
}

void SVersionUpdaterWidget::SetToolUpdateInfo(const FString& InToolName, const FString& InDeveloperName,
	const FString& InDeveloperWebsite, const FString& InUpdateWebsite)
{
	ToolName = InToolName;
	DeveloperName = InDeveloperName;
	DeveloperWebsite = InDeveloperWebsite;
	UpdateWebsite = InUpdateWebsite;
}

DEFINE_LOG_CATEGORY_STATIC(LogVersionUpdater,All,All);

void SVersionUpdaterWidget::OnRequestComplete(FHttpRequestPtr RequestPtr, FHttpResponsePtr ResponsePtr, bool bConnectedSuccessfully)
{
	FString Result = ResponsePtr->GetContentAsString();;
	// UE_LOG(LogVersionUpdater, Log, TEXT("%s"),*Result);
	TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(Result);
	TSharedPtr<FJsonObject> JsonObject;
	if (FJsonSerializer::Deserialize(JsonReader, JsonObject))
	{
		TArray<FString> ToolNames;
		if(JsonObject->TryGetStringArrayField(TEXT("Tools"),ToolNames))
		{
			if(ToolNames.Contains(GetToolName().ToString()))
			{
				const TSharedPtr<FJsonObject>* ToolJsonObject;
				if(JsonObject->TryGetObjectField(GetToolName().ToString(),ToolJsonObject))
				{
					int32 Version = ToolJsonObject->Get()->GetIntegerField(TEXT("Version"));
					FString Developer = ToolJsonObject->Get()->GetStringField(TEXT("Author"));
					FString UpdateURL = ToolJsonObject->Get()->GetStringField(TEXT("URL"));
					FString Website = ToolJsonObject->Get()->GetStringField(TEXT("Website"));
					SetToolUpdateInfo(GetToolName().ToString(),Developer,Website,UpdateURL);
					LatstVersion = Version;
					if(CurrentVersion < LatstVersion)
					{
						UpdateInfoWidget->SetVisibility(EVisibility::Visible);
					}
				}
			}
		}
	}
}

void SVersionUpdaterWidget::RequestVersion(const FString& URL)
{
	if(HttpHeadRequest.IsValid())
	{
		HttpHeadRequest->CancelRequest();
		HttpHeadRequest.Reset();
	}
	HttpHeadRequest = FHttpModule::Get().CreateRequest();
	HttpHeadRequest->SetURL(URL);
	HttpHeadRequest->SetVerb(TEXT("GET"));
	HttpHeadRequest->OnProcessRequestComplete().BindRaw(this,&SVersionUpdaterWidget::OnRequestComplete);
	if (HttpHeadRequest->ProcessRequest())
	{
		UE_LOG(LogVersionUpdater, Log, TEXT("Request %s Version."),*GetToolName().ToString());
	}
}

#undef LOCTEXT_NAMESPACE
