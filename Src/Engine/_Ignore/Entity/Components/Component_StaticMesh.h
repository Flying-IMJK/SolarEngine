#pragma once

#include "Component_RenderMesh.h"
#include "Runtime/Render/RenderObject/StaticMesh.h"
#include "Core/Types/Event.h"

//-------------------------------------------------------------------------

namespace SE
{
    class SE_API_RUNTIME StaticMeshComponent final : public MeshComponent
    {
		SE_CLASS(StaticMeshComponent, MeshComponent)
//        ENGINE_ENTITY_COMPONENT( StaticMeshComponent );

    public:

        using MeshComponent::MeshComponent;

        // Local Scale
        //-------------------------------------------------------------------------

        virtual Float3 const& GetLocalScale() const override { return m_localScale; }
        virtual bool SupportsLocalScale() const override { return true; }

        // Mesh Data
        //-------------------------------------------------------------------------

        virtual bool HasMeshResourceSet() const override final { return m_mesh.IsSet(); }

        inline void SetMesh( ResID meshResourceID )
        {
            ENGINE_ASSERT( IsUnloaded() );
            ENGINE_ASSERT( meshResourceID.IsValid() );
            m_mesh = TResPtr<Render::StaticMesh>(meshResourceID);
        }

        inline Render::StaticMesh const* GetMesh() const
        {
            ENGINE_ASSERT( m_mesh != nullptr && m_mesh->IsValid() );
            return m_mesh.GetPtr();
        }

        virtual List<TResPtr<Render::Material>> const& GetDefaultMaterials() const override;

    protected:

        virtual OBB CalculateLocalBounds() const override final;

        #ifdef SE_DEVELOPMENT
        virtual void PostPropertyEdit( PropertyInfo const* pPropertyEdited ) override;
        #endif

    protected:

        // A local scale that doesnt propagate but that can allow for non-uniform scaling of meshes
        SE_FIELD() Float3 m_localScale = Float3::One;

    private:

        // The mesh resource for this component
//        SE_PROPERTY()
		TResPtr<Render::StaticMesh>              m_mesh;
    };
}