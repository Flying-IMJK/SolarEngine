#pragma once
#include "Runtime/Entity/EntityWorldSettings.h"

//-------------------------------------------------------------------------

namespace SE::Render
{
    #ifdef SE_DEVELOPMENT
    enum class DebugVisualizationMode : int8
    {
        Lighting = 0,
        Albedo = 1,
        Normals = 2,
        Metalness = 3,
        Roughness = 4,
        AmbientOcclusion = 5,

        BitShift = 32 - 3,
    };
    #endif

    //-------------------------------------------------------------------------

    class RenderWorldSettings : public IEntityWorldSettings
    {
        DEFINE_CLASS(RenderWorldSettings);

    public:

        #ifdef SE_DEVELOPMENT
        DebugVisualizationMode              m_visualizationMode = DebugVisualizationMode::Lighting;
        bool                                m_showStaticMeshBounds = false;
        bool                                m_showSkeletalMeshBounds = false;
        bool                                m_showSkeletalMeshBones = false;
        bool                                m_showSkeletalMeshBindPoses = false;
        #endif
    };
}