#include "ResourceSystem.h"
#include "Runtime/Resource/ResourceProvider.h"
#include "Runtime/Resource/ResourceRequest.h"
#include "Runtime/Resource/ResourceLoader.h"
#include "Runtime/Render/Assets/TextureBase.h"
#include "Runtime/Engine.h"
#include "Core/Profiler/Profiler.h"
#include "Core/Types/Collections/ListExtensions.h"


namespace SGE
{
	ENGINE_SYSTEM_REGISTER(ResourceSystem)

    ResourceSystem::ResourceSystem() : m_asyncProcessingTask() , ISystem(SE_TEXT("ResourceSystem"), -100)
    {

    }

    ResourceSystem::~ResourceSystem()
    {
        ENGINE_ASSERT(m_pResourceProvider == nullptr && !IsBusy() && m_resourceRecords.IsEmpty());
    }

    ResourceGlobalSettings const &ResourceSystem::GetSettings() const
    {
        return m_pResourceProvider->GetSettings();
    }


    bool ResourceSystem::IsBusy() const
    {
        if (m_isAsyncTaskRunning)
        {
            return true;
        }

        if (!m_activeRequests.IsEmpty())
        {
            return true;
        }

        if (!m_pendingRequests.IsEmpty())
        {
            return true;
        }

        return false;
    }

    //-------------------------------------------------------------------------

    void ResourceSystem::GetUsersForResource(ResRecord const *pResourceRecord, List<ResourceRequesterID> &userIDs) const
    {
        ENGINE_ASSERT(pResourceRecord != nullptr);
        Threading::ScopeLock lock(m_accessLock);

        for (auto const &requesterID : pResourceRecord->m_References)
        {
            // Internal user i.e. install dependency
            if (requesterID.IsInstallDependencyRequest())
            {
				ResID const resourceID = requesterID.GetInstallDependencyResourcePathID();

				auto recordIter = m_resourceRecords.begin();
				for (;recordIter != m_resourceRecords.end(); ++recordIter)
				{
					if (recordIter->Value->GetResourceID() == resourceID)
					{
						break;
					}
				}
                ENGINE_ASSERT(recordIter != m_resourceRecords.end());

                ResRecord *pFoundRecord = recordIter->Value;
                GetUsersForResource(pFoundRecord, userIDs);
            }
            else // Actual external user
            {
                // Skip manual requests
/*                if (requesterID.IsManualRequest())
                {
                    continue;
                }*/

                // Add unique users to the list
				userIDs.AddUnique(requesterID);
            }
        }
    }

    //-------------------------------------------------------------------------

    void ResourceSystem::RegisterResourceLoader(ResourceLoader *pLoader)
    {
        auto &loadableTypes = pLoader->GetLoadableTypes();
        for (auto &type : loadableTypes)
        {
            auto loaderIter = m_resourceLoaders.Find(type);
            ENGINE_ASSERT(loaderIter == m_resourceLoaders.end()); // Catch duplicate registrations
            m_resourceLoaders[type] = pLoader;
        }
    }

    void ResourceSystem::UnregisterResourceLoader(ResourceLoader *pLoader)
    {
        auto &loadableTypes = pLoader->GetLoadableTypes();
        for (auto &type : loadableTypes)
        {
            auto loaderIter = m_resourceLoaders.Find(type);
            ENGINE_ASSERT(loaderIter != m_resourceLoaders.end()); // Catch invalid unregistrations
            m_resourceLoaders.Remove(loaderIter);
        }
    }

    //-------------------------------------------------------------------------

    ResRecord *ResourceSystem::FindOrCreateResourceRecord(ResID const &resourceID)
    {
        ENGINE_ASSERT(resourceID.IsValid());
        Threading::ScopeLock lock(m_accessLock);

        ResRecord *pRecord = nullptr;
        auto const recordIter = m_resourceRecords.Find(resourceID);
        if (recordIter == m_resourceRecords.end())
        {
            pRecord = New<ResRecord>(resourceID);
            m_resourceRecords[resourceID] = pRecord;
        }
        else
        {
            pRecord = recordIter->Value;
        }

        return pRecord;
    }

