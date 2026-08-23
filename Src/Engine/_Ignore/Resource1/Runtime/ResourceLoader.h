#pragma once

#include "ResourcePtr.h"
#include "IResource.h"

//-------------------------------------------------------------------------

namespace SGE
{
	class MemoryReadStream;

    //-------------------------------------------------------------------------

    enum class ResourceInstallResult
    {
        Unknown,
        InProgress,
        Succeeded,
        Failed,
    };

	// Describes the contents of a resource, every resource has a header
	struct SE_API_RUNTIME ResourceHeader
	{
	public:
		ResourceHeader() = default;

		ResourceHeader(int32 version, TypeID type, uint64_t sourceResourceHash)
			: m_version(version), m_resourceType(type), m_sourceResourceHash(sourceResourceHash)
		{
		}

		TypeID GetResourceTypeID() const { return m_resourceType; }
		void AddInstallDependency(ResID resourceID) { m_installDependencies.Add(resourceID); }

	public:
		int32 m_version = -1;
		TypeID m_resourceType;
		List<ResID> m_installDependencies;
		uint64 m_sourceResourceHash = 0;
	};
	
    //-------------------------------------------------------------------------

    class SE_API_RUNTIME ResourceLoader
    {

    protected:
        inline static ResPtr GetInstallDependency(List<ResPtr> const &installDependencies, ResID const &resourceID)
        {
            for (auto const &pInstallDependency : installDependencies)
            {
                if (pInstallDependency.GetResourceID() == resourceID)
                {
                    return pInstallDependency;
                }
            }

            ENGINE_HALT();
            return nullptr;
        }

    public:
        virtual ~ResourceLoader() = default;

		List<TypeID> const &GetLoadableTypes() const { return m_loadableTypes; }

        // Can this loader proceed when an install dependency fails to load? Certain resource should still be loaded if some of their dependencies fail (i.e. still load a mesh if a material fails to load)
        virtual bool CanProceedWithFailedInstallDependency() const { return false; }

        // This function loads is responsible to deserialize the compiled resource data, read the resource header for install dependencies and to create the new runtime resource object
        bool Load(ResID const &resourceID, List<uint8> &rawData, ResRecord *pResourceRecord) const;

        // This function will destroy the created resource object
        void Unload(ResID const &resourceID, ResRecord *pResourceRecord) const;

        // This function is called once all the install dependencies have been loaded, it allows us to update any internal resource ptrs the resource might hold
		// 所有依赖项加载完成，会调用此函数，允许更新资源持有的内部资源
        virtual ResourceInstallResult Install(ResID const &resourceID, ResRecord *pResourceRecord, List<ResPtr> const &installDependencies) const;

        // 当要卸载资源时，调用此函数，清理资源的依赖内容
        virtual void Uninstall(ResID const &resourceID, ResRecord *pResourceRecord) const {}

        // 检查依赖资源的状态
        virtual ResourceInstallResult UpdateInstall(ResID const &resourceID, ResRecord *pResourceRecord) const;

    protected:
        // (Required) Override this function to implement you custom deserialization and creation logic, resource header has already been read at this point
        virtual bool LoadInternal(ResID const &resourceID, ResRecord *pResourceRecord, MemoryReadStream &stream) const = 0;

        // (Optional) Override this function to implement any custom object destruction logic if needed, by default this will just delete the created resource
        virtual void UnloadInternal(ResID const &resourceID, ResRecord *pResourceRecord) const;

    protected:
		List<TypeID> m_loadableTypes;
    };
    
}