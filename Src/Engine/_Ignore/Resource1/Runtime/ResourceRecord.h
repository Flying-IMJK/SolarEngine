#pragma once

#include "ResourceID.h"
#include "ResourceRequesterID.h"
#include "Core/Types/LoadingStatus.h"
#include "Core/Types/SGUID.h"
#include "Core/Tools/Time.h"
#include <atomic>

//-------------------------------------------------------------------------

namespace SGE
{
	class IResource;
	class ResID;

	/**
	 * 每个请求资源的唯一记录 \n
	 * -资源记录不是线程安全的，因此资源系统需要确保所有外部访问都是线程安全的
	 */
    class SE_API_RUNTIME ResRecord
    {
        friend class ResourceSystem;
        friend class ResourceRequest;
        friend class ResourceLoader;
        friend class ResourceDebugView;

    public:
        ResRecord(ResID resID) : m_ResID(resID) { ENGINE_ASSERT(resID.IsValid()); }
        ~ResRecord();

        inline bool IsValid() const { return m_ResID.IsValid(); }
        inline ResID const &GetResourceID() const { return m_ResID; }

        inline void SetLoadingStatus(LoadingStatus status)
        {
			m_LoadingStatus = status;
            SE_DEVELOPMENT_ONLY(m_compilationLog.Clear());
        }
        inline LoadingStatus GetLoadingStatus() const { return m_LoadingStatus; }

        inline IResource *GetResourceData() { return m_pRes; }
        inline IResource const *GetResourceData() const { return m_pRes; }
        inline void SetResourceData(IResource *pResourceData) { m_pRes = pResourceData; }

        template <typename T>
        inline T *GetResourceData() { return reinterpret_cast<T *>(m_pRes); }

        //-------------------------------------------------------------------------

        inline bool HasReferences() const { return !m_References.IsEmpty(); }

        inline void AddReference(ResourceRequesterID const &requesterID) { m_References.Add(requesterID); }

        inline void RemoveReference(ResourceRequesterID const &requesterID) { ENGINE_ASSERT(m_References.Remove(requesterID)); }

        //-------------------------------------------------------------------------

        inline bool IsLoading() const { return m_LoadingStatus == LoadingStatus::Loading; }
        inline bool IsLoaded() const { return m_LoadingStatus == LoadingStatus::Loaded; }
        inline bool IsUnloading() const { return m_LoadingStatus == LoadingStatus::Unloading; }
        inline bool IsUnloaded() const { return m_LoadingStatus == LoadingStatus::Unloaded; }
        inline bool HasLoadingFailed() const { return m_LoadingStatus == LoadingStatus::Failed; }

        inline List<ResID> const &GetDependencies() const { return m_DependencyResIDs; }

        //-------------------------------------------------------------------------

#ifdef SE_DEVELOPMENT
        inline Milliseconds GetFileReadTime() const { return m_fileReadTime; }
        inline Milliseconds GetLoadTime() const { return m_loadTime; }
        inline Milliseconds GetDependenciesWaitTime() const { return m_waitForDependenciesTime; }
        inline Milliseconds GetInstallTime() const { return m_installTime; }

        inline void SetCompilationLog(String const &log) { m_compilationLog = log; }
        inline void ClearCompilationLog() { m_compilationLog.Clear(); }
        String const &GetCompilationLog() const { return m_compilationLog; }
#endif

    protected:
		// 此记录引用的资源的ID
        ResID m_ResID;
		// 实际加载的资源数据
        IResource *m_pRes = nullptr;
		// 此资源的状态（原子，因为它将被跨多个帧运行的资源请求修改）
        std::atomic<LoadingStatus> m_LoadingStatus = LoadingStatus::Unloaded;
		// 对此资源的引用列表
		List<ResourceRequesterID> m_References;
		// 此资源依赖资源列表
        List<ResID> m_DependencyResIDs;

#ifdef SE_DEVELOPMENT
        uint64 m_sourceResourceHash = 0;
        Milliseconds m_fileReadTime = 0;
        Milliseconds m_loadTime = 0;
        Milliseconds m_waitForDependenciesTime = 0;
        Milliseconds m_installTime = 0;
        String m_compilationLog;
#endif
    };
}