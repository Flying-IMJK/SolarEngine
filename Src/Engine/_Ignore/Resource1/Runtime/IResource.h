#pragma once

#include "ResourceID.h"
#include "Core/TypeSystem/IReflectedType.h"

//-------------------------------------------------------------------------
// Base for all SGE resources
//-------------------------------------------------------------------------
// Note: Virtual resources are resources which dont have explicit resource descriptors or are generated as a side-effect of another resource's compilation
// e.g., We can generate the navmesh for a map as part of compiling the map
// e.g., We generate an anim graph dataset whenever we compile a graph variation

namespace SGE
{
	class SE_API_RUNTIME IResource : public IReflectedType
    {
        friend class ResourceLoader;
		SE_CLASS(IResource, IReflectedType)
    public:

        IResource( IResource const& ) = default;
        virtual ~IResource() = default;

        IResource& operator=( IResource const& ) = default;

        inline ResID const& GetResourceID() const { return m_resourceID; }

        virtual bool IsValid() const = 0;

        #ifdef SE_DEVELOPMENT
        uint64 GetSourceResourceHash() const { return m_sourceResourceHash; }
        #endif

    protected:
        IResource() {}

    private:

        ResID      m_resourceID;
        #ifdef SE_DEVELOPMENT
        uint64     m_sourceResourceHash = 0;
        #endif
    };
}