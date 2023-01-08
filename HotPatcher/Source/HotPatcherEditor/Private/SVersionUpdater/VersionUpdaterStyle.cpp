// Copyright 2019 Lipeng Zha, Inc. All Rights Reserved.

#include "VersionUpdaterStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/SlateStyleRegistry.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/EngineVersionComparison.h"

TSharedPtr< FSlateStyleSet > FVersionUpdaterStyle::StyleInstance = NULL;

void FVersionUpdaterStyle::Initialize(const FString& Name)
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create(Name);
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FVersionUpdaterStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

FName FVersionUpdaterStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("VersionUpdaterStyle"));
	return StyleSetName;
}

#define IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define IMAGE_CORE_BRUSH( RelativePath, ... ) FSlateImageBrush( Style->RootToCoreContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BOX_BRUSH( RelativePath, ... ) FSlateBoxBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BORDER_BRUSH( RelativePath, ... ) FSlateBorderBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define TTF_FONT( RelativePath, ... ) FSlateFontInfo( Style->RootToContentDir( RelativePath, TEXT(".ttf") ), __VA_ARGS__ )
#define OTF_FONT( RelativePath, ... ) FSlateFontInfo( Style->RootToContentDir( RelativePath, TEXT(".otf") ), __VA_ARGS__ )

const FVector2D Updater_Icon12x12(12.0f, 12.0f);
const FVector2D Updater_Icon16x16(16.0f, 16.0f);
const FVector2D Updater_Icon20x20(20.0f, 20.0f);
const FVector2D Updater_Icon40x40(40.0f, 40.0f);
const FVector2D Updater_IconPay(120.0f, 120.0f);

TSharedRef< FSlateStyleSet > FVersionUpdaterStyle::Create(const FString& Name)
{
	TSharedRef< FSlateStyleSet > Style = MakeShareable(new FSlateStyleSet(*Name));
	Style->SetContentRoot( FPaths::EngineContentDir() / TEXT("Slate") );
	Style->SetCoreContentRoot( FPaths::EngineContentDir() / TEXT("Editor/Slate") );
	
	Style->Set("Updater.GroupBorder", new BOX_BRUSH("Common/GroupBorder", FMargin(4.0f/16.0f)));

#if !UE_VERSION_OLDER_THAN(5,1,0)
	Style->Set("Updater.QuickLaunch", new IMAGE_BRUSH("Launcher/Launcher_Launch", Updater_Icon40x40));
#else
	Style->Set("Updater.QuickLaunch", new IMAGE_CORE_BRUSH("Launcher/Launcher_Launch", Updater_Icon40x40));
#endif
	
	Style->Set("Updater.Star", new IMAGE_CORE_BRUSH("Sequencer/Star", Updater_Icon12x12));
	Style->Set("Updater.SpawnableIconOverlay", new IMAGE_CORE_BRUSH(TEXT("Sequencer/SpawnableIconOverlay"), FVector2D(13, 13)));
	
	FString PluginResourceDir = IPluginManager::Get().FindPlugin("HotPatcher")->GetBaseDir() / TEXT("Resources/Payments");
	Style->Set(
		"Updater.WechatPay",
		new FSlateImageBrush( FPaths::Combine(PluginResourceDir , TEXT("wechatpay.png")),
			Updater_IconPay)
			);
	Style->Set(
		"Updater.AliPay",
		new FSlateImageBrush( FPaths::Combine(PluginResourceDir , TEXT("alipay.png")),
			Updater_IconPay)
			);
	return Style;
}

const FSlateBrush* FVersionUpdaterStyle::GetBrush( FName PropertyName, const ANSICHAR* Specifier)
{
	return FVersionUpdaterStyle::StyleInstance->GetBrush( PropertyName, Specifier );
}

#undef IMAGE_BRUSH
#undef BOX_BRUSH
#undef BORDER_BRUSH
#undef TTF_FONT
#undef OTF_FONT

void FVersionUpdaterStyle::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

const ISlateStyle& FVersionUpdaterStyle::Get()
{
	return *StyleInstance;
}