    ResRecord *ResourceSystem::FindExistingResourceRecord(ResID const &resourceID)
    {
        ENGINE_ASSERT(resourceID.IsValid());
        Threading::ScopeLock lock(m_accessLock);

        auto const recordIter = m_resourceRecords.Find(resourceID);
        ENGINE_ASSERT(recordIter != m_resourceRecords.end());
        return recordIter->Value;
    }

    void ResourceSystem::LoadResource(ResPtr &resourcePtr, ResourceRequesterID const &requesterID)
    {
        Threading::ScopeLock lock(m_accessLock);

        // Immediately update the resource ptr
        auto pRecord = FindOrCreateResourceRecord(resourcePtr.GetResourceID());
        resourcePtr.m_pResourceRecord = pRecord;

        //-------------------------------------------------------------------------

        if (!pRecord->HasReferences())
        {
            AddPendingRequest(PendingRequest(PendingRequest::Type::Load, pRecord, requesterID));
        }

        pRecord->AddReference(requesterID);
    }

    void ResourceSystem::UnloadResource(ResPtr &resourcePtr, ResourceRequesterID const &requesterID)
    {
        Threading::ScopeLock lock(m_accessLock);

        // Immediately update the resource ptr
        resourcePtr.m_pResourceRecord = nullptr;

        //-------------------------------------------------------------------------

        auto pRecord = FindExistingResourceRecord(resourcePtr.GetResourceID());
        pRecord->RemoveReference(requesterID);

        if (!pRecord->HasReferences())
        {
            AddPendingRequest(PendingRequest(PendingRequest::Type::Unload, pRecord, requesterID));
        }
    }

    void ResourceSystem::AddPendingRequest(PendingRequest &&request)
    {
        Threading::ScopeLock lock(m_accessLock);

		Function<bool(const PendingRequest &)> predicate = [request](PendingRequest const &value)
        { 
            return value.m_pRecord->GetResourceID() == request.m_pRecord->GetResourceID();
        };
        
        int32 const foundIdx = ListExtensions::IndexOf(m_pendingRequests, predicate);

        // If we dont have a request for this resource ID create one
        if (foundIdx == INVALID_INDEX)
        {
            m_pendingRequests.Add(MoveTemp(request));
        }
        else // Overwrite exiting request - we deal with whether we have to register a task or not in the update
        {
            m_pendingRequests[foundIdx] = request;
        }
    }

    ResourceRequest *ResourceSystem::TryFindActiveRequest(ResRecord const *pResourceRecord) const
    {
        ENGINE_ASSERT(pResourceRecord != nullptr);
        ENGINE_ASSERT(!m_isAsyncTaskRunning);

        Threading::ScopeLock lock(m_accessLock);
        Function<bool(ResourceRequest * const&)> predicate = [pResourceRecord](ResourceRequest * const&pRequest)
        { 
            return pRequest->GetResourceRecord() == pResourceRecord; 
        };

        int32 const foundIdx = ListExtensions::IndexOf(m_activeRequests, predicate);

        if (foundIdx != -1)
        {
            return m_activeRequests[foundIdx];
        }

        return nullptr;
    }

    //-------------------------------------------------------------------------

    void ResourceSystem::UpdateResourceProvider()
    {
        Threading::ScopeLock lock(m_accessLock);
        m_pResourceProvider->Update();

        //-------------------------------------------------------------------------

#ifdef SE_DEVELOPMENT
        for (auto const &updatedResourceID : m_pResourceProvider->GetExternallyUpdatedResources())
        {
            RequestResourceHotReload(updatedResourceID);
        }
#endif
    }

    void ResourceSystem::WaitForAllRequestsToComplete()
    {
        while (IsBusy())
        {
            OnUpdate();
        }
    }

