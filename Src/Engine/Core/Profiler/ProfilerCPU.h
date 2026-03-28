#pragma once

#include "Core/Math/Math.h"
#include "Core/Types/Strings/String.h"
#include "Core/ThirdParty/tracy/tracy/Tracy.hpp"

namespace SE
{
	/// <summary>
	/// Provides CPU performance measuring methods.
	/// </summary>
	class SE_API_CORE ProfilerCPU
	{
	public:
		/// <summary>
		/// Represents single CPU profiling event data.
		/// </summary>
		struct Event
		{
			/// <summary>
			/// The start time (in milliseconds).
			/// </summary>
			double Start;

			/// <summary>
			/// The end time (in milliseconds).
			/// </summary>
			double End;

			/// <summary>
			/// The event depth. Value 0 is used for the root event.
			/// </summary>
			int32 Depth;

			/// <summary>
			/// The native dynamic memory allocation size during this event (excluding the child events). Given value is in bytes.
			/// </summary>
			int32 NativeMemoryAllocation;

			/// <summary>
			/// The managed memory allocation size during this event (excluding the child events). Given value is in bytes.
			/// </summary>
			int32 ManagedMemoryAllocation;

			Char Name[100];
		};

		/// <summary>
		/// Implements simple profiling events ring-buffer.
		/// </summary>
		class SE_API_CORE EventBuffer
		{
			NON_COPYABLE(EventBuffer)
		private:
			Event* _data;
			int32 _capacity;
			int32 _capacityMask;
			int32 _head;
			int32 _count;

		public:
			EventBuffer();
			~EventBuffer();

		public:
			/// <summary>
			/// Gets the amount of the events in the buffer.
			/// </summary>
			inline int32 GetCount() const
			{
				return _count;
			}

			/// <summary>
			/// Gets the event at the specified index.
			/// </summary>
			/// <param name="index">The index.</param>
			/// <returns>The event</returns>
			Event& Get(int32 index) const
			{
				ASSERT_LOW_LAYER(index >= 0 && index < _capacity);
				return _data[index];
			}

			/// <summary>
			/// Adds new event to the buffer.
			/// </summary>
			/// <returns>The event index.</returns>
			int32 Add()
			{
				const int32 index = _head;
				_head = (_head + 1) & _capacityMask;
				_count = Math::Min(_count + 1, _capacity);
				return index;
			}

			/// <summary>
			/// Extracts the buffer data (only ended events starting from the root level with depth=0).
			/// </summary>
			/// <param name="data">The output data.</param>
			/// <param name="withRemoval">True if also remove extracted events to prevent double-gather, false if don't modify the buffer data.</param>
			void Extract(List<Event, HeapAllocation>& data, bool withRemoval);

		public:
			/// <summary>
			/// Ring buffer iterator
			/// </summary>
			struct Iterator
			{
				friend EventBuffer;

			private:
				EventBuffer* _buffer;
				int32 _index;

				inline Iterator(EventBuffer* buffer, const int32 index)
					: _buffer(buffer)
					, _index(index)
				{
				}

			public:
				inline Iterator(const Iterator& other)
					: _buffer(other._buffer)
					, _index(other._index)
				{
				}

				inline Iterator(Iterator&& other) noexcept
					: _buffer(other._buffer)
					, _index(other._index)
				{
				}

				inline Iterator& operator=(Iterator&& other)
				{
					_buffer = other._buffer;
					_index = other._index;
					return *this;
				}

				inline Iterator& operator=(const Iterator& other)
				{
					_buffer = other._buffer;
					_index = other._index;
					return *this;
				}

				inline int32 Index() const
				{
					return _index;
				}

				inline Event& Event() const
				{
					ASSERT_LOW_LAYER(_buffer && _index >= 0 && _index < _buffer->_capacity);
					return _buffer->Get(_index);
				}

				inline bool IsEnd() const
				{
					return _index == _buffer->_head;
				}

				inline bool IsNotEnd() const
				{
					return _index != _buffer->_head;
				}

				inline bool operator==(const Iterator& v) const
				{
					return _buffer == v._buffer && _index == v._index;
				}

				inline bool operator!=(const Iterator& v) const
				{
					return _buffer != v._buffer || _index != v._index;
				}

			public:
				inline Iterator& operator++()
				{
					_index = (_index + 1) & _buffer->_capacityMask;
					return *this;
				}

				inline Iterator operator++(int)
				{
					Iterator temp = *this;
					_index = (_index + 1) & _buffer->_capacityMask;
					return temp;
				}

				inline Iterator& operator--()
				{
					_index = (_index - 1) & _buffer->_capacityMask;
					return *this;
				}

				inline Iterator operator--(int)
				{
					Iterator temp = *this;
					_index = (_index - 1) & _buffer->_capacityMask;
					return temp;
				}
			};

		public:
			inline Iterator Begin()
			{
				return Iterator(this, (_head - _count) & _capacityMask);
			}

			inline Iterator Last()
			{
				ASSERT(_count > 0);
				return Iterator(this, (_head - 1) & _capacityMask);
			}

			inline Iterator End()
			{
				return Iterator(this, _head);
			}
		};

		/// <summary>
		/// Thread registered for profiling. Holds events data with read/write support.
		/// </summary>
		class Thread
		{
		private:
			String _name;
			int32 _depth = 0;

		public:
			Thread(const Char* name)
			{
				_name = name;
			}

			Thread(const String& name)
			{
				_name = name;
			}

