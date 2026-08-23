#pragma once

#include "DebugView.h"

#ifdef SE_DEVELOPMENT
namespace SGE
{
    class ResourceSystem;
    //-------------------------------------------------------------------------

    class SE_API_RUNTIME ResourceDebugView : public DebugView
    {
        SE_CLASS(ResourceDebugView, DebugView)

    public:

        static void DrawRequestHistory( ResourceSystem* pResourceSystem );
        static void DrawResourceSystemOverview( ResourceSystem* pResourceSystem );

    public:

        ResourceDebugView() : DebugView(SE_TEXT("System/Resource") ) {}

    private:

        virtual void Initialize( Systems const& systemRegistry/*, EntityWorld const* pWorld */) override;
        virtual void Shutdown() override;
        virtual void DrawMenu( EntityWorldUpdateContext const& context ) override;

    private:

        ResourceSystem*         m_pResourceSystem = nullptr;
    };
}
#endif