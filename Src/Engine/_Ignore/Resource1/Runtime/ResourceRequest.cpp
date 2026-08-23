#include "ResourceRequest.h"
#include "ResourceRequesterID.h"
#include "ResourceLoader.h"
#include "Core/Platform/File.h"
#include "Core/Profiler/Profiler.h"
#include "Core/Thread/Threading.h"

//-------------------------------------------------------------------------

namespace SGE
{
	ResourceRequesterID::ResourceRequesterID(const ResID& resourceID)
		: m_ID(resourceID), m_isInstallDependency(true)
	{
	}


    ResourceRequest::ResourceRequest(ResourceRequesterID const &requesterID, Type type, ResRecord *pRecord, ResourceLoader *pResourceLoader)
        : m_requesterID(requesterID), m_pResourceRecord(pRecord), m_pResourceLoader(pResourceLoader), m_type(type)
    {
        ENGINE_ASSERT(Threading::IsMainThread());
        ENGINE_ASSERT(m_pResourceRecord != nullptr && m_pResourceRecord->IsValid());
        ENGINE_ASSERT(m_pResourceLoader != nullptr);
        ENGINE_ASSERT(m_type != Type::Invalid);
        ENGINE_ASSERT(!m_pResourceRecord->IsLoading() && !m_pResourceRecord->IsUnloading());

        if (m_type == Type::Load)
        {
            ENGINE_ASSERT(m_pResourceRecord->IsUnloaded() || m_pResourceRecord->HasLoadingFailed());
            m_stage = Stage::RequestRawResource;
            m_pResourceRecord->SetLoadingStatus(LoadingStatus::Loading);
        }
        else // Unload
        {
            if (m_pResourceRecord->HasLoadingFailed())
            {
                m_stage = Stage::UnloadFailedResource;
            }
            else if (m_pResourceRecord->IsLoaded())
            {
                m_stage = Stage::UninstallResource;
                m_pResourceRecord->SetLoadingStatus(LoadingStatus::Unloading);
            }
            else // Why are you unloading an already unloaded resource?!
            {
                ENGINE_UNREACHABLE_CODE();
            }
        }
    }

    void ResourceRequest::OnRawResourceRequestComplete(String const &filePath, String const &log)
    {
        // Raw resource failed to load
        if (filePath.IsEmpty())
        {
            LOG_ERROR("Resource", "Resource Request Failed to find/compile resource file ({0}) - %s", filePath, log.Get());
            m_stage = ResourceRequest::Stage::Complete;
            m_pResourceRecord->SetLoadingStatus(LoadingStatus::Failed);

#ifdef SE_DEVELOPMENT
            m_pResourceRecord->SetCompilationLog(log);
#endif
        }
        else // Continue the load operation
        {
            m_rawResourcePath = filePath;
            m_stage = ResourceRequest::Stage::LoadResource;
        }
    }

    void ResourceRequest::SwitchToLoadTask()
    {
        ENGINE_ASSERT(m_type == Type::Unload);
        ENGINE_ASSERT(!m_isReloadRequest);

        m_type = Type::Load;
        m_pResourceRecord->SetLoadingStatus(LoadingStatus::Loading);

        //-------------------------------------------------------------------------

        switch (m_stage)
        {
        case Stage::Complete:
        {
            m_stage = Stage::RequestRawResource;
        }
        break;

        case Stage::UninstallResource:
        {
            m_stage = Stage::Complete;
            m_pResourceRecord->SetLoadingStatus(LoadingStatus::Loaded);
        }
        break;

        case Stage::CancelWaitForLoadDependencies:
        {
            m_stage = Stage::WaitForLoadDependencies;
        }
        break;

        case Stage::UnloadResource:
        {
            m_stage = Stage::InstallResource;
        }
        break;

        default:
            ENGINE_HALT();
            break;
        }
    }