    void ResourceSystem::ProcessResourceRequests()
    {
		PROFILE_CPU();

        //-------------------------------------------------------------------------

        // We dont have to worry about this loop even if the m_activeRequests array is modified from another thread since we only access the array in 2 places and both use locks
        for (int32_t i = (int32_t)m_activeRequests.Count() - 1; i >= 0; i--)
        {
            ResourceRequest::RequestContext context;
            context.createRawRequestRequest = [this](ResourceRequest *pRequest)
            { m_pResourceProvider->RequestRawResource(pRequest); };
            context.cancelRawRequestRequest = [this](ResourceRequest *pRequest)
            { m_pResourceProvider->CancelRequest(pRequest); };
            context.loadResource = [this](ResourceRequesterID const &requesterID, ResPtr &resourcePtr)
            { LoadResource(resourcePtr, requesterID); };
            context.unloadResource = [this](ResourceRequesterID const &requesterID, ResPtr &resourcePtr)
            { UnloadResource(resourcePtr, requesterID); };

            //-------------------------------------------------------------------------

            bool isRequestComplete = false;

            ResourceRequest *pRequest = m_activeRequests[i];
            if (pRequest->IsActive())
            {
                isRequestComplete = pRequest->Update(context);
            }
            else
            {
                isRequestComplete = true;
            }

            //-------------------------------------------------------------------------

            if (isRequestComplete)
            {
                // We need to process and remove completed requests at the next update stage since unload task may have queued unload requests which refer to the request's allocated memory
                m_completedRequests.Add(pRequest);
                m_activeRequests.RemoveAt(i);
            }
        }
    }

    //-------------------------------------------------------------------------

#ifdef SE_DEVELOPMENT
    void ResourceSystem::RequestResourceHotReload(ResID const &resourceID)
    {
        Threading::ScopeLock lock(m_accessLock);

        // If the resource is not currently in use then just early-out
        auto const recordIter = m_resourceRecords.Find(resourceID);
        if (recordIter == m_resourceRecords.end())
        {
            return;
        }

        // Generate a list of users for this resource
        ResRecord *pRecord = recordIter->Value;
        GetUsersForResource(pRecord, m_usersThatRequireReload);

        // Add to list of resources to be reloaded
		m_externallyUpdatedResources.AddUnique(resourceID);
    }

    void ResourceSystem::ClearHotReloadRequests()
    {
        Threading::ScopeLock lock(m_accessLock);
        m_usersThatRequireReload.Clear();
        m_externallyUpdatedResources.Clear();
    }

	void ResourceSystem::CollectExtensionMap()
	{
		TypeID textureID = Typeof<TextureBase>();
		m_ExtensionToType.Add("png", textureID);
	}

	TypeID ResourceSystem::FileExtensionToTypeID(StringView extension)
	{
		TypeID id = TypeID();

		m_ExtensionToType.TryGet(extension, id);
		return TypeID();
	}

	bool ResourceSystem::OnInit()
	{
		m_pResourceProvider = Engine::pApp->GetResourceProvider();

		Function<void()> predicate = [this] ()
		{
		  return ProcessResourceRequests();
		};
		m_asyncProcessingTask = Threading::Task::StartNew(predicate, this);

		CollectExtensionMap();

		return true;
	}

