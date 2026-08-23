#include "EntityWorldManager.h"
#include "EntityWorld.h"
#include "EntityLog.h"
//#include "Engine/Player/Systems/WorldSystem_PlayerManager.h"
//#include "Engine/Camera/Systems/WorldSystem_CameraManager.h"
//#include "Engine/Camera/Components/Component_Camera.h"
#include "Core/TypeSystem/Types.h"
#include "Core/Systems.h"

//-------------------------------------------------------------------------

namespace SE
{
    EntityWorldManager::~EntityWorldManager()
    {
        ENGINE_ASSERT( m_worlds.IsEmpty() && m_worldSystemTypeInfos.IsEmpty() );
    }

    void EntityWorldManager::Initialize( Systems const& systemsRegistry )
    {
        m_pSystemsRegistry = &systemsRegistry;

        //-------------------------------------------------------------------------
        m_worldSystemTypeInfos = Types::GetAllDerivedTypes( EntityWorldSystem::GetStaticTypeID(), false, false, true );

        // Create a game world
        //-------------------------------------------------------------------------

        CreateWorld( EntityWorldType::Game );
    }

    void EntityWorldManager::Shutdown()
    {
        for ( auto& pWorld : m_worlds )
        {
            pWorld->Shutdown();
            Delete( pWorld );
        }
        m_worlds.Clear();

        //-------------------------------------------------------------------------

        m_worldSystemTypeInfos.Clear();
        m_pSystemsRegistry = nullptr;
    }

    //-------------------------------------------------------------------------

    void EntityWorldManager::StartFrame()
    {
        #ifdef SE_DEVELOPMENT
        for ( auto& pWorld : m_worlds )
        {
            pWorld->ResetDebugDrawingSystem();
        }
        #endif
    }

    void EntityWorldManager::EndFrame()
    {
        // Do Nothing
    }

    //-------------------------------------------------------------------------

    EntityWorld* EntityWorldManager::GetGameWorld()
    {
        for ( auto const& pWorld : m_worlds )
        {
            if ( pWorld->IsGameWorld() )
            {
                return pWorld;
            }
        }

        return nullptr;
    }

    EntityWorld* EntityWorldManager::CreateWorld( EntityWorldType worldType )
    {
        ENGINE_ASSERT( m_pSystemsRegistry != nullptr );

        //-------------------------------------------------------------------------

        // Only a single game world is allowed
        if ( worldType == EntityWorldType::Game )
        {
            if ( GetGameWorld() != nullptr )
            {
                ENGINE_UNREACHABLE_CODE();
                return nullptr;
            }
        }

        //-------------------------------------------------------------------------

        auto pNewWorld = New<EntityWorld>( worldType );
        pNewWorld->Initialize( *m_pSystemsRegistry, m_worldSystemTypeInfos );
        m_worlds.Add( pNewWorld );

        //-------------------------------------------------------------------------

        #ifdef SE_DEVELOPMENT
        if ( worldType == EntityWorldType::Game )
        {
            pNewWorld->SetDebugName( "Game" );
        }
        else
        {
            pNewWorld->SetDebugName( "Editor" );
        }
        #endif

        return pNewWorld;
    }

    void EntityWorldManager::DestroyWorld( EntityWorld* pWorld )
    {
        ENGINE_ASSERT( Threading::IsMainThread() );

        // Remove world from worlds list
        m_worlds.Remove( pWorld );

        // Shutdown and destroy world
        pWorld->Shutdown();
        Delete( pWorld );
    }

    //-------------------------------------------------------------------------

    bool EntityWorldManager::IsBusyLoading() const
    {
        for ( auto const& pWorld : m_worlds )
        {
            if ( pWorld->IsBusyLoading() )
            {
                return true;
            }
        }

        return false;
    }

    void EntityWorldManager::UpdateLoading()
    {
        for ( auto const& pWorld : m_worlds )
        {
            pWorld->UpdateLoading();
        }
    }

