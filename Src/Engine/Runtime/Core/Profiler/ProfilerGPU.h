#pragma once

#include "Runtime/Core/Types/Collections/List.h"
#include "Runtime/Core/Types/Delegate.h"
#include "RenderStats.h"

// Profiler events buffers capacity (tweaked manually)
#define PROFILER_GPU_EVENTS_FRAMES 6

namespace SE
{
	class SE_API_RUNTIME IGPUTimerQueryProfile
	{
	public:
		virtual void Begin() = 0;
		virtual void End() = 0;
		virtual bool HasResult() = 0;
		virtual float GetResult() = 0;
		virtual ~IGPUTimerQueryProfile() = default;
	};

	/// <summary>
	/// Provides GPU performance measuring methods.
	/// </summary>
	class SE_API_RUNTIME ProfilerGPU
	{
	public:
		friend class Engine;
		friend class GPUDevice;

		/// <summary>
		/// Represents single CPU profiling event data.
		/// </summary>
		struct SE_API_RUNTIME Event
		{
			/// <summary>
			/// The name of the event.
			/// </summary>
			const Char* Name;

			/// <summary>
			/// The timer query used to get the exact event time on a GPU. Assigned and managed by the internal profiler layer.
			/// </summary>
			IGPUTimerQueryProfile* Timer;

			/// <summary>
			/// The rendering stats for this event. When event is active it holds the stats on event begin.
			/// </summary>
			RenderStatsData Stats;

			/// <summary>
			/// The event execution time on a GPU (in milliseconds).
			/// </summary>
			float Time;

			/// <summary>
			/// The event depth. Value 0 is used for the root events.
			/// </summary>
			int32 Depth;
		};

		/// <summary>
		/// Implements simple profiling events buffer that holds single frame data.
		/// </summary>
		class SE_API_RUNTIME EventBuffer
		{
			NON_COPYABLE(EventBuffer)
		private:
			bool m_IsResolved = true;
			List<Event> m_Data;

		public:
			EventBuffer() = default;

			/// <summary>
			/// The index of the frame buffer was used for recording events (for the last time).
			/// </summary>
			uint64 FrameIndex;

			/// <summary>
			/// Sum of all present events duration on CPU during this frame (in milliseconds).
			/// </summary>
			float PresentTime;

			/// <summary>
			/// Determines whether this buffer has ready data (resolved and not empty).
			/// </summary>
			bool HasData() const;

			/// <summary>
			/// Ends all used timer queries.
			/// </summary>
			void EndAll();

			/// <summary>
			/// Tries the resolve this frame. Skips if already resolved or has no collected events.
			/// </summary>
			void TryResolve();

			/// <summary>
			/// Gets the event at the specified index.
			/// </summary>
			/// <param name="index">The index.</param>
			/// <returns>The event</returns>
			Event* Get(const int32 index)
			{
				return &m_Data[index];
			}

			/// <summary>
			/// Adds new event to the buffer.
			/// </summary>
			/// <param name="e">The initial event data.</param>
			/// <returns>The event index.</returns>
			int32 Add(const Event& e);

			/// <summary>
			/// Extracts the buffer data.
			/// </summary>
			/// <param name="data">The output data.</param>
			void Extract(List<Event>& data) const;

			/// <summary>
			/// Clears this buffer.
			/// </summary>
			void Clear();
		};

		static Function<IGPUTimerQueryProfile*()> CreateTimerQueryProfileCall;
		static Function<void(const Char*)> EventBeginCall;
		static Function<void()> EventEndCall;
		static Function<uint64()> GetFrameCountCall;

	private:
		static int32 m_Depth;

		static List<IGPUTimerQueryProfile*> m_TimerQueriesPool;
		static List<IGPUTimerQueryProfile*> m_TimerQueriesFree;

		static IGPUTimerQueryProfile* GetTimerQuery();

	public:
		/// <summary>
		/// True if GPU profiling is enabled, otherwise false to disable events collecting and GPU timer queries usage. Can be changed during rendering.
		/// </summary>
		static bool Enabled;

		/// <summary>
		/// The current frame buffer to collect events.
		/// </summary>
		static int32 CurrentBuffer;

		/// <summary>
		/// The events buffers (one per frame).
		/// </summary>
		static EventBuffer Buffers[PROFILER_GPU_EVENTS_FRAMES];

	public:
		/// <summary>
		/// Begins the event. Call EndEvent with index parameter equal to the returned value by BeginEvent function.
		/// </summary>
		/// <param name="name">The event name.</param>
		/// <returns>The event token index</returns>
		static int32 BeginEvent(const Char* name);

		/// <summary>
		/// Ends the active event.
		/// </summary>
		/// <param name="index">The event token index returned by the BeginEvent method.</param>
		static void EndEvent(int32 index);

		/// <summary>
		/// Tries to get the rendering stats from the last frame drawing (that has been resolved and has valid data).
		/// </summary>
		/// <param name="drawTimeMs">The draw execution time on a GPU (in milliseconds).</param>
		/// <param name="presentTimeMs">The final frame present time on a CPU (in milliseconds). Time game waited for vsync or GPU to finish previous frame rendering.</param>
		/// <param name="statsData">The rendering stats data.</param>
		/// <returns>True if got the data, otherwise false.</returns>
		static bool GetLastFrameData(float& drawTimeMs, float& presentTimeMs, RenderStatsData& statsData);

		static void Dispose();
	private:
		static void BeginFrame();
		static void OnPresent();
		static void OnPresentTime(float time);
		static void EndFrame();
	};

	/// <summary>
	/// Helper structure used to call BeginEvent/EndEvent within single code block.
	/// </summary>
	struct ScopeProfileBlockGPU
	{
		int32 Index;

		FORCE_INLINE ScopeProfileBlockGPU(const Char* name)
		{
			Index = ProfilerGPU::BeginEvent(name);
		}

		FORCE_INLINE ~ScopeProfileBlockGPU()
		{
			ProfilerGPU::EndEvent(Index);
		}
	};
}


template<>
struct TIsPODType<SE::ProfilerGPU::Event>
{
    enum { Value = true };
};

#ifdef SE_PROFILER

// Shortcut macro for profiling rendering on GPU
#define PROFILE_GPU(name) ::SE::ScopeProfileBlockGPU ProfileBlockGPU(SE_TEXT(name))

#else

// Empty macros for disabled profiler
#define PROFILE_GPU(name)

#endif
