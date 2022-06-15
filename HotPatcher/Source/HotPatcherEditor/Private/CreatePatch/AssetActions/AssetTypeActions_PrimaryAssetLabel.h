#pragma once

#if WITH_EDITOR_SECTION
#include "ETargetPlatform.h"
#include "CoreMinimal.h"
#include "Engine/StaticMesh.h"
#include "Toolkits/IToolkitHost.h"
#include "AssetTypeActions_Base.h"
#include "Engine/PrimaryAssetLabel.h"

struct FToolMenuSection;
class FMenuBuilder;

class FAssetTypeActions_PrimaryAssetLabel : public FAssetTypeActions_Base
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_PrimaryAssetLabel", "Primary Asset Label"); }
	virtual FColor GetTypeColor() const override { return FColor(0, 255, 255); }
	virtual UClass* GetSupportedClass() const override { return UPrimaryAssetLabel::StaticClass(); }
	virtual bool HasActions( const TArray<UObject*>& InObjects ) const override { return true; }
	virtual void GetActions(const TArray<UObject*>& InObjects, FToolMenuSection& Section) override;
	// virtual void OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>() ) override;
	virtual uint32 GetCategories() override { return EAssetTypeCategories::Basic; }
	// virtual class UThumbnailInfo* GetThumbnailInfo(UObject* Asset) const override;
	virtual bool IsImportedAsset() const override { return true; }
	// virtual void GetResolvedSourceFilePaths(const TArray<UObject*>& TypeAssets, TArray<FString>& OutSourceFilePaths) const override;
	// End IAssetTypeActions

private:
	void ExecuteAddToPatchIncludeFilter(TArray<TWeakObjectPtr<UPrimaryAssetLabel>> Objects);
	void ExecuteAddToChunkConfig(TArray<TWeakObjectPtr<UPrimaryAssetLabel>> Objects);
	void MakeCookAndPakActionsSubMenu(class UToolMenu* Menu,TArray<TWeakObjectPtr<UPrimaryAssetLabel>> Objects);
	void OnCookAndPakPlatform(ETargetPlatform Platform,TArray<TWeakObjectPtr<UPrimaryAssetLabel>> Objects);
};
#endif

