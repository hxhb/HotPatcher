#pragma once

// engine header
#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "HAL/FileManager.h"
#include "FPakEncryptionKeys.generated.h"

USTRUCT(BlueprintType)
struct HOTPATCHERRUNTIME_API FPakEncryptSettings
{
	GENERATED_BODY()
	// Use DefaultCrypto.ini
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	bool bUseDefaultCryptoIni = false;
	// crypto.json (option)
	UPROPERTY(EditAnywhere,BlueprintReadWrite,meta=(EditCondition="!bUseDefaultCryptoIni"))
	FFilePath CryptoKeys;
};



USTRUCT()
struct FEncryptionKeyEntry
{
	GENERATED_BODY()
	UPROPERTY()
	FString Name;
	UPROPERTY()
	FString Guid;
	UPROPERTY()
	FString Key;
};

USTRUCT()
struct FSignKeyItem
{
	GENERATED_BODY()
	UPROPERTY()
	FString Exponent;
	UPROPERTY()
	FString Modulus;
};

USTRUCT()
struct FSignKeyEntry
{
	GENERATED_BODY()
	UPROPERTY()
	FSignKeyItem PublicKey;
	UPROPERTY()
	FSignKeyItem PrivateKey;
};

USTRUCT()
struct FPakEncryptionKeys
{
	GENERATED_BODY();

	UPROPERTY()
	FEncryptionKeyEntry EncryptionKey;
	UPROPERTY()
	TArray<FEncryptionKeyEntry> SecondaryEncryptionKeys;

	UPROPERTY()
	bool bEnablePakIndexEncryption = false;
	UPROPERTY()
	bool bEnablePakIniEncryption = false;
	UPROPERTY()
	bool bEnablePakUAssetEncryption = false;
	UPROPERTY()
	bool bEnablePakFullAssetEncryption = false;
	UPROPERTY()
	bool bDataCryptoRequired = false;
	UPROPERTY()
	bool PakEncryptionRequired = false;
	UPROPERTY()
	bool PakSigningRequired = false;

	UPROPERTY()
	bool bEnablePakSigning = false;
	UPROPERTY()
	FSignKeyEntry SigningKey;
};

