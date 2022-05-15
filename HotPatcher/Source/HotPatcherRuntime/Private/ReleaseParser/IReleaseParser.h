#pragma once

#include "CoreMinimal.h"
#include "ETargetPlatform.h"
#include "FExternFileInfo.h"
#include "FPatcherSpecifyAsset.h"

struct FReleaseParserConf
{
	ETargetPlatform TargetPlatform = ETargetPlatform::None;
};

struct FReleasePakListConf: public FReleaseParserConf
{
	TArray<FString> PakResponseFiles;
};

struct FReleasePakFilesConf : public FReleaseParserConf
{
	TArray<FString> PakFiles;
	FString AESKey;
};

struct FReleaseParserResult
{
	ETargetPlatform Platform = ETargetPlatform::None;
	TArray<FPatcherSpecifyAsset> Assets;
	TArray<FExternFileInfo> ExternFiles;
};

struct IReleaseParser
{
	virtual void Parser(TSharedPtr<FReleaseParserConf> ParserConf, EHashCalculator HashCalculator)=0;
	virtual const FReleaseParserResult& GetParserResult()const {return result;};
 	virtual ~IReleaseParser(){}
protected:
	FReleaseParserResult result;
};