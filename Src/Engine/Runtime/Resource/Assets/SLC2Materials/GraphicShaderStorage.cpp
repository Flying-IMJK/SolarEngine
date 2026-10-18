#include "GraphicShaderStorage.h"

#include "Runtime/Core/Memory/Memory.h"
#include "Runtime/Core/Serialization/MemoryReadStream.h"
#include "Runtime/Core/Serialization/MemoryWriteStream.h"

namespace SE
{
    namespace
    {
        constexpr uint32 Magic = 0x32534847; // GHS2; independent of the SLC2 artifact format.
        constexpr uint32 StateFieldCount = 23;

        void WriteState(MemoryWriteStream& stream, const SLC2GPUPipelineState::Description& state)
        {
            // Explicit fields keep compiler padding and platform enum widths out of the asset format.
            const auto& blend = state.BlendMode;
            const uint32 fields[StateFieldCount] = {
                state.DepthEnable, state.DepthWriteEnable, state.DepthClipEnable, static_cast<uint32>(state.DepthFunc),
                state.StencilEnable, state.StencilReadMask, state.StencilWriteMask,
                static_cast<uint32>(state.StencilFunc), static_cast<uint32>(state.StencilFailOp),
                static_cast<uint32>(state.StencilDepthFailOp), static_cast<uint32>(state.StencilPassOp),
                static_cast<uint32>(state.PrimitiveTopology), state.Wireframe, static_cast<uint32>(state.CullMode),
                blend.AlphaToCoverageEnable, blend.BlendEnable, static_cast<uint32>(blend.SrcBlend),
                static_cast<uint32>(blend.DestBlend), static_cast<uint32>(blend.BlendOp),
                static_cast<uint32>(blend.SrcBlendAlpha), static_cast<uint32>(blend.DestBlendAlpha),
                static_cast<uint32>(blend.BlendOpAlpha), static_cast<uint32>(blend.RenderTargetWriteMask),
            };
            stream.WriteBytes(fields, sizeof(fields));
        }

        void ReadState(MemoryReadStream& stream, SLC2GPUPipelineState::Description& state)
        {
            uint32 fields[StateFieldCount];
            stream.ReadBytes(fields, sizeof(fields));
            state.DepthEnable = fields[0] != 0;
            state.DepthWriteEnable = fields[1] != 0;
            state.DepthClipEnable = fields[2] != 0;
            state.DepthFunc = static_cast<ComparisonFunc>(fields[3]);
            state.StencilEnable = fields[4] != 0;
            state.StencilReadMask = static_cast<uint8>(fields[5]);
            state.StencilWriteMask = static_cast<uint8>(fields[6]);
            state.StencilFunc = static_cast<ComparisonFunc>(fields[7]);
            state.StencilFailOp = static_cast<StencilOperation>(fields[8]);
            state.StencilDepthFailOp = static_cast<StencilOperation>(fields[9]);
            state.StencilPassOp = static_cast<StencilOperation>(fields[10]);
            state.PrimitiveTopology = static_cast<PrimitiveTopologyType>(fields[11]);
            state.Wireframe = fields[12] != 0;
            state.CullMode = static_cast<CullMode>(fields[13]);
            auto& blend = state.BlendMode;
            blend.AlphaToCoverageEnable = fields[14] != 0;
            blend.BlendEnable = fields[15] != 0;
            blend.SrcBlend = static_cast<BlendingMode::Blend>(fields[16]);
            blend.DestBlend = static_cast<BlendingMode::Blend>(fields[17]);
            blend.BlendOp = static_cast<BlendingMode::Operation>(fields[18]);
            blend.SrcBlendAlpha = static_cast<BlendingMode::Blend>(fields[19]);
            blend.DestBlendAlpha = static_cast<BlendingMode::Blend>(fields[20]);
            blend.BlendOpAlpha = static_cast<BlendingMode::Operation>(fields[21]);
            blend.RenderTargetWriteMask = static_cast<BlendingMode::ColorWrite>(fields[22]);
        }
    }

    void GraphicShaderStorage::Encode(const List<byte>& artifact,
                                      const List<GraphicShaderPipeline>& pipelines, List<byte>& output)
    {
        MemoryWriteStream stream(256 + static_cast<uint32>(artifact.Count()));
        stream.WriteUint32(Magic);
        stream.WriteUint32(Version);
        stream.WriteUint32(static_cast<uint32>(pipelines.Count()));
        for (const GraphicShaderPipeline& pipeline : pipelines)
        {
            stream.WriteUint32(static_cast<uint32>(pipeline.ProgramId.Length()));
            stream.WriteBytes(pipeline.ProgramId.Get(), static_cast<uint32>(pipeline.ProgramId.Length() * sizeof(Char)));
            WriteState(stream, pipeline.State);
        }
        stream.WriteUint32(static_cast<uint32>(artifact.Count()));
        stream.WriteBytes(artifact.Get(), static_cast<uint32>(artifact.Count()));
        output.Resize(static_cast<int32>(stream.GetPosition()));
        Platform::MemoryCopy(output.Get(), stream.GetHandle(), stream.GetPosition());
    }

    bool GraphicShaderStorage::Decode(const byte* bytes, const int32 length, List<byte>& artifact,
                                      List<GraphicShaderPipeline>& pipelines)
    {
        if (bytes == nullptr || length < 16)
            return false;

        MemoryReadStream stream(bytes, static_cast<uint32>(length));
        uint32 magic, version, count;
        stream.ReadUint32(&magic);
        stream.ReadUint32(&version);
        stream.ReadUint32(&count);
        if (magic != Magic || version != Version || count == 0 || count > 1024)
            return false;

        List<GraphicShaderPipeline> decoded;
        for (uint32 index = 0; index < count; index++)
        {
            if (stream.GetLength() - stream.GetPosition() < sizeof(uint32))
                return false;
            uint32 chars;
            stream.ReadUint32(&chars);
            const uint32 remaining = stream.GetLength() - stream.GetPosition();
            if (chars == 0 || chars > 1024 || chars > remaining / sizeof(Char) ||
                remaining - chars * sizeof(Char) < StateFieldCount * sizeof(uint32))
                return false;

            GraphicShaderPipeline pipeline;
            pipeline.ProgramId = String(reinterpret_cast<const Char*>(stream.Move(chars * sizeof(Char))),
                                        static_cast<int32>(chars));
            ReadState(stream, pipeline.State);
            decoded.Add(MoveTemp(pipeline));
        }
        if (stream.GetLength() - stream.GetPosition() < sizeof(uint32))
            return false;
        uint32 size;
        stream.ReadUint32(&size);
        if (size == 0 || size != stream.GetLength() - stream.GetPosition())
            return false;

        List<byte> decodedArtifact;
        decodedArtifact.Resize(static_cast<int32>(size));
        stream.ReadBytes(decodedArtifact.Get(), size);
        artifact = MoveTemp(decodedArtifact);
        pipelines = MoveTemp(decoded);
        return true;
    }
} // namespace SE
