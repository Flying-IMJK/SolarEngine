#include "DebugDrawingCommands.h"
#include "Core/Tools/Time.h"

//-------------------------------------------------------------------------

#if ENGINE_DEVELOPMENT_TOOLS
namespace SGE::Drawing
{
    void CommandBuffer::Reset( Seconds deltaTime )
    {
        // NAIVE delete version - profile this

        for ( int32 i = (int32) m_pointCommands.Count() - 1; i >= 0; i-- )
        {
            m_pointCommands[i].m_TTL -= deltaTime;
            if ( m_pointCommands[i].m_TTL <= 0.0f )
            {
                m_pointCommands.RemoveAt(i);
            }
        }

        for ( int32 i = (int32) m_lineCommands.Count() - 1; i >= 0; i-- )
        {
            m_lineCommands[i].m_TTL -= deltaTime;
            if ( m_lineCommands[i].m_TTL <= 0.0f )
            {
                m_lineCommands.RemoveAt(i);
            }
        }

        for ( int32 i = (int32) m_triangleCommands.Count() - 1; i >= 0; i-- )
        {
            m_triangleCommands[i].m_TTL -= deltaTime;
            if ( m_triangleCommands[i].m_TTL <= 0.0f )
            {
                m_triangleCommands.RemoveAt(i);
            }
        }

        for ( int32 i = (int32) m_textCommands.Count() - 1; i >= 0; i-- )
        {
            m_textCommands[i].m_TTL -= deltaTime;
            if ( m_textCommands[i].m_TTL <= 0.0f )
            {
                m_textCommands.RemoveAt(i);
            }
        }
    }

    void FrameCommandBuffer::AddThreadCommands( ThreadCommandBuffer const& threadCommands )
    {
        // TODO:
        // Broad-phase culling
        // Sort transparent and depth test off primitives by distance to camera
        // Sort text by font

        m_opaqueDepthOn.Append( threadCommands.GetOpaqueDepthTestEnabledBuffer() );
        m_opaqueDepthOff.Append( threadCommands.GetOpaqueDepthTestDisabledBuffer() );
        m_transparentDepthOn.Append( threadCommands.GetTransparentDepthTestEnabledBuffer() );
        m_transparentDepthOff.Append( threadCommands.GetTransparentDepthTestDisabledBuffer() );
    }
}
#endif