#pragma once

#include "Core/Types/Variable.h"

namespace SE
{
	/// <summary>
	/// Contains information about current memory usage and capacity.
	/// </summary>
	struct MemoryStats
	{
		/// <summary>
		/// Total amount of physical memory in bytes.
		/// </summary>
		uint64 TotalPhysicalMemory;

		/// <summary>
		/// Amount of used physical memory in bytes.
		/// </summary>
		uint64 UsedPhysicalMemory;

		/// <summary>
		/// Total amount of virtual memory in bytes.
		/// </summary>
		uint64 TotalVirtualMemory;

		/// <summary>
		/// Amount of used virtual memory in bytes.
		/// </summary>
		uint64 UsedVirtualMemory;
	};

	/// <summary>
	/// Contains information about current memory usage by the process.
	/// </summary>
	struct ProcessMemoryStats
	{
		/// <summary>
		/// Amount of used physical memory in bytes.
		/// </summary>
		uint64 UsedPhysicalMemory;

		/// <summary>
		/// Amount of used virtual memory in bytes.
		/// </summary>
		uint64 UsedVirtualMemory;
	};
}