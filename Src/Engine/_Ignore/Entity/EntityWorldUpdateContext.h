#pragma once
#include "Runtime/API.h"
#include "EntityIDs.h"

//-------------------------------------------------------------------------

namespace SE
{
    class EntityWorldSystem;
    class EntityWorld;
	class ISettings;
    namespace Render { class RenderViewport; }
    namespace EntityModel{ class EntityMap; }
    namespace Drawing { class DrawingSystem; class DrawContext; }
    namespace Reflect { class TypeInfo; }

    //-------------------------------------------------------------------------

    class SE_API_RUNTIME EntityWorldUpdateContext
    {
    public:

        EntityWorldUpdateContext(EntityWorld* pWorld );

        // Get the time scaling for the current world
        float GetTimeScale() const;

        // Get the world ID - threadsafe
        EntityWorldID const& GetWorldID() const;

        // Get an entity world system - threadsafe since these never changed during the lifetime of a world
        template<typename T> inline T* GetWorldSystem() const
        {
            static_assert( std::is_base_of<SE::EntityWorldSystem, T>::value, "T is not derived from IEntityWorldSystem" );
            return Cast<T>( GetWorldSystem( T::s_entitySystemID ) );
        }

        // Get the world type - threadsafe since this never changes
        inline bool IsGameWorld() const { return m_isGameWorld; }

        // Is the world paused? i.e. time scale <= 0.0f;
        inline bool IsWorldPaused() const { return m_isPaused; }

        // Get the persistent map - threadsafe - all dynamic entity creation is done in this map
        EntityModel::EntityMap* GetPersistentMap() const;

        // Get the viewport for this world
//        Render::RenderViewport const* GetViewport() const;

        // Get the debug drawing context for this world - threadsafe
        #ifdef SE_DEVELOPMENT
        Drawing::DrawContext GetDrawingContext() const;
        Drawing::DrawingSystem* GetDebugDrawingSystem() const;
        #endif

        // Get world settings
        template<typename T> T* GetSettings() { return TryCast<T>( GetSettings( T::s_pTypeInfo ) ); }

        // Get world settings
        template<typename T> T const* GetSettings() const { return TryCast<T>( const_cast<EntityWorldUpdateContext*>( this )->GetSettings( T::s_pTypeInfo ) ); }

    private:

        // Threadsafe since these never changed during the lifetime of a world
        EntityWorldSystem* GetWorldSystem( uint32_t worldSystemID ) const;

        // Delete any ability to copy this struct
        explicit EntityWorldUpdateContext( EntityWorldUpdateContext const& ) = delete;
        explicit EntityWorldUpdateContext( EntityWorldUpdateContext&& ) = delete;
        EntityWorldUpdateContext& operator=( EntityWorldUpdateContext const& ) = delete;
        EntityWorldUpdateContext& operator=( EntityWorldUpdateContext&& ) = delete;

        ISettings* GetSettings( TypeInfo const* pTypeInfo );

    private:

        EntityWorld*                    m_pWorld = nullptr;
        bool                            m_isGameWorld = true;
        bool                            m_isPaused = false;
    };
}