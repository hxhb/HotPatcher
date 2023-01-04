// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShaderPatcherEditor.h"
#include "Slate/SShaderPatchWidget.h"

#define LOCTEXT_NAMESPACE "FShaderPatcherEditorModule"

DEFINE_LOG_CATEGORY(LogShaderPatcherEditor);

void FShaderPatcherEditorModule::StartupModule()
{
	FHotPatcherModBaseModule::StartupModule();
}

void FShaderPatcherEditorModule::ShutdownModule()
{
	FHotPatcherModBaseModule::ShutdownModule();
}

FHotPatcherModDesc FShaderPatcherEditorModule::GetModDesc() const
{
	FHotPatcherModDesc TmpModDesc;
	TmpModDesc.ModName = MOD_NAME;
	TmpModDesc.CurrentVersion = MOD_VERSION;
	TmpModDesc.bIsBuiltInMod = IS_INTERNAL_MODE;
	
	TmpModDesc.ModActions.Emplace(
		TEXT("Patcher"),MOD_NAME,TEXT("ByShaderPatch"), TEXT("Create an Shader code Patch form Metadata."),
		CREATE_ACTION_WIDGET_LAMBDA(SShaderPatchWidget,TEXT("ByShaderPatch")),-1
		);
	return TmpModDesc;
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FShaderPatcherEditorModule, ShaderPatcherEditor)