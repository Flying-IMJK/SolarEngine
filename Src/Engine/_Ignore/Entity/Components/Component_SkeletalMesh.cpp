#include "Component_SkeletalMesh.h"
//#include "Engine/Animation/AnimationPose.h"
#include "Runtime/Render/Drawing/DebugDrawing.h"
#include "Core/Profiler/Profiler.h"

//-------------------------------------------------------------------------

namespace SGE
{
    OBB SkeletalMeshComponent::CalculateLocalBounds() const
    {
        OBB bounds;

        if ( HasMeshResourceSet() )
        {
            if ( m_boneTransforms.IsEmpty())
            {
                bounds = m_mesh->GetBounds();
            }
            else // Use bones to calculate bounds
            {
                AABB newBounds;
                for ( auto const& boneTransform : m_boneTransforms )
                {
                    newBounds.AddPoint( boneTransform.GetTranslation() );
                }

                bounds = OBB( newBounds );
            }
        }
        else
        {
            bounds = MeshComponent::CalculateLocalBounds();
        }

        return bounds;
    }

    void SkeletalMeshComponent::Initialize()
    {
        MeshComponent::Initialize();

        if ( HasMeshResourceSet() )
        {
            ENGINE_ASSERT( m_mesh.IsLoaded() );

            if ( HasSkeletonResourceSet() )
            {
                GenerateAnimationBoneMap();
            }

            // Set mesh to reference pose
            //-------------------------------------------------------------------------

            m_boneTransforms.Resize( m_mesh->GetNumBones() );
            ResetPose();

            //-------------------------------------------------------------------------

            // Allocate skinning transforms and calculate initial values
            m_skinningTransforms.Resize( m_boneTransforms.Count() );
            FinalizePose();
        }
    }

    void SkeletalMeshComponent::Shutdown()
    {
        m_boneTransforms.Clear();
        m_skinningTransforms.Clear();
        m_animToMeshBoneMap.Clear();
        MeshComponent::Shutdown();
    }

    List<TResPtr<Render::Material>> const& SkeletalMeshComponent::GetDefaultMaterials() const
    {
        ENGINE_ASSERT( IsInitialized() && HasMeshResourceSet() );
        return m_mesh->GetMaterials();
    }

    bool SkeletalMeshComponent::TryFindAttachmentSocketTransform( StringID socketID, Transform& outSocketWorldTransform ) const
    {
        ENGINE_ASSERT( socketID.IsValid() );

        outSocketWorldTransform = GetWorldTransform();

        if ( m_mesh.IsSet() && m_mesh.IsLoaded() )
        {
            auto const boneIdx = m_mesh->GetBoneIndex( socketID );
            if ( boneIdx != INVALID_INDEX )
            {
                if ( IsInitialized() )
                {
                    outSocketWorldTransform = m_boneTransforms[boneIdx] * outSocketWorldTransform;
                }
                else
                {
                    outSocketWorldTransform = m_mesh->GetBindPose()[boneIdx] * outSocketWorldTransform;
                }

                return true;
            }
        }

        return false;
    }

    bool SkeletalMeshComponent::HasSocket( StringID socketID ) const
    {
        ENGINE_ASSERT( socketID.IsValid() );

        if ( m_mesh.IsSet() && m_mesh.IsLoaded() )
        {
            int32_t boneIdx = m_mesh->GetBoneIndex( socketID );
            return boneIdx != INVALID_INDEX;
        }

        return false;
    }

    //-------------------------------------------------------------------------

    void SkeletalMeshComponent::SetSkeleton( ResID skeletonResourceID )
    {
        ENGINE_ASSERT( IsUnloaded() );
        ENGINE_ASSERT( skeletonResourceID.IsValid() );
//        m_Skeleton = skeletonResourceID;
    }

