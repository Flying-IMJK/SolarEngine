#pragma once

#include "Runtime/Graphics/Base/RenderGeometry.h"
#include "Runtime/ShaderCompilation/ShaderCompileTypes.h"
#include "Runtime/Resource/Assets/SLC2Materials/SLC2MaterialPipeline.h"

namespace SE
{
    class SLC2Material;
    class SLC2MaterialInstance;
    struct RenderContext;
    struct DrawCall;


    // RenderList owns a copy until Clear; transient value sources must survive until submission.
    // For non-indexed draws, DrawCall::Draw.IndicesCount carries the vertex count.
    struct SLC2RenderDraw
    {
        SLC2Material* Material = nullptr;
        SLC2MaterialInstance* Instance = nullptr;
        RenderGeometry Geometry;
        ShaderCompileTarget Target;
        List<String> Defines;
        SLC2MaterialGeometryMode GeometryMode = SLC2MaterialGeometryMode::Static;
    };
} // namespace SE
