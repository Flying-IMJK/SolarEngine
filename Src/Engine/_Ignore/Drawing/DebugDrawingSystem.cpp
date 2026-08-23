#include "DebugDrawingSystem.h"

#include "Core/Types/TimeSpan.h"
#include "Core/Memory/Memory.h"
//-------------------------------------------------------------------------

#if ENGINE_DEVELOPMENT_TOOLS
namespace SGE::Drawing
{
    ThreadCommandBuffer& DrawingSystem::GetThreadCommandBuffer()
    {
        Threading::ScopeLock Lock( m_commandBufferMutex );

        auto const threadID = Threading::GetCurrentThreadID();

        // Check for an already created buffer for this thread
        for ( auto& pThreadBuffer : m_threadCommandBuffers )
        {
            if ( pThreadBuffer->GetThreadID() == threadID )
            {
                return *pThreadBuffer;
            }
        }

        // Create a new buffer
        auto pThreadBuffer = New<ThreadCommandBuffer>( threadID );
		m_threadCommandBuffers.Add(pThreadBuffer);
        return *pThreadBuffer;
    }

    DrawingSystem::~DrawingSystem()
    {
        for ( auto& pBuffer : m_threadCommandBuffers )
        {
            Delete( pBuffer );
        }
    }

    void DrawingSystem::ReflectFrameCommandBuffer(Seconds const deltaTime, FrameCommandBuffer& reflectedFrameCommands )
    {
        // Reset the frame buffer for a new frame, flush old commands and only keep ones with a valid TTL
        reflectedFrameCommands.Reset( deltaTime );

        // Reflect all the new commands into the frame buffer
        Threading::ScopeLock Lock( m_commandBufferMutex );
        for ( auto& pThreadBuffer : m_threadCommandBuffers )
        {
            reflectedFrameCommands.AddThreadCommands( *pThreadBuffer );
            pThreadBuffer->Clear();
        }
    }

    void DrawingSystem::Reset()
    {
        Threading::ScopeLock Lock( m_commandBufferMutex );
        for ( auto& pThreadBuffer : m_threadCommandBuffers )
        {
            pThreadBuffer->Clear();
        }
    }
}
#endif