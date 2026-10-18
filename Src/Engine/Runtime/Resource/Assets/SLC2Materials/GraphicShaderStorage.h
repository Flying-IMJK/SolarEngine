#pragma once

#include "GraphicShader.h"

namespace SE
{
    // Chunk 0: independent, versioned GraphicShader envelope around an unmodified SLC2 artifact.
    class GraphicShaderStorage final
    {
    public:
        static constexpr uint32 Version = 1;
        static constexpr int32 Chunk = 0;

        static void Encode(const List<byte>& artifact, const List<GraphicShaderPipeline>& pipelines, List<byte>& output);
        static bool Decode(const byte* bytes, int32 length, List<byte>& artifact,
                           List<GraphicShaderPipeline>& pipelines);
    };
} // namespace SE
