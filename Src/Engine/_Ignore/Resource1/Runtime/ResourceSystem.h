#pragma once


#include "Runtime/Settings/GlobalSettings_Resource.h"

#include "Core/Thread/Threading.h"
#include "Core/Thread/ThreadPool.h"
#include "Core/Systems.h"
#include "Core/Types/Event.h"
#include "Core/Types/TimeSpan.h"
#include "Core/Types/Collections/Dictionary.h"

namespace SGE
{
    //-------------------------------------------------------------------------

    class SE_API_RUNTIME ResourceSystem : public ISystem
    {
        friend class ResourceDebugView;

        struct PendingRequest
        {
            enum class Type
            {
                Load,
                Unload
            };

        public:
            PendingRequest() = default;

            PendingRequest(Type type, ResRecord *pRecord, ResourceRequesterID const &requesterID)
                : m_pRecord(pRecord), m_requesterID(requesterID), m_type(type)
            {
                ENGINE_ASSERT(m_pRecord != nullptr);
            }

            ResRecord *m_pRecord = nullptr;
            ResourceRequesterID m_requesterID;
            Type m_type = Type::Load;
        };

        #ifdef SE_DEVELOPMENT
        struct CompletedRequestLog
        {
			CompletedRequestLog() = default;
            CompletedRequestLog(PendingRequest::Type type, ResID ID) : m_type(type), m_ID(ID) {}

            PendingRequest::Type m_type;
            ResID m_ID;
            TimeSpan m_time;
        };
        #endif

        ENGINE_SYSTEM(ResourceSystem);

    public:
        ResourceSystem();
        ~ResourceSystem();

		bool OnInit() override;
		void OnUpdate() override;
		void OnDispose() override;

		inline bool IsInitialized() const { return m_pResourceProvider != nullptr; }
        ResourceGlobalSettings const &GetSettings() const;

        // Do we still have work we need to perform
        bool IsBusy() const;

        // Blocking wait for all requests to be completed
        void WaitForAllRequestsToComplete();

        // Resource Loaders
        //-------------------------------------------------------------------------

        void RegisterResourceLoader(ResourceLoader *pLoader);
        void UnregisterResourceLoader(ResourceLoader *pLoader);

        // Loading/Unloading
        //-------------------------------------------------------------------------

        // Request a load of a resource, can optionally provide a ResourceRequesterID for identification of the request source
        void LoadResource(ResPtr &resourcePtr, ResourceRequesterID const &requesterID = ResourceRequesterID());

        // Request an unload of a resource, can optionally provide a ResourceRequesterID for identification of the request source
        void UnloadResource(ResPtr &resourcePtr, ResourceRequesterID const &requesterID = ResourceRequesterID());

        template <typename T>
        inline void LoadResource(TResPtr<T> &resourcePtr, ResourceRequesterID const &requesterID = ResourceRequesterID()) { LoadResource((ResPtr &)resourcePtr, requesterID); }

        template <typename T>
        inline void UnloadResource(TResPtr<T> &resourcePtr, ResourceRequesterID const &requesterID = ResourceRequesterID()) { UnloadResource((ResPtr &)resourcePtr, requesterID); }

        // Hot Reload
        //-------------------------------------------------------------------------

#ifdef SE_DEVELOPMENT
        void RequestResourceHotReload(ResID const &resourceID);
        inline bool RequiresHotReloading() const { return !m_usersThatRequireReload.IsEmpty(); }
        inline List<ResourceRequesterID> const &GetUsersToBeReloaded() const { return m_usersThatRequireReload; }
        inline List<ResID> const &GetResourcesToBeReloaded() const { return m_externallyUpdatedResources; }
        void ClearHotReloadRequests();
#endif
		//-------------------------------------------------------------------------
		TypeID FileExtensionToTypeID(StringView extension);

    private:
        void UpdateResourceProvider();

        ResRecord *FindOrCreateResourceRecord(ResID const &resourceID);
        ResRecord *FindExistingResourceRecord(ResID const &resourceID);

        void AddPendingRequest(PendingRequest &&request);
        ResourceRequest *TryFindActiveRequest(ResRecord const *pResourceRecord) const;

        // Returns a list of all unique external references for the given resource
        void GetUsersForResource(ResRecord const *pResourceRecord, List<ResourceRequesterID> &requesterIDs) const;

        // Process all queued resource requests
        void ProcessResourceRequests();

		void CollectExtensionMap();

    private:
        ResourceProvider *m_pResourceProvider = nullptr;
		Dictionary<TypeID, ResourceLoader *> m_resourceLoaders;
        Dictionary<ResID, ResRecord *> m_resourceRecords;
        mutable CriticalSection m_accessLock;

        // Requests
		List<PendingRequest> m_pendingRequests;
		List<ResourceRequest *> m_activeRequests;
		List<ResourceRequest *> m_completedRequests;

        // ASync
        Threading::Task* m_asyncProcessingTask;
        std::atomic<bool> m_isAsyncTaskRunning = false;

#ifdef SE_DEVELOPMENT
		List<ResourceRequesterID> m_usersThatRequireReload;
		List<ResID> m_externallyUpdatedResources;
		List<CompletedRequestLog> m_history;
#endif

		Dictionary<String, TypeID> m_ExtensionToType;
    };
}