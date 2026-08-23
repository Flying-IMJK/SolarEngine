#include "ResourceLoader.h"
#include "Core/Serialization/MemoryReadStream.h"

//-------------------------------------------------------------------------

namespace SGE
{
    bool ResourceLoader::Load(ResID const &resourceID, List<uint8> &rawData, ResRecord *pResourceRecord) const
    {
        MemoryReadStream readStream;
		readStream.Init(rawData);

        // Read resource header
        ResourceHeader header;
//		readStream.Read(header.m_version);
//		readStream.Read(header.m_resourceType);
//		readStream.Read(header.m_version);
//		readStream.Read(header.m_version);

#ifdef SE_DEVELOPMENT
        pResourceRecord->m_sourceResourceHash = header.m_sourceResourceHash;
#endif

        // Set all install dependencies
        pResourceRecord->m_DependencyResIDs.Resize(header.m_installDependencies.Count());
        for (auto const &depResourceID : header.m_installDependencies)
        {
            pResourceRecord->m_DependencyResIDs.Add(depResourceID);
        }

        // Perform resource load
        if (!LoadInternal(resourceID, pResourceRecord, readStream))
        {
            LOG_ERROR("Resource", "Resource Loader Failed to load resource: {0}", resourceID.GetTypeID().ToString());
            return false;
        }

        // loader 必须始终设置有效的资源数据指针，即使资源内部无效
        // 这是为了防止在 loader 分配资源，尝试加载资源失败后忘记释放分配的数据，造成内存泄漏。
        ENGINE_ASSERT(pResourceRecord->GetResourceData() != nullptr);
        return true;
    }

    ResourceInstallResult ResourceLoader::Install(ResID const &resourceID, ResRecord *pResourceRecord, List<ResPtr> const &installDependencies) const
    {
        ENGINE_ASSERT(pResourceRecord != nullptr);
        pResourceRecord->m_pRes->m_resourceID = resourceID;

#ifdef SE_DEVELOPMENT
        pResourceRecord->m_pRes->m_sourceResourceHash = pResourceRecord->m_sourceResourceHash;
#endif

        return ResourceInstallResult::Succeeded;
    }

    ResourceInstallResult ResourceLoader::UpdateInstall(ResID const &resourceID, ResRecord *pResourceRecord) const
    {
        // This function should never be called directly!!
        // If your resource requires multi-frame installation, you need to override this function in your loader and return InstallResult::InProgress from the install function!
        ENGINE_UNREACHABLE_CODE();
        return ResourceInstallResult::Succeeded;
    }

    void ResourceLoader::Unload(ResID const &resourceID, ResRecord *pResourceRecord) const
    {
        ENGINE_ASSERT(pResourceRecord != nullptr);
        ENGINE_ASSERT(pResourceRecord->IsUnloading() || pResourceRecord->HasLoadingFailed());
        UnloadInternal(resourceID, pResourceRecord);
        pResourceRecord->m_DependencyResIDs.Clear();
    }

    void ResourceLoader::UnloadInternal(ResID const &resourceID, ResRecord *pResourceRecord) const
    {
        IResource *pData = pResourceRecord->GetResourceData();
        Delete(pData);
        pResourceRecord->SetResourceData(nullptr);
    }
}