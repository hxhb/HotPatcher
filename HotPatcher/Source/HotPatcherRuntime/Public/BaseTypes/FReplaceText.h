#pragma once

#include "CoreMinimal.h"
#include "FReplaceText.generated.h"

UENUM(BlueprintType)
enum class ESearchCaseMode :uint8
{
	CaseSensitive,
	IgnoreCase
};
USTRUCT(BlueprintType)
struct FReplaceText
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category="")
		FString From;
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category="")
		FString To;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="")
		ESearchCaseMode SearchCase = ESearchCaseMode::CaseSensitive;
};
