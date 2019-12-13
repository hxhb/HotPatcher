// Fill out your copyright notice in the Description page of Project Settings.

#include "GameExtensionSettingsModule.h"
#include "Modules/ModuleManager.h"
#include "ISettingsModule.h"

// project header
#include "GameExtensionSettings.h"


IMPLEMENT_PRIMARY_GAME_MODULE( FGameExtensionSettingsModule, GameExtensionSettings, "GameExtensionSettings" );

#define LOCTEXT_NAMESPACE "GameExtensionSettings"

void FGameExtensionSettingsModule::StartupModule()
{

	// Initialize setting, take care of this registry syntax
    {
        ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");
        if (SettingsModule != nullptr)
        {
            // ClassViewer Editor Settings
            SettingsModule->RegisterSettings("Project", "Game", "GameExtensionSettings",
                LOCTEXT("GameExtensionSettingsDisplayName", "Game Extension Settings"),
                LOCTEXT("GameExtensionSettingsDescription", "Game Extension Settings."),
                GetMutableDefault<UGameExtensionSettings>()
            );
        }


    }
}

void FGameExtensionSettingsModule::ShutdownModule()
{
	
}

#undef LOCTEXT_NAMESPACE