		public:
			/// <summary>
			/// The current thread.
			/// </summary>
			static THREADLOCAL Thread* Current;

		public:
			/// <summary>
			/// Gets the name.
			/// </summary>
			inline const String& GetName() const
			{
				return _name;
			}

			/// <summary>
			/// The events buffer.
			/// </summary>
			EventBuffer Buffer;

		public:
			/// <summary>
			/// Begins the event running on a this thread. Call EndEvent with index parameter equal to the returned value by BeginEvent function.
			/// </summary>
			/// <returns>The event token.</returns>
			int32 BeginEvent();

			/// <summary>
			/// Ends the event running on a this thread.
			/// </summary>
			/// <param name="index">The event index returned by the BeginEvent method.</param>
			void EndEvent(int32 index);

			/// <summary>
			/// Ends the last event running on a this thread.
			/// </summary>
			void EndEvent();
		};

	public:
		/// <summary>
		/// The registered threads.
		/// </summary>
		static List<Thread*, InlinedAllocation<64>> Threads;

		/// <summary>
		/// The profiling tools usage flag. Can be used to disable profiler. Engine turns it down before the exit and before platform startup.
		/// </summary>
		static bool Enabled;

	public:
		/// <summary>
		/// Determines whether the current (calling) thread is being profiled by the service (it may has no active profile block but is registered).
		/// </summary>
		static bool IsProfilingCurrentThread();

		/// <summary>
		/// Gets the current thread (profiler service shadow object).
		/// </summary>
		static Thread* GetCurrentThread();

		/// <summary>
		/// Begins the event. Call EndEvent with index parameter equal to the returned value by BeginEvent function.
		/// </summary>
		/// <returns>The event token.</returns>
		static int32 BeginEvent();

		/// <summary>
		/// Begins the event. Call EndEvent with index parameter equal to the returned value by BeginEvent function.
		/// </summary>
		/// <param name="name">The event name.</param>
		/// <returns>The event token.</returns>
		static int32 BeginEvent(const Char* name);

		/// <summary>
		/// Begins the event. Call EndEvent with index parameter equal to the returned value by BeginEvent function.
		/// </summary>
		/// <param name="name">The event name.</param>
		/// <returns>The event token.</returns>
		static int32 BeginEvent(const char* name);

		/// <summary>
		/// Ends the event.
		/// </summary>
		/// <param name="index">The event index returned by the BeginEvent method.</param>
		static void EndEvent(int32 index);

		/// <summary>
		/// Ends the last event.
		/// </summary>
		static void EndEvent();

		/// <summary>
		/// Releases resources. Calls to the profiling API after Dispose are not valid.
		/// </summary>
		static void Dispose();
	};

	/// <summary>
	/// Helper structure used to call BeginEvent/EndEvent within single code block.
	/// </summary>
	struct ScopeProfileBlockCPU
	{
		int32 Index;

		inline ScopeProfileBlockCPU(const Char* name)
		{
			Index = ProfilerCPU::BeginEvent(name);
		}

		inline ScopeProfileBlockCPU(const char* name)
		{
			Index = ProfilerCPU::BeginEvent(name);
		}

		inline ~ScopeProfileBlockCPU()
		{
			ProfilerCPU::EndEvent(Index);
		}
	};
}

template<>
struct TIsPODType<SE::ProfilerCPU::Event>
{
    enum { Value = true };
};

#include "ProfilerSrcLoc.h"

// Shortcut macros for profiling a single code block execution on CPU
// Use ZoneTransient for Tracy for code that can be hot-reloaded (eg. in Editor) or if name can be a variable
#define PROFILE_CPU_USE_TRANSIENT_DATA 0

#ifdef SE_PROFILER

#if PROFILE_CPU_USE_TRANSIENT_DATA
#define PROFILE_CPU() ZoneTransient(___tracy_scoped_zone, true); :SE::ScopeProfileBlockCPU ProfileBlockCPU(__FUNCTION__)
#define PROFILE_CPU_NAMED(name) ZoneTransientN(___tracy_scoped_zone, name, true); :SE::ScopeProfileBlockCPU ProfileBlockCPU(name)
#else
#define PROFILE_CPU() ZoneNamed(___tracy_scoped_zone, true); ::SE::ScopeProfileBlockCPU ProfileBlockCPU(__FUNCTION__)
#define PROFILE_CPU_NAMED(name) ZoneNamedN(___tracy_scoped_zone, name, true); ::SE::ScopeProfileBlockCPU ProfileBlockCPU(name)
#endif

#define PROFILE_CPU_SRC_LOC(srcLoc) tracy::ScopedZone ___tracy_scoped_zone( (tracy::SourceLocationData*)&(srcLoc) ); ::SE::ScopeProfileBlockCPU ProfileBlockCPU((srcLoc).name)
#define PROFILE_CPU_ASSET(asset) ZoneScoped; const ::SE::StringView __tracy_asset_name((asset)->GetPath()); ZoneName(*__tracy_asset_name, __tracy_asset_name.Length())
#define PROFILE_CPU_ACTOR(actor) ZoneScoped; const ::SE::StringView __tracy_actor_name((actor)->GetName()); ZoneName(*__tracy_actor_name, __tracy_actor_name.Length())

#else

// Empty macros for disabled profiler
#define PROFILE_CPU()
#define PROFILE_CPU_NAMED(name)
#define PROFILE_CPU_SRC_LOC(srcLoc)
#define PROFILE_CPU_ASSET(asset)
#define PROFILE_CPU_ACTOR(actor)

#endif
