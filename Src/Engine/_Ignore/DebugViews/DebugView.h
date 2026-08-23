#pragma once

#include "Core/TypeSystem/IReflectedType.h"
#include "Core/Types/SGUID.h"
#include "Core/Types/Delegate.h"
#include "Core/Math/Math.h"
#include "Runtime/API.h"

//-------------------------------------------------------------------------

struct ImGuiWindowClass;

//-------------------------------------------------------------------------
// Runtime Debug View
//-------------------------------------------------------------------------
// Provides debug info and tools for the game world
// Each view is drawn at the end of each fame

#ifdef SE_DEVELOPMENT
namespace SGE
{
    class Systems;
    class EntityWorld;
    class EntityWorldUpdateContext;

    //-------------------------------------------------------------------------

    class SE_API_RUNTIME DebugView : public IReflectedType
    {
        SE_CLASS( DebugView, IReflectedType)

        friend class EngineDebugUI;

    public:

        struct Window
        {
			Window() = default;

            Window( String const& name, Function<void(/*EntityWorldUpdateContext const&, */bool, uint64)>&& func, float alpha = 1.f )
                : m_name( name )
                , m_drawFunction( func )
                , m_alpha( Math::Clamp( alpha, 0.0f, 1.0f ) )
            {
                ENGINE_ASSERT( !m_name.IsEmpty() );
            }

            Window( char const* pName, Function<void(/*EntityWorldUpdateContext const&,*/ bool, uint64)>&& func, float alpha = 1.f )
                : m_name( pName )
                , m_drawFunction( func )
                , m_alpha( Math::Clamp( alpha, 0.0f, 1.0f ) )
            {
                ENGINE_ASSERT( pName != nullptr );
            }

        public:

            // The window name
            String                                                                   m_name;

            // The function to call to draw this window. Args: EntityWorldUpdateContext const& context, bool isFocused
			Function<void(/*EntityWorldUpdateContext const&,*/ bool, uint64)>        m_drawFunction;

            // Window Transparency
            float                                                                    m_alpha = 1.0f;

            // Is the window currently open
            bool                                                                     m_isOpen = false;

            // Optional: Window Type Info - used by debug views to identify different windows by type
            StringID                                                                 m_typeID;

            // Optional: User data - Passed to the drawing function
            uint64                                                                   m_userData = 0;
        };

    public:

        DebugView() = default;
        DebugView( DebugView const& ) = default;
        DebugView( String const& menuPath ) : m_menuPath( menuPath ) {}
        virtual ~DebugView() 
        { 
            //ENGINE_ASSERT( m_pWorld == nullptr ); 
        }

        DebugView& operator=( DebugView const& rhs ) = default;

        inline bool HasMenu() const { return !m_menuPath.IsEmpty(); }

        virtual void Initialize( Systems const& systemRegistry/*, EntityWorld const* pWorld */);
        virtual void Shutdown();

    protected:

        // Called at the end of the frame before we draw any windows
        virtual void Update( EntityWorldUpdateContext const& context ) {}

        // Called to draw the view's menu
        virtual void DrawMenu( EntityWorldUpdateContext const& context ) {};

        // Called within the context of a large overlay window allowing you to draw helpers and widgets over a viewport
        virtual void DrawOverlayElements( EntityWorldUpdateContext const& context ) {}

        // Check if a window with this typeID exists
        Window* GetDebugWindow( StringID typeID );

        // Check if a window with this userdata exists
        Window* GetDebugWindow( uint64 userData );

        // Check if a window with this typeID and userdata exists
        Window* GetDebugWindow( StringID typeID, uint64 userData );

        // Check if a window with this typeID exists
        inline bool HasDebugWindow( StringID typeID ) { return GetDebugWindow( typeID ) != nullptr; }

        // Check if a window with this userdata exists
        inline bool HasDebugWindow( uint64 userData ) { return GetDebugWindow( userData ) != nullptr; }

        // Check if a window with this typeID and userdata exists
        inline bool HasDebugWindow( StringID typeID, uint64 userData ) { return GetDebugWindow( typeID, userData ) != nullptr; }

        // Called before we hot-reload anything to allow you to unload any resource that need to be reloaded
        virtual void HotReload_UnloadResources(Function<ResourceRequesterID> const& usersToReload, List<ResID> const& resourcesToBeReloaded ) {}

        // Called once all unloads are completed - user can reload their desired resources here
        virtual void HotReload_ReloadResources() {}

    protected:

        // EntityWorld const*      m_pWorld = nullptr;

        // Defines the name and placement of the menu and are separated using '/' (e.g. "Input/Controller Utils")
        String                m_menuPath;

        // All the windows that this view has
		List<Window>          m_windows;
    };
}
#endif