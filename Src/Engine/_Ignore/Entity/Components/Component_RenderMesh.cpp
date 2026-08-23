#include "Component_RenderMesh.h"

//-------------------------------------------------------------------------

namespace SGE
{
    void MeshComponent::Initialize()
    {
        SpatialEntityComponent::Initialize();

        if ( HasMeshResourceSet() )
        {
            UpdateMaterialCache();
        }
    }

    void MeshComponent::Shutdown()
    {
        m_materials.Clear();
        SpatialEntityComponent::Shutdown();
    }

    void MeshComponent::SetMaterialOverride( int32 materialIdx, ResID materialResourceID )
    {
        ENGINE_ASSERT( IsUnloaded() );
        ENGINE_ASSERT( materialResourceID.IsValid() );

        if ( materialIdx >= m_materialOverrides.Count() )
        {
            m_materialOverrides.Resize( materialIdx + 1 );
            m_materialOverrides[materialIdx] = TResPtr<Render::Material>(materialResourceID);
        }
    }

    void MeshComponent::UpdateMaterialCache()
    {
        ENGINE_ASSERT( HasMeshResourceSet() );

        auto const& defaultMaterials = GetDefaultMaterials();
        size_t const numMaterials = defaultMaterials.Count();
        size_t const numMaterialOverrides = m_materialOverrides.Count();

        m_materials.Resize( numMaterials );

        for ( size_t i = 0; i < numMaterials; i++ )
        {
            if ( i < numMaterialOverrides )
            {
                if ( m_materialOverrides[i].IsSet() )
                {
                    ENGINE_ASSERT( m_materialOverrides[i].IsLoaded() );
                    m_materials[i] = m_materialOverrides[i].GetPtr();
                }
                else
                {
                    m_materials[i] = nullptr;
                }
            }
            else // Use default material
            {
                if ( defaultMaterials[i].IsSet() )
                {
                    m_materials[i] = ( defaultMaterials[i].IsLoaded() ) ? defaultMaterials[i].GetPtr() : nullptr;
                }
                else
                {
                    m_materials[i] = nullptr;
                }
            }
        }
    }
}