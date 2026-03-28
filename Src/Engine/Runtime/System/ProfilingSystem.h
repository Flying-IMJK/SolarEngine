#pragma once

#include "Core/Types/Collections/Dictionary.h"
#include "Core/Platform/MemoryStats.h"
#include "Core/Systems.h"
#include "Core/Profiler/Profiler.h"

#include "Runtime/API.h"

namespace SE
{
	/// <summary>
	/// Profiler tools for development. Allows to gather profiling data and events from the engine.
	/// </summary>
	class SE_API_RUNTIME ProfilingSystem : ISystem
	{
		ENGINE_SYSTEM(ProfilingSystem)
	public:
		/// <summary>
		/// The GPU memory stats.
		/// </summary>
		struct MemoryStatsGPU
		{
			/// <summary>
			/// The total amount of memory in bytes (as reported by the driver).
			/// </summary>
			uint64 Total;

			/// <summary>
			/// The used by the game amount of memory in bytes (estimated).
			/// </summary>
			uint64 Used;
		};

		/// <summary>
		/// Engine profiling data header. Contains main info and stats.
		/// </summary>
		struct MainStats
		{
			/// <summary>
			/// The process memory stats.
			/// </summary>
			ProcessMemoryStats ProcessMemory;

			/// <summary>
			/// The CPU memory stats.
			/// </summary>
			MemoryStats MemoryCPU;

			/// <summary>
			/// The GPU memory stats.
			/// </summary>
			MemoryStatsGPU MemoryGPU;

			/// <summary>
			/// The frames per second (fps counter).
			/// </summary>
			int32 FPS;

			/// <summary>
			/// The update time on CPU (in milliseconds).
			/// </summary>
			float UpdateTimeMs;

			/// <summary>
			/// The fixed update time on CPU (in milliseconds).
			/// </summary>
			float PhysicsTimeMs;

			/// <summary>
			/// The draw time on CPU (in milliseconds).
			/// </summary>
			float DrawCPUTimeMs;

			/// <summary>
			/// The draw time on GPU (in milliseconds).
			/// </summary>
			float DrawGPUTimeMs;

			/// <summary>
			/// The last rendered frame stats.
			/// </summary>
			RenderStatsData DrawStats;
		};

		/// <summary>
		/// The CPU thread stats.
		/// </summary>
		struct ThreadStats
		{
			/// <summary>
			/// The thread name.
			/// </summary>
			String Name;

			/// <summary>
			/// The events list.
			/// </summary>
			List<ProfilerCPU::Event> Events;
		};

		/// <summary>
		/// The network stat.
		/// </summary>
		struct NetworkEventStat
		{
			// Amount of occurrences.
			uint16 Count;
			// Transferred data size (in bytes).
			uint16 DataSize;
			// Transferred message (data+header) size (in bytes).
			uint16 MessageSize;
			// Amount of peers that will receive this message.
			uint16 Receivers;
			byte Name[120];
		};

	public:
		/// <summary>
		/// Controls the engine profiler (CPU, GPU, etc.) usage.
		/// </summary>
		static bool GetEnabled();

		/// <summary>
		/// Controls the engine profiler (CPU, GPU, etc.) usage.
		/// </summary>
		static void SetEnabled(bool enabled);

		/// <summary>
		/// The current collected main stats by the profiler from the local session. Updated every frame.
		/// </summary>
		static MainStats Stats;

		/// <summary>
		/// The CPU threads profiler events.
		/// </summary>
		static List<ThreadStats, InlinedAllocation<64>> EventsCPU;

		/// <summary>
		/// The GPU rendering profiler events.
		/// </summary>
		static List<ProfilerGPU::Event> EventsGPU;

		/// <summary>
		/// The networking profiler events.
		/// </summary>
		static List<NetworkEventStat> EventsNetwork;

	public:

		ProfilingSystem();

		void Update();

		void Dispose();
	};
}


