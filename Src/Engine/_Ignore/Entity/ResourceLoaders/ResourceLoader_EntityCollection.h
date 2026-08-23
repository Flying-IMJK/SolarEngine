#pragma once

#include "Engine/_Module/API.h"
#include "Engine/Entity/EntityDescriptors.h"
#include "Base/Resource/ResourceLoader.h"

//-------------------------------------------------------------------------

namespace SE::TypeSystem { class TypeRegistry; }

//-------------------------------------------------------------------------

namespace SE::EntityModel
{
    class EntityCollectionLoader : public Resource::ResourceLoader
    {
    public:

        EntityCollectionLoader();
        ~EntityCollectionLoader() { ENGINE_ASSERT( m_pTypeRegistry == nullptr ); }

        void SetTypeRegistryPtr( TypeRegistry const* pTypeRegistry );
        inline void ClearTypeRegistryPtr() { m_pTypeRegistry = nullptr; }

    private:

        virtual bool LoadInternal( ResID const& resID, Resource::ResRecord* pResourceRecord, Serialization::BinaryInputArchive& archive ) const final;
        virtual bool CanProceedWithFailedInstallDependency() const override { return true; }

    private:

        TypeRegistry const* m_pTypeRegistry;
    };
}