    void EntityWorldManager::UpdateWorlds( UpdateContext const& context )
    {
        //-------------------------------------------------------------------------
        // World Update
        //-------------------------------------------------------------------------

        for ( auto const& pWorld : m_worlds )
        {
            if ( pWorld->IsSuspended() )
            {
                continue;
            }

            // Run world updates
            //-------------------------------------------------------------------------

            pWorld->Update();

            // Update world view
            //-------------------------------------------------------------------------

			/*if ( pWorld->GetViewport() != nullptr )
			{
				auto pViewport = pWorld->GetViewport();
				auto pCameraManager = pWorld->GetWorldSystem<CameraManager>();
				if ( pCameraManager->HasActiveCamera() )
				{
					auto pActiveCamera = pCameraManager->GetActiveCamera();

					// Update camera view dimensions if they differ (needed when we resize the viewport even if the camera hasn't updated)
					if ( pViewport->GetDimensions() != pActiveCamera->GetViewVolume().GetViewDimensions() )
					{
						pActiveCamera->UpdateViewDimensions( pViewport->GetDimensions() );
						pViewport->SetViewVolume( pActiveCamera->GetViewVolume() );
					}

					// Update world view volume only if camera has been updated
					if ( pActiveCamera->ShouldReflectViewVolume() )
					{
						Render::ViewVolume const& cameraViewVolume = pActiveCamera->ReflectViewVolume();
						pViewport->SetViewVolume( cameraViewVolume );
					}
				}
            }*/
        }

        //-------------------------------------------------------------------------
        // Handle Entity Log
        //-------------------------------------------------------------------------
#ifdef SE_DEVELOPMENT
		auto queuedLogRequests = EntityModel::RetrieveQueuedLogRequests();
		for ( auto const& request : queuedLogRequests )
		{
			// Resolve all IDs
			//-------------------------------------------------------------------------

			EntityWorld const* pFoundWorld = nullptr;
			Entity const* pFoundEntity = nullptr;
			EntityComponent const* pFoundComponent = nullptr;

			for ( auto pWorld : m_worlds )
			{
				pFoundEntity = pWorld->FindEntity( request.m_entityID );
				if ( pFoundEntity != nullptr )
				{
					pFoundWorld = pWorld;
					if ( request.m_componentID.IsValid() )
					{
						pFoundComponent = pFoundEntity->FindComponent( request.m_componentID );
					}
					break;
				}
			}

			ENGINE_ASSERT( pFoundWorld != nullptr );

			// Create source info
			//-------------------------------------------------------------------------

			String sourceInfoStr;

			if ( pFoundComponent == nullptr )
			{
				sourceInfoStr = String::Format(SE_TEXT("W: {0}, E: {1}"), pFoundWorld->GetDebugName().Get(),
					pFoundEntity->GetNameID().ToString(), request.m_message);
			}
			else
			{
				sourceInfoStr = String::Format( SE_TEXT("W: {0}, E: {1}, C: {2}"),
					pFoundWorld->GetDebugName().Get(), pFoundEntity->GetNameID().ToString(), pFoundComponent->GetNameID().ToString(),
					request.m_message);
			}

			// Add Log
			//-------------------------------------------------------------------------

			Log::AddEntry(request.m_severity, request.m_category.Get(), request.m_filename.Get(),
				request.m_lineNumber, sourceInfoStr.Get());
		}
#endif
    }

    //-------------------------------------------------------------------------

    #ifdef SE_DEVELOPMENT
    void EntityWorldManager::HotReload_UnloadEntities( List<ResourceRequesterID> const& usersToReload )
    {
        for ( auto const& pWorld : m_worlds )
        {
            pWorld->HotReload_UnloadEntities( usersToReload );
        }
    }

    void EntityWorldManager::HotReload_ReloadEntities()
    {
        for ( auto const& pWorld : m_worlds )
        {
            pWorld->HotReload_ReloadEntities();
        }
    }
    #endif
} 