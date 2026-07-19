#pragma once

#include "Runtime/Core/Platform/Platform.h"
#include "Runtime/API.h"
#include "Runtime/Core/Templates.h"

namespace SE
{
	/// <summary>
	/// High-resolution performance counter based on Platform::GetTimeSeconds.
	/// </summary>
	struct Stopwatch
	{
	private:
		double m_Start, m_End;

	public:
		Stopwatch()
		{
			m_Start = m_End = Platform::GetTimeSeconds();
		}

	public:
		// Starts the counter.
		FORCE_INLINE void Start()
		{
			m_Start = Platform::GetTimeSeconds();
		}

		// Stops the counter.
		FORCE_INLINE void Stop()
		{
			m_End = Platform::GetTimeSeconds();
		}

		/// <summary>
		/// Gets the milliseconds time.
		/// </summary>
		FORCE_INLINE int32 GetMilliseconds() const
		{
			return (int32)((m_End - m_Start) * 1000.0);
		}

		/// <summary>
		/// Gets the total number of milliseconds.
		/// </summary>
		FORCE_INLINE double GetTotalMilliseconds() const
		{
			return (float)((m_End - m_Start) * 1000.0);
		}

		/// <summary>
		/// Gets the total number of seconds.
		/// </summary>
		FORCE_INLINE float GetTotalSeconds() const
		{
			return (float)(m_End - m_Start);
		}
	};
}

template<>
struct TIsPODType<SE::Stopwatch>
{
	enum
	{
		Value = true
	};
};
