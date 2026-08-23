#pragma once

#include "Core/Types/Delegate.h"
#include "Core/Types/TimeSpan.h"
#include "ResourcePtr.h"
#include "ResourceRequesterID.h"

namespace SGE
{
	class ResourceLoader;
	class ResRecord;
	class ResPtr;
	class ResID;

    class SE_API_RUNTIME ResourceRequest
    {
    public:
        enum class Stage
        {
            None = -1,

            // Load Stages
            RequestRawResource,
            WaitForRawResourceRequest,
            LoadResource,
            WaitForLoadDependencies,
            InstallResource,
            WaitForInstallResource,

            // Unload Stages
            UninstallResource,
            UnloadResource,
            UnloadFailedResource,

            // Special Cases
            CancelWaitForLoadDependencies, // This stage is needed so we can resume correctly when going from load -> unload -> load
            CancelRawResourceRequest,

            Complete,
        };

        enum class Type
        {
            Invalid = -1,
            Load,
            Unload,
        };

        enum class SwitchRequestType
        {
            None = 0,
            SwitchToLoad,
            SwitchToUnload,
            SwitchToReload,
        };

        struct RequestContext
        {
            Function<void(ResourceRequest *)> createRawRequestRequest;
			Function<void(ResourceRequest *)> cancelRawRequestRequest;
			Function<void(ResourceRequesterID const &, ResPtr &)> loadResource;
			Function<void(ResourceRequesterID const &, ResPtr &)> unloadResource;
        };

    public:
        ResourceRequest() = default;
        ResourceRequest(ResourceRequesterID const &requesterID, Type type, ResRecord *pRecord, ResourceLoader *pResourceLoader);

        inline bool IsValid() const { return m_pResourceRecord != nullptr; }
        inline bool IsActive() const { return m_stage != Stage::Complete; }
        inline bool IsComplete() const { return m_stage == Stage::Complete; }
        inline bool IsLoadRequest() const { return m_type == Type::Load; }
        inline bool IsUnloadRequest() const { return m_type == Type::Unload; }

        inline Stage GetStage() const { return m_stage; }

        inline ResRecord const *GetResourceRecord() const { return m_pResourceRecord; }
		ResID const &GetResourceID() const;
		LoadingStatus GetLoadingStatus() const;

        inline bool operator==(ResourceRequest const &other) const { return GetResourceID() == other.GetResourceID(); }
        inline bool operator!=(ResourceRequest const &other) const { return GetResourceID() != other.GetResourceID(); }

        //-------------------------------------------------------------------------

        // 由资源系统调用以更新请求进度
        bool Update(RequestContext &requestContext);

        // Called by the resource provider once the request operation completes and provides the raw resource data
        void OnRawResourceRequestComplete(String const &filePath, String const &log);

        // 中断加载并将其转换为卸载
        void SwitchToLoadTask();

        // 中断卸载并将其转换为加载
        void SwitchToUnloadTask();

        //-------------------------------------------------------------------------

        void RequestRawResource(RequestContext &requestContext);
        void LoadResource(RequestContext &requestContext);
        void WaitForLoadDependencies(RequestContext &requestContext);
        void InstallResource(RequestContext &requestContext);
        void WaitForInstallResource(RequestContext &requestContext);
        void UninstallResource(RequestContext &requestContext);
        void UnloadResource(RequestContext &requestContext);
        void UnloadFailedResource(RequestContext &requestContext);
        void CancelRawRequestRequest(RequestContext &requestContext);

    private:
        ResourceRequesterID m_requesterID;
        ResRecord *m_pResourceRecord = nullptr;
        ResourceLoader *m_pResourceLoader = nullptr;
        String m_rawResourcePath;
		List<uint8> m_rawResourceData;
		List<ResPtr> m_pendingInstallDependencies;
		List<ResPtr> m_installDependencies;
        Type m_type = Type::Invalid;
        Stage m_stage = Stage::None;
        bool m_isReloadRequest = false;

#ifdef SE_DEVELOPMENT
        TimeSpan m_stageTimer;
#endif
    };
}