    void ResourceRequest::SwitchToUnloadTask()
    {
        ENGINE_ASSERT(m_type == Type::Load);
        ENGINE_ASSERT(!m_pResourceRecord->HasReferences());

        m_type = Type::Unload;
        m_pResourceRecord->SetLoadingStatus(LoadingStatus::Unloading);

        //-------------------------------------------------------------------------

        switch (m_stage)
        {
        case Stage::WaitForRawResourceRequest:
        {
            m_stage = Stage::CancelRawResourceRequest;
        }
        break;

        case Stage::LoadResource:
        {
            m_stage = Stage::Complete;
            m_pResourceRecord->SetLoadingStatus(LoadingStatus::Unloaded);
        }
        break;

        case Stage::WaitForLoadDependencies:
        {
            m_stage = Stage::CancelWaitForLoadDependencies;
        }
        break;

        case Stage::InstallResource:
        case Stage::WaitForInstallResource:
        {
            m_stage = Stage::UnloadResource;
        }
        break;

        case Stage::Complete:
        {
            m_stage = Stage::UninstallResource;
        }
        break;

        default:
            ENGINE_HALT();
            break;
        }
    }

    //-------------------------------------------------------------------------

    bool ResourceRequest::Update(RequestContext &requestContext)
    {
        // Update loading
        //-------------------------------------------------------------------------

        switch (m_stage)
        {
        case ResourceRequest::Stage::RequestRawResource:
        {
            RequestRawResource(requestContext);
        }
        break;

        case ResourceRequest::Stage::WaitForRawResourceRequest:
        {
            // Do Nothing
			PROFILE_CPU_NAMED("Wait For Raw Resource Request");
        }
        break;

        case ResourceRequest::Stage::LoadResource:
        {
            LoadResource(requestContext);
        }
        break;

        case ResourceRequest::Stage::WaitForLoadDependencies:
        {
            WaitForLoadDependencies(requestContext);
        }
        break;

        case ResourceRequest::Stage::InstallResource:
        {
            InstallResource(requestContext);
        }
        break;

        case ResourceRequest::Stage::WaitForInstallResource:
        {
            WaitForInstallResource(requestContext);
        }
        break;

            //-------------------------------------------------------------------------

        case ResourceRequest::Stage::UninstallResource:
        {
            // Execute all unload operations immediately
            UninstallResource(requestContext);
            UnloadResource(requestContext);
        }
        break;

        case ResourceRequest::Stage::UnloadResource:
        {
            UnloadResource(requestContext);
        }
        break;

        case ResourceRequest::Stage::UnloadFailedResource:
        {
            UnloadFailedResource(requestContext);
        }
        break;

        case ResourceRequest::Stage::CancelWaitForLoadDependencies:
        {
            // Execute all unload operations immediately
            m_pendingInstallDependencies.Clear();
            m_installDependencies.Clear();
            m_stage = Stage::UnloadResource;
            UnloadResource(requestContext);
        }
        break;

        case ResourceRequest::Stage::CancelRawResourceRequest:
        {
            CancelRawRequestRequest(requestContext);
        }
        break;

        default:
        {
            ENGINE_UNREACHABLE_CODE();
        }
        break;
        }

        //-------------------------------------------------------------------------

#ifdef SE_DEVELOPMENT
        if (IsComplete())
        {
            ENGINE_ASSERT(m_pResourceRecord->IsLoaded() || m_pResourceRecord->IsUnloaded() || m_pResourceRecord->HasLoadingFailed());
        }
#endif

        return IsComplete();
    }

    //-------------------------------------------------------------------------

    void ResourceRequest::RequestRawResource(RequestContext &requestContext)
    {
		PROFILE_CPU();
        m_stage = ResourceRequest::Stage::WaitForRawResourceRequest;
        requestContext.createRawRequestRequest(this);
    }

