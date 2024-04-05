#pragma once
#include "CoreMinimal.h"
#include "Resources/Version.h"
#include "Misc/EngineVersionComparison.h"
#if WITH_PACKAGE_CONTEXT && ENGINE_MAJOR_VERSION > 4
#include "Serialization/PackageWriter.h"
#include "PackageWriterToSharedBuffer.h"

class FHotPatcherPackageWriter:public TPackageWriterToSharedBuffer<ICookedPackageWriter>
{
public:
	virtual FCookCapabilities GetCookCapabilities() const override
	{
		FCookCapabilities Result;
		Result.bDiffModeSupported = true;
		return Result;
	}

	virtual void BeginPackage(const FBeginPackageInfo& Info) override;
#if !UE_VERSION_NEWER_THAN(5,1,1) // FOR UE5.1
	virtual void AddToExportsSize(int64& ExportsSize) override;
#endif
	virtual FDateTime GetPreviousCookTime() const override;
	virtual void Initialize(const FCookInfo& Info) override;
	virtual void BeginCook(
#if UE_VERSION_NEWER_THAN(5,1,1)
		const FCookInfo& Info
#endif
	) override;
	virtual void EndCook(
#if UE_VERSION_NEWER_THAN(5,1,1)
		const FCookInfo& Info
#endif
	) override;
	// virtual void Flush() override;
	virtual TUniquePtr<FAssetRegistryState> LoadPreviousAssetRegistry()override;
	
	virtual FCbObject GetOplogAttachment(FName PackageName, FUtf8StringView AttachmentKey) override;
	virtual void RemoveCookedPackages(TArrayView<const FName> PackageNamesToRemove) override;
	virtual void RemoveCookedPackages() override;
	virtual void MarkPackagesUpToDate(TArrayView<const FName> UpToDatePackages) override;
	virtual bool GetPreviousCookedBytes(const FPackageInfo& Info, FPreviousCookedBytesData& OutData) override;
#if UE_VERSION_OLDER_THAN(5,3,0)
	virtual void CompleteExportsArchiveForDiff(const FPackageInfo& Info, FLargeMemoryWriter& ExportsArchive)override;
#else
	virtual EPackageWriterResult BeginCacheForCookedPlatformData(FBeginCacheForCookedPlatformDataInfo& Info){ return EPackageWriterResult::Success;}
#endif
	virtual void CommitPackageInternal(FPackageRecord&& Record,const IPackageWriter::FCommitPackageInfo& Info)override;

	virtual FPackageWriterRecords::FPackage* ConstructRecord() override;
#if UE_VERSION_NEWER_THAN(5,1,1)
	virtual TFuture<FCbObject> WriteMPCookMessageForPackage(FName PackageName);
	/** Read PackageData written by WriteMPCookMessageForPackage on a CookWorker. Called only on CookDirector. */
	virtual bool TryReadMPCookMessageForPackage(FName PackageName, FCbObjectView Message){ return false;}
#endif
	// virtual TFuture<FMD5Hash> CommitPackage(FCommitPackageInfo&& Info)override;
	// virtual void WritePackageData(const FPackageInfo& Info, FLargeMemoryWriter& ExportsArchive, const TArray<FFileRegion>& FileRegions) override;
	// virtual void WriteBulkData(const FBulkDataInfo& Info, const FIoBuffer& BulkData, const TArray<FFileRegion>& FileRegions) override;
	// virtual void WriteLinkerAdditionalData(const FLinkerAdditionalDataInfo& Info, const FIoBuffer& Data, const TArray<FFileRegion>& FileRegions) override;

private:
	/** Version of the superclass's per-package record that includes our class-specific data. */
	struct FRecord : public FPackageWriterRecords::FPackage
	{
		bool bCompletedExportsArchiveForDiff = false;
	};
	
	/** Buffers that are combined into the HeaderAndExports file (which is then split into .uasset + .uexp or .uoasset + .uoexp). */
	struct FExportBuffer
	{
		FSharedBuffer Buffer;
		TArray<FFileRegion> Regions;
	};

	/**
	 * The data needed to asynchronously write one of the files (.uasset, .uexp, .ubulk, any optional and any additional),
	 * without reference back to other data on this writer.
	 */
	struct FWriteFileData
	{
		FString Filename;
		FCompositeBuffer Buffer;
		TArray<FFileRegion> Regions;
		bool bIsSidecar;
		bool bContributeToHash = true;

		void Write(FMD5& AccumulatedHash, EWriteOptions WriteOptions) const;
	};

	/** Stack data for the helper functions of CommitPackageInternal. */
	struct FCommitContext
	{
		const FCommitPackageInfo& Info;
		TArray<TArray<FExportBuffer>> ExportsBuffers;
		TArray<FWriteFileData> OutputFiles;
	};

	void CollectForSavePackageData(FRecord& Record, FCommitContext& Context);
	void CollectForSaveBulkData(FRecord& Record, FCommitContext& Context);
	void CollectForSaveLinkerAdditionalDataRecords(FRecord& Record, FCommitContext& Context);
	void CollectForSaveAdditionalFileRecords(FRecord& Record, FCommitContext& Context);
	void CollectForSaveExportsFooter(FRecord& Record, FCommitContext& Context);
	void CollectForSaveExportsBuffers(FRecord& Record, FCommitContext& Context);

	TMap<FName, TRefCountPtr<FPackageHashes>>& GetPackageHashes() override
	{
		return AllPackageHashes;
	}
	// If EWriteOptions::ComputeHash is not set, the package will not get added to this.
	TMap<FName, TRefCountPtr<FPackageHashes>> AllPackageHashes;
	
	TFuture<FMD5Hash> AsyncSave(FRecord& Record, const FCommitPackageInfo& Info);
	TFuture<FMD5Hash> AsyncSaveOutputFiles(FRecord& Record, FCommitContext& Context);
};

#endif