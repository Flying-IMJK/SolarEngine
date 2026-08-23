#pragma once

#include "Component_RenderMesh.h"
#include "Runtime/Render/RenderObject/SkeletalMesh.h"
//#include "Engine/Animation/AnimationSkeleton.h"

//-------------------------------------------------------------------------

namespace SE::Animation { class Pose; }

//-------------------------------------------------------------------------

namespace SE
{
    class SE_API_RUNTIME SkeletalMeshComponent : public MeshComponent
    {
		SE_CLASS(SkeletalMeshComponent, MeshComponent)
//        ENGINE_ENTITY_COMPONENT( SkeletalMeshComponent );

    public:

        using MeshComponent::MeshComponent;

        // Mesh Data
        //-------------------------------------------------------------------------

        virtual bool HasMeshResourceSet() const override final { return m_mesh.IsSet(); }

        inline void SetMesh( ResID meshResourceID )
        {
            ENGINE_ASSERT( IsUnloaded() );
            ENGINE_ASSERT( meshResourceID.IsValid() );
            m_mesh = TResPtr<Render::SkeletalMesh>(meshResourceID);
        }

        inline Render::SkeletalMesh const* GetMesh() const
        {
            ENGINE_ASSERT( m_mesh != nullptr && m_mesh->IsValid() );
            return m_mesh.GetPtr();
        }

        // Skeletal Pose
        //-------------------------------------------------------------------------

        // Get the character space (global) transforms for the mesh
        inline List<Transform> const& GetBoneTransforms() const { return m_boneTransforms; }

        // The the global space transform for a specific bone
        inline void SetBoneTransform( int32_t boneIdx, Transform const& transform )
        {
            ENGINE_ASSERT( boneIdx >= 0 && boneIdx < m_boneTransforms.Count() );
            m_boneTransforms[boneIdx] = transform;
        }

        // This function will finalize the pose, run any procedural bone solvers and generate the skinning transforms
        // Only run this function once per frame once you have set the final global pose
        void FinalizePose();

        // Get the skinning transforms for this mesh - these are the global transforms relative to the bind pose
        inline List<Matrix> const& GetSkinningTransforms() const { return m_skinningTransforms; }

        // Animation Pose
        //-------------------------------------------------------------------------

        inline bool HasSkeletonResourceSet() const { return false; } //return m_skeleton.IsSet(); }
//        inline Animation::Skeleton const* GetSkeleton() const { return m_skeleton.GetPtr(); }
        void SetSkeleton( ResID skeletonResourceID );

//        void SetPose( Animation::Pose const* pPose );

        void ResetPose();

        // Debug
        //-------------------------------------------------------------------------

        #ifdef SE_DEVELOPMENT
        void DrawPose( Drawing::DrawContext& drawingContext ) const;
        #endif

    protected:

        virtual List<TResPtr<Render::Material>> const& GetDefaultMaterials() const override final;

        void UpdateSkinningTransforms();
        void GenerateAnimationBoneMap();

        virtual OBB CalculateLocalBounds() const override final;

        virtual void Initialize() override;
        virtual void Shutdown() override;

        virtual bool TryFindAttachmentSocketTransform( StringID socketID, Transform& outSocketWorldTransform ) const override final;
        virtual bool HasSocket( StringID socketID ) const override final;

    protected:

//        SE_PROPERTY()
        TResPtr<Render::SkeletalMesh>           m_mesh;
//        SE_PROPERTY()
//        TResPtr<Animation::Skeleton>            m_skeleton = nullptr;
        List<int32_t>                                m_animToMeshBoneMap;
        List<Transform>                              m_boneTransforms;
        List<Matrix>                                 m_skinningTransforms;
    };

    //-------------------------------------------------------------------------

    // We often have the need to find the specific mesh component that is the main character mesh.
    // This class makes it explicit, no need for name or tag matching!
    class SE_API_RUNTIME CharacterMeshComponent final : public SkeletalMeshComponent
    {
		SE_CLASS(CharacterMeshComponent, SkeletalMeshComponent)
//        ENGINE_SINGLETON_ENTITY_COMPONENT( CharacterMeshComponent );

        using SkeletalMeshComponent::SkeletalMeshComponent;
    };
}