    void ResourceRequest::LoadResource(RequestContext &requestContext)
    {
		PROFILE_CPU();
        ENGINE_ASSERT(m_stage == ResourceRequest::Stage::LoadResource);
        ENGINE_ASSERT(!m_rawResourcePath.IsEmpty());

        // Read file
        //-------------------------------------------------------------------------

        {
			PROFILE_CPU_NAMED("Read File");
//            PROFILE_TAG("filename", m_rawResourcePath.GetFilename().c_str());

#ifdef SE_DEVELOPMENT
//            ScopedTimer<PlatformClock> timer(m_pResourceRecord->m_fileReadTime);
#endif

            if (!File::ReadAllBytes(m_rawResourcePath, m_rawResourceData))
            {
                LOG_ERROR("Resource", "Resource Request Failed to load resource file ({0})", m_rawResourcePath);
                m_stage = ResourceRequest::Stage::Complete;
                m_pResourceRecord->SetLoadingStatus(LoadingStatus::Failed);
                return;
            }
        }

        // Load resource
        //-------------------------------------------------------------------------

        {
			PROFILE_CPU_NAMED("Load Resource");

#ifdef SE_DEVELOPMENT
            char resTypeID[5];
//            m_pResourceRecord->GetResourceTypeID().GetString(resTypeID);
//            PROFILE_TAG("Loader", resTypeID);
#endif

            // Load the resource
            ENGINE_ASSERT(!m_rawResourceData.IsEmpty());

#ifdef SE_DEVELOPMENT
//            ScopedTimer<PlatformClock> timer(m_pResourceRecord->m_loadTime);
#endif

            if (!m_pResourceLoader->Load(GetResourceID(), m_rawResourceData, m_pResourceRecord))
            {
                LOG_ERROR("Resource", "Resource Request Failed to load compiled resource data ({0})", m_rawResourcePath);
                m_pResourceRecord->SetLoadingStatus(LoadingStatus::Failed);
                m_pResourceLoader->Unload(GetResourceID(), m_pResourceRecord);
                m_stage = ResourceRequest::Stage::Complete;
                return;
            }

            // Release raw data
            m_rawResourceData.Clear();
        }

        // Load dependencies
        //-------------------------------------------------------------------------

        // Create the resource ptrs for the install dependencies and request their load
        // These resource ptrs are temporary and will be clear upon completion of the request
        ResourceRequesterID const installDependencyRequesterID(m_pResourceRecord->GetResourceID());
        uint32 const numInstallDependencies = (uint32)m_pResourceRecord->m_DependencyResIDs.Count();
        m_pendingInstallDependencies.Resize(numInstallDependencies);
        for (uint32 i = 0; i < numInstallDependencies; i++)
        {
            // Do not use the requester ID for install dependencies! Since they are not explicitly loaded by a specific user!
            // Instead we create a ResourceRequesterID from the depending resource's resourceID
            m_pendingInstallDependencies[i] = ResPtr(m_pResourceRecord->m_DependencyResIDs[i]);
            requestContext.loadResource(installDependencyRequesterID, m_pendingInstallDependencies[i]);
        }

        m_stage = ResourceRequest::Stage::WaitForLoadDependencies;

#ifdef SE_DEVELOPMENT
        m_pResourceRecord->m_waitForDependenciesTime = 0.0f;
//        m_stageTimer.Start();
#endif
    }

    void ResourceRequest::WaitForLoadDependencies(RequestContext &requestContext)
    {
		PROFILE_CPU();
        ENGINE_ASSERT(m_stage == ResourceRequest::Stage::WaitForLoadDependencies);

        enum class InstallStatus
        {
            Loading,
            ShouldProceed,
            ShouldFail,
        };

        // Check if all dependencies are finished installing
        auto status = InstallStatus::ShouldProceed;

        for (size_t i = 0; i < m_pendingInstallDependencies.Count(); i++)
        {
            if (m_pendingInstallDependencies[i].HasLoadingFailed())
            {
                if (!m_pResourceLoader->CanProceedWithFailedInstallDependency())
                {
                    LOG_ERROR("Resource", "Resource Request Failed to load install dependency: {0}");
                    status = InstallStatus::ShouldFail;
                    break;
                }
            }

            // If it's loaded, move it to the loaded list and continue iterating
            if (m_pendingInstallDependencies[i].IsLoaded() || m_pendingInstallDependencies[i].HasLoadingFailed())
            {
                m_installDependencies.Add(m_pendingInstallDependencies[i]);
                m_pendingInstallDependencies.RemoveAt(i);
                i--;
            }
            else
            {
                status = InstallStatus::Loading;
                break;
            }
        }

        // If dependency has failed, the resource has failed to load so immediately unload and set status to failed
        if (status == InstallStatus::ShouldFail)
        {
            LOG_ERROR("Resource", "Resource Request Failed to load resource file due to failed dependency ({0})");

            // Do not use the user ID for install dependencies! Since they are not explicitly loaded by a specific user!
            // Instead we create a ResourceRequesterID from the depending resource's resourceID
            ResourceRequesterID const installDependencyRequesterID(m_pResourceRecord->GetResourceID());

            // Unload all install dependencies
            for (auto &pendingDependency : m_pendingInstallDependencies)
            {
                requestContext.unloadResource(installDependencyRequesterID, pendingDependency);
            }

            for (auto &dependency : m_installDependencies)
            {
                requestContext.unloadResource(installDependencyRequesterID, dependency);
            }

            m_pendingInstallDependencies.Clear();
            m_installDependencies.Clear();
            m_pResourceRecord->SetLoadingStatus(LoadingStatus::Failed);
            m_pResourceLoader->Unload(GetResourceID(), m_pResourceRecord);
            m_stage = ResourceRequest::Stage::Complete;
        }
        // Install runtime resource
        else if (status == InstallStatus::ShouldProceed)
        {
            ENGINE_ASSERT(m_pendingInstallDependencies.IsEmpty());
            m_stage = ResourceRequest::Stage::InstallResource;

#ifdef SE_DEVELOPMENT
//            m_pResourceRecord->m_waitForDependenciesTime = m_stageTimer.GetElapsedTimeMilliseconds();
//            m_stageTimer.Start();
#endif
        }
    }

