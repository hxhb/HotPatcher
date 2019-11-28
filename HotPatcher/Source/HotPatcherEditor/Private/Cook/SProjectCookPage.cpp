// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "SProjectCookPage.h"

#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SExpandableArea.h"

#include "SHotPatcherCookedPlatforms.h"
#include "SHotPatcherCookSetting.h"
#include "SHotPatcherCookMaps.h"

#define LOCTEXT_NAMESPACE "SProjectCookPage"


/* SProjectCookPage interface
 *****************************************************************************/

void SProjectCookPage::Construct(const FArguments& InArgs, TSharedPtr<FHotPatcherCookModel> InCookModel)
{
	mCookModel = InCookModel;

	ChildSlot
	[
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.FillWidth(1.0)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("WhichProjectToUseText", "How would you like to cook the content?"))
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0, 8.0, 0.0, 0.0)
		[
			SNew(SExpandableArea)
			.AreaTitle(LOCTEXT("CookPlatforms", "Platforms"))
			.InitiallyCollapsed(true)
			.Padding(8.0)
			.BodyContent()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SHotPatcherCookedPlatforms,mCookModel)
				]
			]
			
		]
		+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0, 8.0, 0.0, 0.0)
			[
				SNew(SExpandableArea)
				.AreaTitle(LOCTEXT("CookMaps", "Map(s)"))
				.InitiallyCollapsed(true)
				.Padding(8.0)
				.BodyContent()
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SHotPatcherCookMaps, mCookModel)
					]
				]

			]
		+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0, 8.0, 0.0, 0.0)
			[
				SNew(SExpandableArea)
				.AreaTitle(LOCTEXT("CookSetting", "Settings"))
				.InitiallyCollapsed(true)
				.Padding(8.0)
				.BodyContent()
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SHotPatcherCookSetting, mCookModel)
					]
				]

			]
		+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0, 8.0, 0.0, 0.0)

		+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Right)
			.Padding(4, 4, 10, 4)
			[
				SNew(SButton)
				.Text(LOCTEXT("RunCook", "Cook Content"))
				.OnClicked(this,&SProjectCookPage::RunCook)
				.IsEnabled(this,&SProjectCookPage::CanExecuteCook)
			]
	];
}

bool SProjectCookPage::CanExecuteCook()const
{
	if (!(mCookModel->GetAllSelectedPlatform().Num() > 0))
	{
		return false;
	}
	if (!(mCookModel->GetAllSelectedCookMap().Num() > 0) && 
		!mCookModel->GetAllSelectedSettings().Contains(TEXT("CookAll")))
	{
		return false;
	}
	return true;
}

FReply SProjectCookPage::RunCook()const
{
	FString ProjectFilePath;
	FString FinalCookCommand;
	{
		FString ProjectPath = UKismetSystemLibrary::GetProjectDirectory();
		FString ProjectName = FString(FApp::GetProjectName()).Append(TEXT(".uproject"));
		ProjectFilePath = FPaths::Combine(ProjectPath, ProjectName);
	}
	FString EngineBin = FPaths::ConvertRelativePathToFull(FPaths::EngineDir() / TEXT("Binaries/Win64/UE4Editor-Cmd.exe"));
	
	if (FPaths::FileExists(ProjectFilePath))
	{
		FinalCookCommand = TEXT("\"") + ProjectFilePath + TEXT("\"");
		FinalCookCommand.Append(TEXT(" -run=cook "));

		FString CookParams;
		FString ErrorMsg;
		if (mCookModel->CombineAllCookParams(CookParams, ErrorMsg))
		{
			FinalCookCommand.Append(CookParams);
		}
		RunCookProc(EngineBin, FinalCookCommand);
		// FPlatformProcess::CreateProc(*EngineBin, *FinalCookCommand,true,false,false,NULL,NULL,NULL,NULL);
		
	}
	return FReply::Handled();
}


void SProjectCookPage::RunCookProc(const FString& InBinPath, const FString& InCommand)const
{
	void* PipeRead = nullptr;
	void* PipeWrite = nullptr;
	int32 ReturnCode = -1;

	verify(FPlatformProcess::CreatePipe(PipeRead, PipeWrite));
	bool bLaunchDetached = false;
	bool bLaunchHidden = true;
	bool bLaunchReallyHidden = true;
	uint32* OutProcessID = nullptr;
	int32 PriorityModifier = -1;
	const TCHAR* OptionalWorkingDirectory = nullptr;
	FProcHandle ProcessHandle = FPlatformProcess::CreateProc(
		*InBinPath, *InCommand,
		bLaunchDetached, bLaunchHidden, bLaunchReallyHidden,
		OutProcessID, PriorityModifier,
		OptionalWorkingDirectory,
		PipeWrite
	);

	//if (ProcessHandle.IsValid())
	//{
	//	FPlatformProcess::WaitForProc(ProcessHandle);
	//	FPlatformProcess::GetProcReturnCode(ProcessHandle, &ReturnCode);
	//	if (ReturnCode == 0)
	//	{
	//		TArray<FString> OutResults;
	//		const FString StdOut = FPlatformProcess::ReadPipe(PipeRead);
	//		StdOut.ParseIntoArray(OutResults, TEXT("\n"), true);
	//		UE_LOG(LogTemp, Log, TEXT("Cook Task Successfuly:\n%s"), *StdOut);
	//	}
	//	else
	//	{
	//		TArray<FString> OutErrorMessages;
	//		const FString StdOut = FPlatformProcess::ReadPipe(PipeRead);
	//		StdOut.ParseIntoArray(OutErrorMessages, TEXT("\n"), true);
	//		UE_LOG(LogTemp, Warning, TEXT("Cook Task Falied:\nReturnCode=%d\n%s"), ReturnCode, *StdOut);
	//	}

	//	FPlatformProcess::ClosePipe(PipeRead, PipeWrite);
	//	FPlatformProcess::CloseProc(ProcessHandle);
	//}
}
#undef LOCTEXT_NAMESPACE
