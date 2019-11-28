// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Interfaces/ITargetPlatformManagerModule.h"
#include "Interfaces/ITargetPlatform.h"
#include "Model/FHotPatcherCreatePatchModel.h"
#include "SharedPointer.h"
/**
 * Implements the cooked platforms panel.
 */
class SHotPatcherExportRelease
	: public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SHotPatcherExportRelease) { }
	SLATE_END_ARGS()

public:

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs The Slate argument list.
	 */
	void Construct(	const FArguments& InArgs,TSharedPtr<FHotPatcherCreatePatchModel> InCreateModel);

protected:


private:

	TSharedPtr<FHotPatcherCreatePatchModel> mCreatePatchModel;


};