    void ResourceRequest::InstallResource(RequestContext &requestContext)
    {
		PROFILE_CPU();
        ENGINE_ASSERT(m_stage == ResourceRequest::Stage::InstallResource);
        ENGINE_ASSERT(m_pendingInstallDependencies.IsEmpty());

        {
#ifdef SE_DEVELOPMENT
//            ScopedTimer<PlatformClock> timer(m_pResourceRecord->m_installTime);
#endif

            ResourceInstallResult const result = m_pResourceLoader->Install(GetResourceID(), m_pResourceRecord, m_installDependencies);
            switch (result)
            {
            // Finished installing the resource
            case ResourceInstallResult::Succeeded:
            {
                ENGINE_ASSERT(m_pResourceRecord->GetResourceData() != nullptr);
                m_installDependencies.Clear();
                m_pResourceRecord->SetLoadingStatus(LoadingStatus::Loaded);
                m_stage = ResourceRequest::Stage::Complete;

#ifdef SE_DEVELOPMENT
//                m_pResourceRecord->m_installTime = m_stageTimer.GetElapsedTimeMilliseconds();
#endif
            }
            break;

            // Wait for install to complete
            case ResourceInstallResult::InProgress:
            {
                ENGINE_ASSERT(m_pResourceRecord->GetResourceData() != nullptr);
                m_installDependencies.Clear();
                m_stage = ResourceRequest::Stage::WaitForInstallResource;
            }
            break;

            // Install operation failed, unload resource and set status to failed
            case ResourceInstallResult::Failed:
            {
                LOG_ERROR("Resource", "Resource Request Failed to install resource ({0})");

                m_stage = ResourceRequest::Stage::UnloadResource;
                UnloadResource(requestContext);
                m_pResourceRecord->SetLoadingStatus(LoadingStatus::Failed);
                m_stage = ResourceRequest::Stage::Complete;
            }
            break;

            default:
            {
                ENGINE_UNREACHABLE_CODE();
            }
            break;
            }
        }
    }

    void ResourceRequest::WaitForInstallResource(RequestContext &requestContext)
    {
		PROFILE_CPU();
        ENGINE_ASSERT(m_stage == ResourceRequest::Stage::WaitForInstallResource);
        ENGINE_ASSERT(m_pendingInstallDependencies.IsEmpty());
        ENGINE_ASSERT(m_pResourceRecord->GetResourceData() != nullptr);

        ResourceInstallResult const result = m_pResourceLoader->UpdateInstall(GetResourceID(), m_pResourceRecord);
        switch (result)
        {
        // Finished installing the resource
        case ResourceInstallResult::Succeeded:
        {
            m_pResourceRecord->SetLoadingStatus(LoadingStatus::Loaded);
            m_stage = ResourceRequest::Stage::Complete;
        }
        break;

        // Wait for install to complete
        case ResourceInstallResult::InProgress:
        {
            // Do Nothing
        }
        break;

        // Install operation failed, unload resource and set status to failed
        case ResourceInstallResult::Failed:
        {
            LOG_ERROR("Resource", "Resource Request Failed to install resource ({0})");

            m_stage = ResourceRequest::Stage::UnloadResource;
            UnloadResource(requestContext);
            m_pResourceRecord->SetLoadingStatus(LoadingStatus::Failed);
            m_stage = ResourceRequest::Stage::Complete;
        }
        break;

        default:
        {
            ENGINE_UNREACHABLE_CODE();
        }
        break;
        }
    }

