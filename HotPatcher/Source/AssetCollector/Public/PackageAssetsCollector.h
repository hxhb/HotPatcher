// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "PlatformInfo.h"
#include "GameDelegates.h"
#include "UObject/ConstructorHelpers.h"
#include "Misc/RedirectCollector.h"
#include "PackageHelperFunctions.h"
#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "PackageAssetsCollector.generated.h"

enum class ECookInitializationFlags
{
	None =										0x00000000, // No flags
	//unused =									0x00000001, 
	Iterative =									0x00000002, // use iterative cooking (previous cooks will not be cleaned unless detected out of date, experimental)
	SkipEditorContent =							0x00000004, // do not cook any content in the content\editor directory
	Unversioned =								0x00000008, // save the cooked packages without a version number
	AutoTick =									0x00000010, // enable ticking (only works in the editor)
	AsyncSave =									0x00000020, // save packages async
	// unused =									0x00000040,
	IncludeServerMaps =							0x00000080, // should we include the server maps when cooking
	UseSerializationForPackageDependencies =	0x00000100, // should we use the serialization code path for generating package dependencies (old method will be deprecated)
	BuildDDCInBackground =						0x00000200, // build ddc content in background while the editor is running (only valid for modes which are in editor IsCookingInEditor())
	GeneratedAssetRegistry =					0x00000400, // have we generated asset registry yet
	OutputVerboseCookerWarnings =				0x00000800, // output additional cooker warnings about content issues
	EnablePartialGC =							0x00001000, // mark up with an object flag objects which are in packages which we are about to use or in the middle of using, this means we can gc more often but only gc stuff which we have finished with
	TestCook =									0x00002000, // test the cooker garbage collection process and cooking (cooker will never end just keep testing).
	//unused =									0x00004000,
	LogDebugInfo =								0x00008000, // enables additional debug log information
	IterateSharedBuild =						0x00010000, // iterate from a build in the SharedIterativeBuild directory 
	IgnoreIniSettingsOutOfDate =				0x00020000, // if the inisettings say the cook is out of date keep using the previously cooked build
	IgnoreScriptPackagesOutOfDate =				0x00040000, // for incremental cooking, ignore script package changes
};
ENUM_CLASS_FLAGS(ECookInitializationFlags);

enum class ECookByTheBookOptions
{
	None =								0x00000000, // no flags
	CookAll	=							0x00000001, // cook all maps and content in the content directory
	MapsOnly =							0x00000002, // cook only maps
	NoDevContent =						0x00000004, // don't include dev content
	ForceDisableCompressed =			0x00000010, // force compression to be disabled even if the cooker was initialized with it enabled
	ForceEnableCompressed =				0x00000020, // force compression to be on even if the cooker was initialized with it disabled
	ForceDisableSaveGlobalShaders =		0x00000040, // force global shaders to not be saved (used if cooking multiple times for the same platform and we know we are up todate)
	NoGameAlwaysCookPackages =			0x00000080, // don't include the packages specified by the game in the cook (this cook will probably be missing content unless you know what you are doing)
	NoAlwaysCookMaps =					0x00000100, // don't include always cook maps (this cook will probably be missing content unless you know what you are doing)
	NoDefaultMaps =						0x00000200, // don't include default cook maps (this cook will probably be missing content unless you know what you are doing)
	NoSlatePackages =					0x00000400, // don't include slate content (this cook will probably be missing content unless you know what you are doing)
	NoInputPackages =					0x00000800, // don't include slate content (this cook will probably be missing content unless you know what you are doing)
	DisableUnsolicitedPackages =		0x00001000, // don't cook any packages which aren't in the files to cook list (this is really dangerious as if you request a file it will not cook all it's dependencies automatically)
	FullLoadAndSave =					0x00002000, // Load all packages into memory and save them all at once in one tick for speed reasons. This requires a lot of RAM for large games.
	PackageStore =						0x00004000, // Cook package header information into a global package store
};
ENUM_CLASS_FLAGS(ECookByTheBookOptions);

