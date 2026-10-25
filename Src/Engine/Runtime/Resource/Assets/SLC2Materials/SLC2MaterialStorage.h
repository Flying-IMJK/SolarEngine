#pragma once

#include <Runtime/Render/Assets/Material/MaterialParams.h>
#include <Runtime/Resource/Assets/SLC2Materials/SLC2MaterialBinding.h>
#include <Runtime/Resource/Assets/SLC2Materials/SLC2MaterialPipeline.h>

namespace SE
{
    // Both asset types share one envelope. This type only serializes data and owns no runtime policy.
    class SLC2MaterialStorage final
    {
    public:
        static constexpr uint32 Version      = 3;
        static constexpr uint32 Magic        = 0x334D4C53; // SLM3
        static constexpr uint32 MaterialType = 1;
        static constexpr uint32 InstanceType = 2;
        static constexpr int32  Chunk        = 0;

        static void EncodeMaterial(const UID&                                shaderId,
                                   MaterialParams&                           parameters,
                                   const List<SLC2MaterialParameterBinding>& bindings,
                                   const List<SLC2MaterialPipelineState>&    pipelines,
                                   List<byte>&                               output);
        static void
        EncodeInstance(uint32 referenceKind, const UID& referenceId, MaterialParams& parameters, List<byte>& output);
        static bool DecodeMaterial(const byte*                         pBytes,
                                   int32                               length,
                                   UID&                                shaderId,
                                   MaterialParams&                     parameters,
                                   List<SLC2MaterialParameterBinding>& bindings,
                                   List<SLC2MaterialPipelineState>&    pipelines);
        static bool DecodeInstance(const byte*     pBytes,
                                   int32           length,
                                   uint32&         referenceKind,
                                   UID&            referenceId,
                                   MaterialParams& parameters);
    };
} // namespace SE
