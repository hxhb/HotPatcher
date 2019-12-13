#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "GameExtensionSettings.generated.h"

UCLASS(BlueprintType, Blueprintable, config = GameUserSettings, defaultconfig)
class GAMEEXTENSIONSETTINGS_API UGameExtensionSettings: public UObject
{
	GENERATED_UCLASS_BODY()
public:

	UPROPERTY(config,EditAnywhere,BlueprintReadWrite,Category = GameUpdater)
	TSubclassOf<UObject> GameUpdater;
};
