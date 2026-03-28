#pragma once

#include "Core/Types/Variable.h"
#include "Core/API.h"

namespace SE
{
	/// <summary>
	/// Contains information about CPU (Central Processing Unit).
	/// </summary>
	struct SE_API_CORE CPUInfo
	{
		/// <summary>
		/// The number of physical processor packages.
		/// </summary>
		uint32 ProcessorPackageCount;

		/// <summary>
		/// The number of processor cores (physical).
		/// </summary>
		uint32 ProcessorCoreCount;

		/// <summary>
		/// The number of logical processors (including hyper-threading).
		/// </summary>
		uint32 LogicalProcessorCount;

		/// <summary>
		/// The size of processor L1 caches (in bytes).
		/// </summary>
		uint32 L1CacheSize;

		/// <summary>
		/// The size of processor L2 caches (in bytes).
		/// </summary>
		uint32 L2CacheSize;

		/// <summary>
		/// The size of processor L3 caches (in bytes).
		/// </summary>
		uint32 L3CacheSize;

		/// <summary>
		/// The CPU memory page size (in bytes).
		/// </summary>
		uint32 PageSize;

		/// <summary>
		/// The CPU clock speed (in Hz).
		/// </summary>
		uint64 ClockSpeed;

		/// <summary>
		/// The CPU cache line size (in bytes).
		/// </summary>
		uint32 CacheLineSize;
	};

}