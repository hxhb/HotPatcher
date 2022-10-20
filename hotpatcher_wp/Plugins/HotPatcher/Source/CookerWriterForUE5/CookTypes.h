// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Containers/Map.h"
#include "Containers/Set.h"
#include "CookOnTheSide/CookOnTheFlyServer.h" // ECookTickFlags
#include "HAL/Platform.h"
#include "Templates/Function.h"

#define COOK_CHECKSLOW_PACKAGEDATA 1
#define DEBUG_COOKONTHEFLY 0

/** A BaseKeyFuncs for Maps and Sets with a quicker hash function for pointers than TDefaultMapKeyFuncs */
template<typename KeyType>
struct TFastPointerSetKeyFuncs : public DefaultKeyFuncs<KeyType>
{
	using typename DefaultKeyFuncs<KeyType>::KeyInitType;
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

// template<typename KeyType, typename ValueType, bool bInAllowDuplicateKeys>
// struct TFastPointerMapKeyFuncs : public TDefaultMapKeyFuncs<KeyType, ValueType, bInAllowDuplicateKeys>
// {
// 	using typename TDefaultMapKeyFuncs<KeyType, ValueType, bInAllowDuplicateKeys>::KeyInitType;
// 	static FORCEINLINE uint32 GetKeyHash(KeyInitType Key)
// 	{
// 		return TFastPointerSetKeyFuncs<KeyType>::GetKeyHash(Key);
// 	}
// };

// /** A TMap which uses TFastPointerMapKeyFuncs instead of TDefaultMapKeyFuncs */
// template<typename KeyType, typename ValueType, typename SetAllocator = FDefaultSetAllocator>
// class TFastPointerMap : public TMap<KeyType, ValueType, SetAllocator, TFastPointerMapKeyFuncs<KeyType, ValueType, false>>
// {};

/** A TSet which uses TFastPointerSetKeyFuncs instead of DefaultKeyFuncs */
template<typename KeyType, typename SetAllocator = FDefaultSetAllocator>
class TFastPointerSet : public TSet<KeyType, TFastPointerSetKeyFuncs<KeyType>, SetAllocator>
{};

namespace UE
{
namespace Cook
{
	/** A function that is called when a requested package finishes cooking (when successful, failed, or skipped) */
	typedef TUniqueFunction<void()> FCompletionCallback;

	/** External Requests to the cooker can either by cook requests for a specific file, or arbitrary callbacks that need to execute within the Scheduler's lock. */
	enum class EExternalRequestType
	{
		None,
		Callback,
		Cook
	};

	/* The Result of a Cook */
	enum class ECookResult
	{
		Unseen,		/* The package has not finished cooking, or if it was previously cooked its result was removed due to e.g. modification of the package. */
		Succeeded,  /* The package was saved with success. */
		Failed,     /* The package was processed but failed to load or save. */
		Skipped     /* For reporting the ECookResults specific to a request: the package was skipped due to e.g. already being cooked or being in NeverCook packages. */
	};

	/** The type of callback for External Requests that needs to be executed within the Scheduler's lock. */
	typedef TUniqueFunction<void()> FSchedulerCallback;

	/** Which phase of cooking a Package is in.  */
	enum class EPackageState
	{
		Idle = 0,	  /* The Package is not being operated on by the cooker, and is not in any queues.  This is the state both for packages that have never been requested and for packages that have finished cooking. */
		Request,	  /* The Package is in the RequestQueue; it is known to the cooker but has not had any operations performed on it. */
		LoadPrepare,  /* The Package is in the LoadPrepareQueue. Preloading is in progress. */
		LoadReady,	  /* The package is in the LoadReadyQueue. Preloading is complete and it will be loaded when its turn comes up. */
		Save,		  /* The Package is in the SaveQueue; it has been fully loaded and some target data may have been calculated. */

		Min = Idle,
		Max = Save,
		Count = Max + 1, /* Number of values in this enum, not a valid value for any EPackageState variable. */
		BitCount = 3, /* Number of bits required to store a valid EPackageState */
	};

	enum class EPackageStateProperty // Bitfield
	{
		None		= 0,
		InProgress	= 0x1, /* The package is being worked on by the cooker. */
		Loading		= 0x2, /* The package is in one of the loading states and has preload data. */
		HasPackage	= 0x4, /* The package has progressed past the loading state, and the UPackage pointer is available on the FPackageData. */

		Min = InProgress,
		Max = HasPackage
	};
	ENUM_CLASS_FLAGS(EPackageStateProperty);

	/** Used as a helper to timeslice cooker functions. */
	struct FCookerTimer
	{
	public:
		enum EForever
		{
			Forever
		};
		enum ENoWait
		{
			NoWait
		};

		FCookerTimer(const float& InTimeSlice, bool bInIsRealtimeMode, int InMaxNumPackagesToSave = 50);
		FCookerTimer(EForever);
		FCookerTimer(ENoWait);

		double GetTimeTillNow() const;
		bool IsTimeUp() const;
		void SavedPackage();
		double GetTimeRemain() const;

	public:
		const bool bIsRealtimeMode;
		const double StartTime;
		const float& TimeSlice;
		const int MaxNumPackagesToSave; // maximum packages to save before exiting tick (this should never really hit unless we are not using realtime mode)
		int NumPackagesSaved;

	private:
		static float ZeroTimeSlice;
		static float ForeverTimeSlice;
	};

	/** Fields from TickCookOnTheSide that helper functions will need access to. */
	struct FTickStackData
	{
		/** The return value from TickCookOnTheSide; it is a bitmask of flags in enum ECookOnTheSideResult */
		uint32 ResultFlags = 0;
		/** The number of packages that have been cooked in the current call to TickCookOnTheSide. It is used for e.g. decisions about when to collect garbage. */
		uint32 CookedPackageCount = 0;
		/** The CookerTimer for the current TickCookOnTheSide request. Used by slow reentrant operations that need to check whether they have timed out. */
		FCookerTimer Timer;
		/** CookFlags describing details of the caller's desired behavior of the current TickCookOnTheSide request. */
		ECookTickFlags TickFlags;

		explicit FTickStackData(const float& TimeSlice, const bool bIsRealtimeMode, ECookTickFlags InTickFlags)
			:Timer(TimeSlice, bIsRealtimeMode), TickFlags(InTickFlags)
		{
		}
	};

	/* Thread Local Storage access to identify which thread is the SchedulerThread for cooking. */
	void InitializeTls();
	bool IsSchedulerThread();
	void SetIsSchedulerThread(bool bValue);
}
}