    //-------------------------------------------------------------------------

    void ResourceRequest::UninstallResource(RequestContext &requestContext)
    {
        ENGINE_ASSERT(m_stage == ResourceRequest::Stage::UninstallResource);
        m_pResourceLoader->Uninstall(GetResourceID(), m_pResourceRecord);
        m_stage = ResourceRequest::Stage::UnloadResource;
    }

    void ResourceRequest::UnloadResource(RequestContext &requestContext)
    {
        ENGINE_ASSERT(m_stage == ResourceRequest::Stage::UnloadResource);

        // Unload dependencies
        //-------------------------------------------------------------------------

        // Create the resource ptrs for the install dependencies and request the unload
        // These resource ptrs are temporary and will be cleared upon completion of the request
        ResourceRequesterID const installDependencyRequesterID(m_pResourceRecord->GetResourceID());
        uint32 const numInstallDependencies = (uint32)m_pResourceRecord->m_DependencyResIDs.Count();
        m_pendingInstallDependencies.Resize(numInstallDependencies);
        for (uint32 i = 0; i < numInstallDependencies; i++)
        {
            // Do not use the user ID for install dependencies! Since they are not explicitly loaded by a specific user!
            // Instead we create a ResourceRequesterID from the depending resource's resourceID
            m_pendingInstallDependencies[i] = ResPtr(m_pResourceRecord->m_DependencyResIDs[i]);
            requestContext.unloadResource(installDependencyRequesterID, m_pendingInstallDependencies[i]);
        }

        // Unload resource
        //-------------------------------------------------------------------------

        ENGINE_ASSERT(m_pResourceRecord->IsUnloading());
        m_pResourceLoader->Unload(GetResourceID(), m_pResourceRecord);
        m_pResourceRecord->SetLoadingStatus(LoadingStatus::Unloaded);

        m_stage = ResourceRequest::Stage::Complete;

        if (m_isReloadRequest)
        {
            // Clear the flag here, in case we re-used this request again
            m_isReloadRequest = false;
            SwitchToLoadTask();
        }
    }

    void ResourceRequest::UnloadFailedResource(RequestContext &requestContext)
    {
        ENGINE_ASSERT(m_stage == ResourceRequest::Stage::UnloadFailedResource);
        ENGINE_ASSERT(m_pResourceRecord->HasLoadingFailed());

        m_pResourceLoader->Unload(GetResourceID(), m_pResourceRecord);
        m_pResourceRecord->SetLoadingStatus(LoadingStatus::Unloaded);
        m_stage = ResourceRequest::Stage::Complete;

        if (m_isReloadRequest)
        {
            // Clear the flag here, in case we re-used this request again
            m_isReloadRequest = false;
            SwitchToLoadTask();
        }
    }

    //-------------------------------------------------------------------------

    void ResourceRequest::CancelRawRequestRequest(RequestContext &requestContext)
    {
        ENGINE_ASSERT(m_stage == ResourceRequest::Stage::CancelRawResourceRequest);
        requestContext.cancelRawRequestRequest(this);
        m_pResourceRecord->SetLoadingStatus(LoadingStatus::Unloaded);
        m_stage = ResourceRequest::Stage::Complete;
    }

	ResID const& ResourceRequest::GetResourceID() const
	{
		return m_pResourceRecord->GetResourceID();
	}

	LoadingStatus ResourceRequest::GetLoadingStatus() const
	{
		return m_pResourceRecord->GetLoadingStatus();
	}

}