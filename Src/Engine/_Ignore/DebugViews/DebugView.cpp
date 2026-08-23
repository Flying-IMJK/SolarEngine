#include "DebugView.h"
// #include "Runtime/Entity/EntityWorld.h"

//-------------------------------------------------------------------------

#ifdef SE_DEVELOPMENT
namespace SGE
{
    void DebugView::Initialize(Systems const& systemRegistry/*, EntityWorld const* pWorld*/)
    {
        // m_pWorld = pWorld;
        // ENGINE_ASSERT(m_pWorld != nullptr && m_pWorld->IsGameWorld());
    }

    void DebugView::Shutdown()
    {
        m_windows.Clear();
        // m_pWorld = nullptr;
    }

    DebugView::Window* DebugView::GetDebugWindow( StringID typeID )
    {
        for ( auto& window : m_windows )
        {
            if ( window.m_typeID == typeID )
            {
                return &window;
            }
        }

        return nullptr;
    }

    DebugView::Window* DebugView::GetDebugWindow( uint64_t userData )
    {
        for ( auto& window : m_windows )
        {
            if ( window.m_userData == userData )
            {
                return &window;
            }
        }

        return nullptr;
    }

    DebugView::Window* DebugView::GetDebugWindow( StringID typeID, uint64_t userData )
    {
        for ( auto& window : m_windows )
        {
            if ( window.m_typeID == typeID && window.m_userData == userData )
            {
                return &window;
            }
        }

        return nullptr;
    }
}
#endif