/** Simple thread safe proxy for TSet<FName> */
template <typename T>
class FThreadSafeSet
{
	TSet<T> InnerSet;
	FCriticalSection SetCritical;
public:
	void Add(T InValue)
	{
		FScopeLock SetLock(&SetCritical);
		InnerSet.Add(InValue);
	}
	bool AddUnique(T InValue)
	{
		FScopeLock SetLock(&SetCritical);
		if (!InnerSet.Contains(InValue))
		{
			InnerSet.Add(InValue);
			return true;
		}
		return false;
	}
	bool Contains(T InValue)
	{
		FScopeLock SetLock(&SetCritical);
		return InnerSet.Contains(InValue);
	}
	void Remove(T InValue)
	{
		FScopeLock SetLock(&SetCritical);
		InnerSet.Remove(InValue);
	}
	void Empty()
	{
		FScopeLock SetLock(&SetCritical);
		InnerSet.Empty();
	}

	void GetValues(TSet<T>& OutSet)
	{
		FScopeLock SetLock(&SetCritical);
		OutSet.Append(InnerSet);
	}
};


/** A BaseKeyFuncs for Maps and Sets with a quicker hash function for pointers than TDefaultMapKeyFuncs */
template<typename KeyType, typename ValueType, bool bInAllowDuplicateKeys>
struct TFastPointerKeyFuncs : public TDefaultMapKeyFuncs<KeyType, ValueType, bInAllowDuplicateKeys>
{
	using typename TDefaultMapKeyFuncs<KeyType, ValueType, bInAllowDuplicateKeys>::KeyInitType;
	static FORCEINLINE uint32 GetKeyHash(KeyInitType Key)
	{
#if PLATFORM_64BITS
		static_assert(sizeof(UPTRINT) == sizeof(uint64), "Expected pointer size to be 64 bits");
		// Ignoring the lower 4 bits since they are likely zero anyway.
		const uint64 ImportantBits = reinterpret_cast<uint64>(Key) >> 4;
		return GetTypeHash(ImportantBits);
#else
		static_assert(sizeof(UPTRINT) == sizeof(uint32), "Expected pointer size to be 32 bits");
		return static_cast<uint32>(reinterpret_cast<uint32>(Key));
#endif
	}
};

/** A TMap which uses TFastPointerKeyFuncs instead of TDefaultMapKeyFuncs */
template<typename KeyType, typename ValueType, typename SetAllocator = FDefaultSetAllocator>
class TFastPointerMap : public TMap<KeyType, ValueType, FDefaultSetAllocator, TFastPointerKeyFuncs<KeyType, ValueType, false>>
{};

struct FCookByTheBookOptionsEx
{
	bool							bGenerateDependenciesForMaps = false;
	bool							bDisableUnsolicitedPackages = false;
	TMap<FName, TArray<FName>>		SourceToLocalizedPackageVariants;
	TFastPointerMap<const ITargetPlatform*,TMap<FName,TSet<FName>>> MapDependencyGraphs;
	TArray<FName>					StartupPackages;
};

struct FCookTrackerEx
{
	// tracker
	FThreadSafeSet<FName>						NeverCookPackageList;
	TFastPointerMap<const ITargetPlatform*, TSet<FName>> 	PlatformSpecificNeverCookPackages;
};
struct FCookByTheBookStartupOptions
{
public:
	TArray<ITargetPlatform*> TargetPlatforms;
	TArray<FString> CookMaps;
	TArray<FString> CookDirectories;
	TArray<FString> NeverCookDirectories;
	TArray<FString> CookCultures; 
	TArray<FString> IniMapSections;
	TArray<FString> CookPackages; // list of packages we should cook, used to specify specific packages to cook
	ECookByTheBookOptions CookOptions = ECookByTheBookOptions::None;
	FString DLCName;
	FString CreateReleaseVersion;
	FString BasedOnReleaseVersion;
	bool bGenerateStreamingInstallManifests = false;
	bool bGenerateDependenciesForMaps = false;
	bool bErrorOnEngineContentUse = false; // this is a flag for dlc, will cause the cooker to error if the dlc references engine content
};


struct FPackageTracker : public FUObjectArray::FUObjectCreateListener, public FUObjectArray::FUObjectDeleteListener
{
	enum class ERequestType
	{
		None,
		TickCommand,
		Cook
	};
	typedef TFunction<void()> FTickCommand;

