#include "Component_StaticMesh.h"
#include "Runtime/Entity/EntityLog.h"

//-------------------------------------------------------------------------

namespace SGE
{
    OBB StaticMeshComponent::CalculateLocalBounds() const
    {
        if ( HasMeshResourceSet() )
        {
            OBB scaledMeshBounds = m_mesh->GetBounds();
            scaledMeshBounds.m_Center *= m_localScale;
            scaledMeshBounds.m_Extents *= m_localScale;
            return scaledMeshBounds;
        }
        else
        {
            return MeshComponent::CalculateLocalBounds();
        }
    }

    List<TResPtr<Render::Material>> const& StaticMeshComponent::GetDefaultMaterials() const
    {
        ENGINE_ASSERT( IsInitialized() && HasMeshResourceSet() );
        return m_mesh->GetMaterials();
    }

    //-------------------------------------------------------------------------

    #ifdef SE_DEVELOPMENT
    void StaticMeshComponent::PostPropertyEdit( PropertyInfo const* pPropertyEdited )
    {
        MeshComponent::PostPropertyEdit( pPropertyEdited );

        if ( Math::IsNearZero( m_localScale.x ) ) m_localScale.x = 0.1f;
        if ( Math::IsNearZero( m_localScale.y ) ) m_localScale.y = 0.1f;
        if ( Math::IsNearZero( m_localScale.z ) ) m_localScale.z = 0.1f;
    }
    #endif
}