#pragma once

#include "IReleaseParser.h"

struct FReleasePakParser : public IReleaseParser
{
	virtual void Parser(TSharedPtr<FReleaseParserConf> ParserConf) override;
};