	FPackageTracker(/*FPackageNameCache* InPackageNameCache, FCriticalSection& InRequestLock, UCookOnTheFlyServer::FPlatformManager& InPlatformManager*/)
		// : RequestLock(InRequestLock)
		// , PackageNameCache(InPackageNameCache)
		// , PlatformManager(InPlatformManager)
	{
		for (TObjectIterator<UPackage> It; It; ++It)
		{
			UPackage* Package = *It;

			if (Package->GetOuter() == nullptr)
			{
				LoadedPackages.Add(Package);
			}
		}

		NewPackages = LoadedPackages;

		GUObjectArray.AddUObjectDeleteListener(this);
		GUObjectArray.AddUObjectCreateListener(this);

		// We will call FilterLoadedPackage on every package in LoadedPackages the next time GetPackagesPendingSave is called
		// bPackagesPendingSaveDirty = true;
	}

	~FPackageTracker()
	{
		GUObjectArray.RemoveUObjectDeleteListener(this);
		GUObjectArray.RemoveUObjectCreateListener(this);
	}

	TArray<UPackage*> GetNewPackages()
	{
		return MoveTemp(NewPackages);
	}

	virtual void NotifyUObjectCreated(const class UObjectBase *Object, int32 Index) override
	{
		if (Object->GetClass() == UPackage::StaticClass())
		{
			auto Package = const_cast<UPackage*>(static_cast<const UPackage*>(Object));

			if (Package->GetOuter() == nullptr)
			{
				LoadedPackages.Add(Package);
				NewPackages.Add(Package);

				// if (PlatformManager.HasSelectedSessionPlatforms())
				// {
				// 	FilterLoadedPackage(Package);
				// }
			}
		}
	}

	virtual void NotifyUObjectDeleted(const class UObjectBase *Object, int32 Index) override
	{
		if (Object->GetClass() == UPackage::StaticClass())
		{
			auto Package = const_cast<UPackage*>(static_cast<const UPackage*>(Object));

			LoadedPackages.Remove(Package);
			NewPackages.Remove(Package);
			// PostLoadFixupPackages.Remove(Package);
			// PackagesPendingSave.Remove(Package);
		}
	}

	virtual void OnUObjectArrayShutdown() override
	{
		GUObjectArray.RemoveUObjectDeleteListener(this);
		GUObjectArray.RemoveUObjectCreateListener(this);
	}
	// This is a complete list of currently loaded UPackages
	TArray<UPackage*>		LoadedPackages;

	// This list contains the UPackages loaded since last call to GetNewPackages
	TArray<UPackage*>		NewPackages;
};
/**
 * 
 */
UCLASS(Blueprintable,BlueprintType)
class ASSETCOLLECTOR_API UPackageAssetsCollector : public UObject
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable)
	TArray<FString> StartAssetsCollector(const TArray<FString>& PlatformNames);
protected:
	TArray<FString> AssetsCollector(const FCookByTheBookStartupOptions& CookByTheBookStartupOptions, const TArray<ITargetPlatform*> InTargetPlatforms);
	void DiscoverPlatformSpecificNeverCookPackages(const TArrayView<ITargetPlatform*>& TargetPlatforms, const TArray<FString>& UBTPlatformStrings);
	void CollectFilesToCook(TArray<FName>& FilesInPath, const TArray<FString>& CookMaps, const TArray<FString>& InCookDirectories,
	const TArray<FString> &IniMapSections, ECookByTheBookOptions FilesToCookFlags, const TArray<ITargetPlatform*>& TargetPlatforms);

	bool IsCookFlagSet( const ECookInitializationFlags& InCookFlags );
	void AddFileToCook( TArray<FName>& InOutFilesToCook, const FString &InFilename ) const;
	TArray<UPackage*> GetUnsolicitedPackages(const TArray<ITargetPlatform*>& TargetPlatforms) const;

private:
	FCookByTheBookOptionsEx CookByTheBookOptions;
	FCookTrackerEx CookTracker;
	ECookInitializationFlags CookFlags;
	TSharedPtr<FPackageTracker> PackageTracker;
};


