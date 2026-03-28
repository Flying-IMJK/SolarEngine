
#include "Time.h"

#include "../../Core/Math/Math.h"
#include "../../Core/Types/DateTime.h"

namespace SE
{
	namespace
	{
		bool FixedDeltaTimeEnable;
		float FixedDeltaTimeValue;
		float MaxUpdateDeltaTime = 0.1f;
	}

	DateTime Time::StartupTime;
	float Time::UpdateFPS = 60.0f;
	float Time::DrawFPS = 120.0f;
	float Time::TimeScale = 1.0f;
	Time::TickData Time::Update;
	Time::TickData Time::Render;
	Time::TickData* Time::Current = nullptr;

	void Time::TickData::Synchronize(float targetFps, double currentTime)
	{
		Time = UnscaledTime = TimeSpan::Zero();
		DeltaTime = UnscaledDeltaTime = targetFps > Math::ZeroTolerance ? TimeSpan::FromSeconds(1.0f / targetFps) : TimeSpan::Zero();
		LastLength = static_cast<double>(DeltaTime.Ticks) / TimeSpan::TicksPerSecond;
		LastBegin = currentTime - LastLength;
		LastEnd = currentTime;
		NextBegin = targetFps > Math::ZeroTolerance ? LastBegin + (1.0f / targetFps) : 0.0;
	}

	void Time::TickData::OnReset(float targetFps, double currentTime)
	{
		DeltaTime = UnscaledDeltaTime = targetFps > Math::ZeroTolerance ? TimeSpan::FromSeconds(1.0f / targetFps) : TimeSpan::Zero();
		LastLength = static_cast<double>(DeltaTime.Ticks) / TimeSpan::TicksPerSecond;
		LastBegin = currentTime - LastLength;
		LastEnd = currentTime;
	}

	bool Time::TickData::OnTickBegin(double time, float targetFps, float maxDeltaTime)
	{
		// Check if can perform a tick
		double deltaTime;
		if (FixedDeltaTimeEnable)
		{
			deltaTime = (double)FixedDeltaTimeValue;
		}
		else
		{
			if (time < NextBegin)
				return false;

			deltaTime = Math::Max((time - LastBegin), 0.0);
			if (deltaTime > maxDeltaTime)
			{
				deltaTime = (double)maxDeltaTime;
				NextBegin = time;
			}

			if (targetFps > Math::ZeroTolerance)
			{
				int skip = (int)(1 + (time - NextBegin) * targetFps);
				NextBegin += (1.0 / targetFps) * skip;
			}
		}

		// Update data
		Advance(time, deltaTime);

		return true;
	}

	void Time::TickData::OnTickEnd()
	{
		const double time = Platform::GetTimeSeconds();
		LastEnd = time;
		LastLength = time - LastBegin;
	}

	void Time::TickData::Advance(double time, double deltaTime)
	{
		float timeScale = TimeScale;
		/*if (_gamePaused)
			timeScale = 0.0f;*/
		LastBegin = time;
		UnscaledDeltaTime = TimeSpan::FromSeconds(deltaTime);
		UnscaledTime += UnscaledDeltaTime;
		DeltaTime = TimeSpan::FromSeconds(deltaTime * (double)timeScale);
		Time += DeltaTime;
		TicksCount++;
	}


	void Time::Synchronize()
	{
		// Initialize tick data (based on a time settings)
		const double time = Platform::GetTimeSeconds();
		Update.Synchronize(UpdateFPS, time);
		Render.Synchronize(DrawFPS, time);
	}

	bool Time::OnBeginUpdate(double time)
	{
		if (Update.OnTickBegin(time, UpdateFPS, MaxUpdateDeltaTime))
		{
			Current = &Update;
			return true;
		}
		return false;
	}

	bool Time::OnBeginDraw(double time)
	{
		if (Render.OnTickBegin(time, DrawFPS, 1.0f))
		{
			Current = &Render;
			return true;
		}
		return false;
	}

	void Time::OnEndUpdate()
	{
		Update.OnTickEnd();
		Current = nullptr;
	}

	void Time::OnEndRender()
	{
		Render.OnTickEnd();
		Current = nullptr;
	}

	float Time::GetDeltaTime()
	{
		auto* data = Current ? Current : &Update;
		return data->DeltaTime.GetTotalSeconds();
	}
	float Time::GetGameTime()
	{
		auto* data = Current ? Current : &Update;
		return data->Time.GetTotalSeconds();
	}

	float Time::GetUnscaledDeltaTime()
	{
		auto* data = Current ? Current : &Update;
		return data->UnscaledDeltaTime.GetTotalSeconds();
	}

	float Time::GetUnscaledGameTime()
	{
		auto* data = Current ? Current : &Update;
		return data->UnscaledTime.GetTotalSeconds();
	}

	float Time::GetTimeSinceStartup()
	{
		return (DateTime::Now() - StartupTime).GetTotalSeconds();
	}

	double Time::GetNextTick()
	{
		const double nextUpdate = Time::Update.NextBegin;
		const double nextRender = Time::Render.NextBegin;

		double nextTick = Max_double;
		if (UpdateFPS > Math::ZeroTolerance && nextUpdate < nextTick)
		{
			nextTick = nextUpdate;
		}
		if (DrawFPS > Math::ZeroTolerance && nextRender < nextTick)
		{
			nextTick = nextRender;
		}

		if (nextTick == Max_double)
		{
			return 0.0;
		}
		return nextTick;
	}
}
