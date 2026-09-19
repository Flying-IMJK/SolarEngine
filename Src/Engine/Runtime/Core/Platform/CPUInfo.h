#pragma once

#include "Runtime/Core/Types/Variable.h"
#include "Runtime/API.h"

namespace SE
{
	/// <summary>
	/// Contains information about CPU (Central Processing Unit).
	/// </summary>
	SE_STRUCT(API())
	struct SE_API_RUNTIME CPUInfo
	{
		SCRIPTING_TYPE_MIN(CPUInfo)
		/// <summary>
		/// The number of physical processor packages.
		/// </summary>
		SE_FIELD(API())
		uint32 ProcessorPackageCount;

		/// <summary>
		/// The number of processor cores (physical).
		/// </summary>
		SE_FIELD(API())
		uint32 ProcessorCoreCount;

		/// <summary>
		/// The number of logical processors (including hyper-threading).
		/// </summary>
		SE_FIELD(API())
		uint32 LogicalProcessorCount;

		/// <summary>
		/// The size of processor L1 caches (in bytes).
		/// </summary>
		SE_FIELD(API())
		uint32 L1CacheSize;

		/// <summary>
		/// The size of processor L2 caches (in bytes).
		/// </summary>
		SE_FIELD(API())
		uint32 L2CacheSize;

		/// <summary>
		/// The size of processor L3 caches (in bytes).
		/// </summary>
		SE_FIELD(API())
		uint32 L3CacheSize;

		/// <summary>
		/// The CPU memory page size (in bytes).
		/// </summary>
		SE_FIELD(API())
		uint32 PageSize;

		/// <summary>
		/// The CPU clock speed (in Hz).
		/// </summary>
		SE_FIELD(API())
		uint64 ClockSpeed;

		/// <summary>
		/// The CPU cache line size (in bytes).
		/// </summary>
		SE_FIELD(API())
		uint32 CacheLineSize;
	};

}
