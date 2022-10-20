#pragma once

#if WITH_EDITOR_SECTION
#include "Chunker/HotPatcherPrimaryLabel.h"
#include "ETargetPlatform.h"
#include "CoreMinimal.h"
#include "Engine/StaticMesh.h"
#include "Toolkits/IToolkitHost.h"
#include "AssetTypeActions_Base.h"
#include "Engine/PrimaryAssetLabel.h"

struct FToolMenuSection;
class FMenuBuilder;

class FAssetTypeActions_HotPatcherAssetLabel : public FAssetTypeActions_Base
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_HotPatcherAssetLabel", "HotPatcher Asset Label"); }
	virtual FColor GetTypeColor() const override { return FColor(0, 255, 255); }
	virtual UClass* GetSupportedClass() const override { return UHotPatcherPrimaryLabel::StaticClass(); }
	virtual bool HasActions( const TArray<UObject*>& InObjects ) const override { return true; }
	virtual void GetActions(const TArray<UObject*>& InObjects, FToolMenuSection& Section) override;
	// virtual void OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>() ) override;
	virtual uint32 GetCategories() override { return EAssetTypeCategories::Basic; }
	// virtual class UThumbnailInfo* GetThumbnailInfo(UObject* Asset) const override;
	virtual bool IsImportedAsset() const override { return true; }
	// virtual void GetResolvedSourceFilePaths(const TArray<UObject*>& TypeAssets, TArray<FString>& OutSourceFilePaths) const override;
	// End IAssetTypeActions

private:
	void ExecuteAddToChunkConfig(TArray<TWeakObjectPtr<UHotPatcherPrimaryLabel>> Objects);
	void MakeCookAndPakActionsSubMenu(class UToolMenu* Menu,TArray<TWeakObjectPtr<UHotPatcherPrimaryLabel>> Objects);
	void OnCookAndPakPlatform(ETargetPlatform Platform,TArray<TWeakObjectPtr<UHotPatcherPrimaryLabel>> Objects);
};
#endif

