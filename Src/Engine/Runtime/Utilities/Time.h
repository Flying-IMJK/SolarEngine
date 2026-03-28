#pragma once

#include "../API.h"
#include "../../Core/Types/TimeSpan.h"

namespace SE
{
	class Engine;

	class SE_API_RUNTIME Time
	{
	public:
		friend class Engine;

		/// <summary>
		/// Engine subsystem updating data.
		/// Used to invoke game logic updates, physics updates and rendering with possibly different frequencies.
		/// </summary>
		class SE_API_RUNTIME TickData
		{
		public:
			virtual ~TickData() = default;

			/// <summary>
			/// The total amount of tick since start.
			/// </summary>
			uint64 TicksCount = 0;

			/// <summary>
			/// The last tick start time (gathered from PlatformTime::Seconds)
			/// </summary>
			double LastBegin;

			/// <summary>
			/// The last tick end time (gathered from PlatformTime::Seconds)
			/// </summary>
			double LastEnd;

			/// <summary>
			/// The last tick length in seconds. Note: LastEnd-LastBegin may be invalid inside a tick but LastLength is always valid.
			/// </summary>
			double LastLength;

			/// <summary>
			/// The next tick start time.
			/// </summary>
			double NextBegin;

			/// <summary>
			/// The delta time.
			/// Always returns the delta time for the current event, meaning it can be used in Update, FixedUpdate, Draw, etc.
			/// </summary>
			TimeSpan DeltaTime;

			/// <summary>
			/// The total time.
			/// </summary>
			TimeSpan Time;

			/// <summary>
			/// The unscaled delta time.
			/// </summary>
			TimeSpan UnscaledDeltaTime;

			/// <summary>
			/// The unscaled total time.
			/// </summary>
			TimeSpan UnscaledTime;

		public:
			virtual void Synchronize(float targetFps, double currentTime);
			virtual void OnReset(float targetFps, double currentTime);
			virtual bool OnTickBegin(double time, float targetFps, float maxDeltaTime);
			virtual void OnTickEnd();

		protected:
			void Advance(double time, double deltaTime);
		};
		
	public:
		/// <summary>
		/// The time at which the game started (UTC local).
		/// </summary>
		static DateTime StartupTime;

		/// <summary>
		/// The target amount of the game logic updates per second (script updates frequency).
		/// </summary>
		static float UpdateFPS;

		/// <summary>
		/// The target amount of the frames rendered per second (target game FPS).
		/// </summary>
		/// <remarks>
		/// To get the actual game FPS use <see cref="Engine.FramesPerSecond"/>
		/// </remarks>
		static float DrawFPS;

		/// <summary>
		/// The game time scale factor. Default is 1.
		/// </summary>
		static float TimeScale;

	public:
		/// <summary>
		/// The game logic updating data.
		/// </summary>
		static TickData Update;

		/// <summary>
		/// The rendering data.
		/// </summary>
		static TickData Render;

		/// <summary>
		/// The current tick data (update, physics or draw).
		/// </summary>
		static TickData* Current;

	public:

		/// <summary>
		/// Gets time in seconds it took to complete the last frame, <see cref="TimeScale"/> dependent.
		/// </summary>
		static float GetDeltaTime();

		/// <summary>
		/// Gets time at the beginning of this frame. This is the time in seconds since the start of the game.
		/// </summary>
		static float GetGameTime();

		/// <summary>
		/// Gets timeScale-independent time in seconds it took to complete the last frame.
		/// </summary>
		static float GetUnscaledDeltaTime();

		/// <summary>
		/// Gets timeScale-independent time at the beginning of this frame. This is the time in seconds since the start of the game.
		/// </summary>
		static float GetUnscaledGameTime();

		/// <summary>
		/// Gets the time since startup in seconds (unscaled).
		/// </summary>
		static float GetTimeSinceStartup();

		/// <summary>
		/// Gets the time of next upcoming tick data of ticking group with defined update frequency.
		/// </summary>
		/// <returns>The time of next tick.</returns>
		static double GetNextTick();

		/// <summary>
		/// Synchronizes update, fixed update and draw. Resets any pending deltas for fresh ticking in sync.
		/// </summary>
		static void Synchronize();

	private:
		static bool OnBeginUpdate(double time);
		static bool OnBeginDraw(double time);

		static void OnEndUpdate();
		static void OnEndRender();
	};
}


