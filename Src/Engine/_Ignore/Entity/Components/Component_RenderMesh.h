#pragma once

#include "Runtime/API.h"
#include "Runtime/Entity/EntitySpatialComponent.h"

//-------------------------------------------------------------------------

namespace SE
{
    class SE_API_RUNTIME MeshComponent : public SpatialEntityComponent
    {
		SE_CLASS(MeshComponent, SpatialEntityComponent);
//        ENGINE_ENTITY_COMPONENT( MeshComponent );

    public:

        inline MeshComponent() = default;
        inline MeshComponent( StringID name ) : SpatialEntityComponent( name ) {}

        // Does this component have a valid mesh set
        virtual bool HasMeshResourceSet() const = 0;

        // Visibility
        //-------------------------------------------------------------------------

        // Is this mesh visible
        inline bool IsVisible() const { return m_isVisible; }

        // Set the entire mesh visibility
        inline void SetVisible( bool isVisible ) { m_isVisible = isVisible; }

        // Set section visibility
        inline void SetSectionVisibility( int32 sectionIdx, bool isVisible )
        {
            ENGINE_ASSERT( sectionIdx >= 0 && sectionIdx < 64 );
            if ( isVisible )
            {
                m_sectionVisibilityMask |=  1ull << sectionIdx;
            }
            else
            {
                m_sectionVisibilityMask &= ~( 1ull << sectionIdx );
            }
        }

        // Is a given section visible?
        inline bool IsSectionVisible( int32 sectionIdx ) const
        {
            ENGINE_ASSERT( sectionIdx >= 0 && sectionIdx < 64 );
            return ( m_sectionVisibilityMask & ( 1ull << sectionIdx ) ) != 0;
        }

        // Get the section visibility mask
        inline uint64 GetSectionVisibilityMask() const { return m_sectionVisibilityMask; }

        // Materials
        //-------------------------------------------------------------------------

        inline int32 GetNumRequiredMaterials() const { return (int32) GetDefaultMaterials().Count(); }
        inline List<Render::Material const*> const& GetMaterials() const { return m_materials; }
        void SetMaterialOverride( int32 materialIdx, ResID materialResourceID );

    protected:

        virtual void Initialize() override;
        virtual void Shutdown() override;

        // Get the default materials for the set mesh
        virtual List<TResPtr<Render::Material>> const& GetDefaultMaterials() const = 0;

    private:

        void UpdateMaterialCache();

    private:

        // Any user material overrides
//        SE_PROPERTY();
        List<TResPtr<Render::Material>>                         m_materialOverrides;

        // The final set of materials to use for this component
        List<Render::Material const*>                           m_materials;

        uint64                                                	m_sectionVisibilityMask = 0xFFFFFFFF;

        // Should this component be rendered
        bool                                                    m_isVisible = true;
    };
}