	void ResourceSystem::OnUpdate()
	{
		PROFILE_CPU();
		ENGINE_ASSERT(Threading::IsMainThread());
		ENGINE_ASSERT(m_pResourceProvider != nullptr);

		m_pResourceProvider->Update();


		// Update resource provider
		//-------------------------------------------------------------------------
		// This will also update the hot-reload data

		UpdateResourceProvider();

		// Wait for async task to complete
		//-------------------------------------------------------------------------

		if (m_isAsyncTaskRunning && m_asyncProcessingTask != nullptr && !m_asyncProcessingTask->IsFinished())
		{
			return;
		}

		m_isAsyncTaskRunning = false;

		// Process and Update requests
		//-------------------------------------------------------------------------

		{
			Threading::ScopeLock lock(m_accessLock);

			for (auto &pendingRequest : m_pendingRequests)
			{
				// Get existing active request
				auto pActiveRequest = TryFindActiveRequest(pendingRequest.m_pRecord);

				// Load request
				if (pendingRequest.m_type == PendingRequest::Type::Load)
				{
					if (pActiveRequest != nullptr)
					{
						if (pActiveRequest->IsUnloadRequest())
						{
							pActiveRequest->SwitchToLoadTask();
						}
					}
					else if (pendingRequest.m_pRecord->IsLoaded()) // Can occur due to multiple requests for the same resource in the same frame
					{
						// Do Nothing
					}
					else // Create new request
					{
						auto loaderIter = m_resourceLoaders.Find(pendingRequest.m_pRecord->GetResourceID().GetTypeID());
						ENGINE_ASSERT(loaderIter != m_resourceLoaders.end());
						m_activeRequests.Add(New<ResourceRequest>(pendingRequest.m_requesterID,
							ResourceRequest::Type::Load,
							pendingRequest.m_pRecord,
							loaderIter->Value));
					}
				}
				else // Unload request
				{
					if (pActiveRequest != nullptr)
					{
						if (pActiveRequest->IsLoadRequest())
						{
							pActiveRequest->SwitchToUnloadTask();
						}
					}
					else if (pendingRequest.m_pRecord->IsUnloaded()) // Can occur due to multiple requests for the same resource in the same frame
					{
						if (!pendingRequest.m_pRecord->HasReferences())
						{
							auto recordIter = m_resourceRecords.Find(pendingRequest.m_pRecord->m_ResID);
							ENGINE_ASSERT(recordIter != m_resourceRecords.end());
							ENGINE_ASSERT(recordIter->Value == pendingRequest.m_pRecord);

							Delete(pendingRequest.m_pRecord);
							m_resourceRecords.Remove(recordIter);
						}
					}
					else // Create new request
					{
						auto loaderIter = m_resourceLoaders.Find(pendingRequest.m_pRecord->GetResourceID().GetTypeID());
						ENGINE_ASSERT(loaderIter != m_resourceLoaders.end());
						m_activeRequests.Add(New<ResourceRequest>(pendingRequest.m_requesterID,
							ResourceRequest::Type::Unload,
							pendingRequest.m_pRecord,
							loaderIter->Value));
					}
				}
			}

			m_pendingRequests.Clear();

			// Process completed requests
			//-------------------------------------------------------------------------

			for (auto pCompletedRequest : m_completedRequests)
			{
				ResID const resourceID = pCompletedRequest->GetResourceID();
				ENGINE_ASSERT(pCompletedRequest->IsComplete());

#ifdef SE_DEVELOPMENT
				m_history.Add(CompletedRequestLog(pCompletedRequest->IsLoadRequest() ? PendingRequest::Type::Load : PendingRequest::Type::Unload, resourceID));
#endif

				if (pCompletedRequest->IsUnloadRequest())
				{
					// Check if we can remove the record, we may have had a load request for it in the meantime
					if (!pCompletedRequest->GetResourceRecord()->HasReferences())
					{
						auto recordIter = m_resourceRecords.Find(resourceID);
						ENGINE_ASSERT(recordIter != m_resourceRecords.end());
						ENGINE_ASSERT(recordIter->Value == pCompletedRequest->GetResourceRecord());

						Delete(recordIter->Value);
						m_resourceRecords.Remove(recordIter);
					}
				}

				// Delete request
				Delete(pCompletedRequest);
			}

			m_completedRequests.Clear();
		}

		// Kick off new async task
		//-------------------------------------------------------------------------

		if (!m_activeRequests.IsEmpty())
		{
			Function<void()> predicate = [this] ()
			{
			  return ProcessResourceRequests();
			};
			m_asyncProcessingTask = Threading::Task::StartNew(predicate, this);
			m_isAsyncTaskRunning = true;
		}
	}

	void ResourceSystem::OnDispose()
	{
		WaitForAllRequestsToComplete();
		m_pResourceProvider = nullptr;
	}
#endif
}