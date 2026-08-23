#pragma once

#include "Runtime/API.h"
#include "DebugDrawing.h"
#include "Core/Thread/Threading.h"

//-------------------------------------------------------------------------

#if ENGINE_DEVELOPMENT_TOOLS
namespace SGE::Drawing
{
    class SE_API_RUNTIME DrawingSystem
    {

    public:

        DrawingSystem() = default;
        ~DrawingSystem();

        // Empty all per thread buffers
        void Reset();

        // Returns a per-thread drawing context, this removes the need for constantly calling get thread command buffer
        inline DrawContext GetDrawingContext() { return DrawContext( GetThreadCommandBuffer() ); }

        // Reflects all the individual per-thread buffers into a single supplied frame command buffer. Clears all thread buffers.
        void ReflectFrameCommandBuffer(Seconds const deltaTime, FrameCommandBuffer& reflectedFrameCommands );

    private:

        ThreadCommandBuffer& GetThreadCommandBuffer();

    private:

        List<ThreadCommandBuffer*>       m_threadCommandBuffers;
        CriticalSection                  m_commandBufferMutex;
    };
}
#endif