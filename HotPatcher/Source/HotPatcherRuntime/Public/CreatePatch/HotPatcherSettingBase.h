#pragma once
#include "FPlatformExternFiles.h"
#include "FPatcherSpecifyAsset.h"
#include "FPlatformExternAssets.h"
#include "BaseTypes/AssetManager/FAssetDependenciesInfo.h"
// engine
#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "HotPatcherSettingBase.generated.h"

USTRUCT(BlueprintType)
struct HOTPATCHERRUNTIME_API FPatcherEntitySettingBase
{
    GENERATED_BODY();
    virtual ~FPatcherEntitySettingBase(){}
};
USTRUCT(BlueprintType)
struct HOTPATCHERRUNTIME_API FHotPatcherSettingBase:public FPatcherEntitySettingBase
{
    GENERATED_USTRUCT_BODY()
    FHotPatcherSettingBase();
    
    virtual TArray<FPlatformExternAssets>& GetAddExternAssetsToPlatform();
    virtual void Init();

    virtual TArray<FExternFileInfo> GetAllExternFilesByPlatform(ETargetPlatform InTargetPlatform,bool InGeneratedHash = false);
    virtual TMap<ETargetPlatform,FPlatformExternFiles> GetAllPlatfotmExternFiles(bool InGeneratedHash = false);
    virtual TArray<FExternFileInfo> GetAddExternFilesByPlatform(ETargetPlatform InTargetPlatform);
    virtual TArray<FExternDirectoryInfo> GetAddExternDirectoryByPlatform(ETargetPlatform InTargetPlatform);

    virtual FString GetSaveAbsPath()const;
    FORCEINLINE virtual bool IsStandaloneMode()const {return bStandaloneMode;}
    FORCEINLINE virtual bool IsSaveConfig()const {return bStorageConfig;}
    FORCEINLINE virtual TArray<FString> GetAdditionalCommandletArgs()const{return AdditionalCommandletArgs;}
    FORCEINLINE virtual FString GetCombinedAdditionalCommandletArgs()const
    {
        FString Result;
        for(const auto& Option:GetAdditionalCommandletArgs())
        {
            Result+=FString::Printf(TEXT("%s "),*Option);
        }
        return Result;
    }

    FORCEINLINE virtual bool IsForceSkipContent()const{return bForceSkipContent;}
    FORCEINLINE virtual TArray<FDirectoryPath> GetForceSkipContentRules()const {return ForceSkipContentRules;}
    FORCEINLINE virtual TArray<FSoftObjectPath> GetForceSkipAssets()const {return ForceSkipAssets;}
    virtual TArray<FString> GetAllSkipContents()const;

    FORCEINLINE virtual TArray<FDirectoryPath>& GetAssetIncludeFilters() { return AssetIncludeFilters; }
    FORCEINLINE virtual TArray<FPatcherSpecifyAsset>& GetIncludeSpecifyAssets() { return IncludeSpecifyAssets; }
    FORCEINLINE virtual TArray<FDirectoryPath>& GetAssetIgnoreFilters()  { return AssetIgnoreFilters; }
    FORCEINLINE TArray<FPatcherSpecifyAsset> GetSpecifyAssets()const { return IncludeSpecifyAssets; }
    FORCEINLINE bool AddSpecifyAsset(FPatcherSpecifyAsset const& InAsset){ return IncludeSpecifyAssets.AddUnique(InAsset) != INDEX_NONE; }
    FORCEINLINE virtual TArray<UClass*>& GetForceSkipClasses() { return ForceSkipClasses; }
    // virtual TArray<FString> GetAssetIgnoreFiltersPaths()const;
    FORCEINLINE bool IsAnalysisFilterDependencies()const { return bAnalysisFilterDependencies; }
    FORCEINLINE bool IsRecursiveWidgetTree()const {return bRecursiveWidgetTree;}
    FORCEINLINE bool IsIncludeHasRefAssetsOnly()const { return bIncludeHasRefAssetsOnly; }
    FORCEINLINE TArray<EAssetRegistryDependencyTypeEx> GetAssetRegistryDependencyTypes()const { return AssetRegistryDependencyTypes; }
    
    virtual ~FHotPatcherSettingBase(){}
public:

    UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "Asset Filters",meta = (RelativeToGameContentDir, LongPackageName))
    TArray<FDirectoryPath> AssetIncludeFilters;
    // Ignore directories in AssetIncludeFilters 
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asset Filters", meta = (RelativeToGameContentDir, LongPackageName))
    TArray<FDirectoryPath> AssetIgnoreFilters;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asset Filters")
    bool bIncludeHasRefAssetsOnly = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asset Filters")
    bool bAnalysisFilterDependencies=true;
	
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asset Filters")
    TArray<EAssetRegistryDependencyTypeEx> AssetRegistryDependencyTypes;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asset Filters")
    TArray<FPatcherSpecifyAsset> IncludeSpecifyAssets;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asset Filters")
    bool bRecursiveWidgetTree = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asset Filters")
    bool bForceSkipContent = false;
    
    // force exclude asset folder e.g. Exclude editor content when cooking in Project Settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "Asset Filters",meta = (RelativeToGameContentDir, LongPackageName, EditCondition="bForceSkipContent"))
    TArray<FDirectoryPath> ForceSkipContentRules;
    UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "Asset Filters",meta = (EditCondition="bForceSkipContent"))
    TArray<FSoftObjectPath> ForceSkipAssets;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asset Filters",meta = (EditCondition="bForceSkipContent"))
    TArray<UClass*> ForceSkipClasses;
    
    // backup current project Cooked/PLATFORM/PROJECTNAME/Metadata directory
    UPROPERTY(EditAnywhere, Category = "SaveTo")
    bool bStorageConfig = true;
    UPROPERTY(EditAnywhere, Category = "SaveTo")
    FDirectoryPath SavePath;
    // create a UE4Editor-cmd.exe process execute patch mission.
    UPROPERTY(EditAnywhere, Category = "Advanced")
    bool bStandaloneMode = true;
    UPROPERTY(EditAnywhere, Category = "Advanced")
    TArray<FString> AdditionalCommandletArgs;
    
};