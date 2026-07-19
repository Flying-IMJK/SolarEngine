#pragma once

#include "Runtime/Core/Thread/ThreadPool.h"

namespace SE
{
	/// <summary>
	/// Async DoDragDrop helper (used for rendering frames during main thread stall).
	/// </summary>
	class DoDragDropJob : public Threading::ThreadPoolTask
	{
	public:
		int64 ExitFlag = 0;

		// [ThreadPoolTask]
		bool Run() override
		{
//			Scripting::GetScriptsDomain()->Dispatch();
			while (Platform::AtomicRead(&ExitFlag) == 0)
			{
#if SE_EDITOR
				// Flush any single-frame shapes to prevent memory leaking (eg. via terrain collision debug during scene drawing with PhysicsColliders or PhysicsDebug flag)
//            	DebugDraw::UpdateContext(nullptr, 0.0f);
#endif
//				Engine::OnDraw();
				Platform::Sleep(20);
			}
			return false;
		}
	};
}
