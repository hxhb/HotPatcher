#pragma once

#include "IReleaseParser.h"

struct FReleasePaklistParser : public IReleaseParser
{
	virtual void Parser(TSharedPtr<FReleaseParserConf> ParserConf) override;
};