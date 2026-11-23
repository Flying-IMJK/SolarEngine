#pragma once

#include <Runtime/Core/Scripting/ManagedCLR/CLRDomain.h>
#include <Runtime/Core/Scripting/Scripting.h>
#include <Runtime/Core/Thread/ThreadPool.h>
#include <Runtime/Engine.h>

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
            Scripting::GetScriptsDomain()->Dispatch();
            while (Platform::AtomicRead(&ExitFlag) == 0)
            {
                // OnRender owns the GPU lock and draws the managed GUI as part of the frame.
                Engine::OnRender();
                Platform::Sleep(20);
            }
            return true;
        }
    };
}
