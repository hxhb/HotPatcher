// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

class IPatchableInterface
{

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	virtual void ImportConfig()=0;
	virtual void ExportConfig()const=0;
	virtual void ResetConfig() = 0;
	virtual void DoGenerate()=0;
};