	/* void SkeletalMeshComponent::SetPose( Animation::Pose const* pPose )
	{
		ENGINE_ASSERT( IsInitialized() );
		ENGINE_ASSERT( HasMeshResourceSet() && HasSkeletonResourceSet() );
		ENGINE_ASSERT( !m_animToMeshBoneMap.IsEmpty() );
	    ENGINE_ASSERT( pPose != nullptr && pPose->HasModelSpaceTransforms() );

		int32_t const numAnimBones = pPose->GetNumBones();
		for ( auto animBoneIdx = 0; animBoneIdx < numAnimBones; animBoneIdx++ )
		{
			int32_t const meshBoneIdx = m_animToMeshBoneMap[animBoneIdx];
			if ( meshBoneIdx != INVALID_INDEX )
			{
				Transform const boneTransform = pPose->GetGlobalTransform( animBoneIdx );
				m_boneTransforms[meshBoneIdx] = boneTransform;
			}
		}
	}*/

    void SkeletalMeshComponent::ResetPose()
    {
        ENGINE_ASSERT( IsInitialized() );

        if ( HasSkeletonResourceSet() )
        {
/*            Animation::Pose referencePose( m_skeleton.GetPtr() );
            referencePose.CalculateModelSpaceTransforms();
            SetPose( &referencePose );*/
        }
        else
        {
            m_boneTransforms = m_mesh->GetBindPose();
        }
    }

    void SkeletalMeshComponent::FinalizePose()
    {
        ENGINE_ASSERT( m_mesh.IsSet() && m_mesh.IsLoaded() );

        NotifySocketsUpdated();
        UpdateBounds();
        UpdateSkinningTransforms();
    }

    //-------------------------------------------------------------------------

    void SkeletalMeshComponent::UpdateSkinningTransforms()
    {
        ENGINE_ASSERT( m_mesh.IsSet() && m_mesh.IsLoaded() );

        auto const numBones = m_boneTransforms.Count();
        ENGINE_ASSERT( m_skinningTransforms.Count() == numBones );

        auto const& inverseBindPose = m_mesh->GetInverseBindPose();
        for ( auto i = 0; i < numBones; i++ )
        {
            Transform const skinningTransform = inverseBindPose[i] * m_boneTransforms[i];
            m_skinningTransforms[i] = ( skinningTransform ).ToMatrix();
        }
    }

    void SkeletalMeshComponent::GenerateAnimationBoneMap()
    {
        /*ENGINE_ASSERT( m_mesh != nullptr && m_skeleton != nullptr );

        auto const pMesh = GetMesh();

        auto const numBones = m_skeleton->GetNumBones();
        m_animToMeshBoneMap.Resize( numBones, INVALID_INDEX );

        for ( auto boneIdx = 0; boneIdx < numBones; boneIdx++ )
        {
            auto const& boneID = m_skeleton->GetBoneID( boneIdx );
            m_animToMeshBoneMap[boneIdx] = pMesh->GetBoneIndex( boneID );
        }*/
    }

    //-------------------------------------------------------------------------

    #ifdef SE_DEVELOPMENT
    void SkeletalMeshComponent::DrawPose( Drawing::DrawContext& drawingContext ) const
    {
        ENGINE_ASSERT( IsInitialized() );

        if ( !m_mesh.IsSet() || !m_mesh.IsLoaded() )
        {
            return;
        }

        //-------------------------------------------------------------------------

        Transform const& worldTransform = GetWorldTransform();
        auto const numBones = m_boneTransforms.Count();

/*        Transform boneWorldTransform = m_boneTransforms[0] * worldTransform;
        drawingContext.DrawBox( boneWorldTransform, Float3( 0.005f ), Colors::Orange.ToFloat4() );
        drawingContext.DrawAxis( boneWorldTransform, 0.05f );*/

/*        for ( auto i = 1; i < numBones; i++ )
        {
            boneWorldTransform = m_boneTransforms[i] * worldTransform;

            auto const parentBoneIdx = m_mesh->GetParentBoneIndex( i );
            Transform const parentBoneWorldTransform = m_boneTransforms[parentBoneIdx] * worldTransform;

            drawingContext.DrawLine( parentBoneWorldTransform.GetTranslation(), boneWorldTransform.GetTranslation(), Colors::Orange.ToFloat4() );
            drawingContext.DrawAxis( boneWorldTransform, 0.03f, 2.0f );
        }*/
    }
    #endif
}
