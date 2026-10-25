#include <Runtime/Resource/Assets/SLC2Materials/SLC2MaterialStorage.h>

#include <Runtime/Core/Memory/Memory.h>
#include <Runtime/Core/Serialization/MemoryReadStream.h>
#include <Runtime/Core/Serialization/MemoryWriteStream.h>

namespace SE
{
    namespace
    {
        constexpr uint32 StateFieldCount       = 23;
        constexpr uint32 MaximumParameterBytes = 64 * 1024 * 1024;

        void WriteText(MemoryWriteStream& stream, const String& value)
        {
            stream.WriteUint32(static_cast<uint32>(value.Length()));
            stream.WriteBytes(value.Get(), static_cast<uint32>(value.Length() * sizeof(Char)));
        }

        bool ReadText(MemoryReadStream& stream, String& value, const uint32 maximumLength)
        {
            if (stream.GetLength() - stream.GetPosition() < sizeof(uint32))
                return false;
            uint32 length;
            stream.ReadUint32(&length);
            const uint32 remaining = stream.GetLength() - stream.GetPosition();
            if (length == 0 || length > maximumLength || length > remaining / sizeof(Char))
                return false;
            value =
                String(reinterpret_cast<const Char*>(stream.Move(length * sizeof(Char))), static_cast<int32>(length));
            return true;
        }

        void WriteParameters(MemoryWriteStream& stream, MaterialParams& parameters)
        {
            MemoryWriteStream parameterStream(256);
            parameters.Save(&parameterStream);
            stream.WriteUint32(parameterStream.GetPosition());
            if (parameterStream.GetPosition() != 0)
                stream.WriteBytes(parameterStream.GetHandle(), parameterStream.GetPosition());
        }

        bool ReadParameters(MemoryReadStream& stream, MaterialParams& parameters)
        {
            if (stream.GetLength() - stream.GetPosition() < sizeof(uint32))
                return false;
            uint32 length;
            stream.ReadUint32(&length);
            if (length > MaximumParameterBytes || length > stream.GetLength() - stream.GetPosition())
                return false;
            if (length == 0)
            {
                parameters.Dispose();
                return true;
            }
            MemoryReadStream parameterStream(static_cast<const byte*>(stream.Move(length)), length);
            return !parameters.Load(&parameterStream) && parameterStream.GetPosition() == parameterStream.GetLength();
        }

        void WriteState(MemoryWriteStream& stream, const SLC2GPUPipelineState::Description& state)
        {
            const BlendingMode& blend                   = state.BlendMode;
            const uint32        fields[StateFieldCount] = {
                state.DepthEnable,
                state.DepthWriteEnable,
                state.DepthClipEnable,
                static_cast<uint32>(state.DepthFunc),
                state.StencilEnable,
                state.StencilReadMask,
                state.StencilWriteMask,
                static_cast<uint32>(state.StencilFunc),
                static_cast<uint32>(state.StencilFailOp),
                static_cast<uint32>(state.StencilDepthFailOp),
                static_cast<uint32>(state.StencilPassOp),
                static_cast<uint32>(state.PrimitiveTopology),
                state.Wireframe,
                static_cast<uint32>(state.CullMode),
                blend.AlphaToCoverageEnable,
                blend.BlendEnable,
                static_cast<uint32>(blend.SrcBlend),
                static_cast<uint32>(blend.DestBlend),
                static_cast<uint32>(blend.BlendOp),
                static_cast<uint32>(blend.SrcBlendAlpha),
                static_cast<uint32>(blend.DestBlendAlpha),
                static_cast<uint32>(blend.BlendOpAlpha),
                static_cast<uint32>(blend.RenderTargetWriteMask),
            };
            stream.WriteBytes(fields, sizeof(fields));
        }

        bool ReadState(MemoryReadStream& stream, SLC2GPUPipelineState::Description& state)
        {
            if (stream.GetLength() - stream.GetPosition() < StateFieldCount * sizeof(uint32))
                return false;
            uint32 fields[StateFieldCount];
            stream.ReadBytes(fields, sizeof(fields));
            state.DepthEnable           = fields[0] != 0;
            state.DepthWriteEnable      = fields[1] != 0;
            state.DepthClipEnable       = fields[2] != 0;
            state.DepthFunc             = static_cast<ComparisonFunc>(fields[3]);
            state.StencilEnable         = fields[4] != 0;
            state.StencilReadMask       = static_cast<uint8>(fields[5]);
            state.StencilWriteMask      = static_cast<uint8>(fields[6]);
            state.StencilFunc           = static_cast<ComparisonFunc>(fields[7]);
            state.StencilFailOp         = static_cast<StencilOperation>(fields[8]);
            state.StencilDepthFailOp    = static_cast<StencilOperation>(fields[9]);
            state.StencilPassOp         = static_cast<StencilOperation>(fields[10]);
            state.PrimitiveTopology     = static_cast<PrimitiveTopologyType>(fields[11]);
            state.Wireframe             = fields[12] != 0;
            state.CullMode              = static_cast<CullMode>(fields[13]);
            BlendingMode& blend         = state.BlendMode;
            blend.AlphaToCoverageEnable = fields[14] != 0;
            blend.BlendEnable           = fields[15] != 0;
            blend.SrcBlend              = static_cast<BlendingMode::Blend>(fields[16]);
            blend.DestBlend             = static_cast<BlendingMode::Blend>(fields[17]);
            blend.BlendOp               = static_cast<BlendingMode::Operation>(fields[18]);
            blend.SrcBlendAlpha         = static_cast<BlendingMode::Blend>(fields[19]);
            blend.DestBlendAlpha        = static_cast<BlendingMode::Blend>(fields[20]);
            blend.BlendOpAlpha          = static_cast<BlendingMode::Operation>(fields[21]);
            blend.RenderTargetWriteMask = static_cast<BlendingMode::ColorWrite>(fields[22]);
            return SLC2MaterialPipeline::ValidateState(state);
        }

        void
        WriteHeader(MemoryWriteStream& stream, const uint32 type, const uint32 referenceKind, const UID& referenceId)
        {
            stream.WriteUint32(SLC2MaterialStorage::Magic);
            stream.WriteUint32(SLC2MaterialStorage::Version);
            stream.WriteUint32(type);
            stream.WriteUint32(referenceKind);
            stream.Write(referenceId);
        }

        bool ReadHeader(MemoryReadStream& stream, const uint32 expectedType, uint32& referenceKind, UID& referenceId)
        {
            if (stream.GetLength() - stream.GetPosition() < 4 * sizeof(uint32) + sizeof(UID))
                return false;
            uint32 magic;
            uint32 version;
            uint32 type;
            stream.ReadUint32(&magic);
            stream.ReadUint32(&version);
            stream.ReadUint32(&type);
            stream.ReadUint32(&referenceKind);
            stream.Read(referenceId);
            return magic == SLC2MaterialStorage::Magic && version == SLC2MaterialStorage::Version &&
                   type == expectedType && referenceKind <= 1 &&
                   (type != SLC2MaterialStorage::MaterialType || referenceKind == 0);
        }
    } // namespace

    void SLC2MaterialStorage::EncodeMaterial(const UID&                                shaderId,
                                             MaterialParams&                           parameters,
                                             const List<SLC2MaterialParameterBinding>& bindings,
                                             const List<SLC2MaterialPipelineState>&    pipelines,
                                             List<byte>&                               output)
    {
        MemoryWriteStream stream(512);
        WriteHeader(stream, MaterialType, 0, shaderId);
        WriteParameters(stream, parameters);
        stream.WriteUint32(static_cast<uint32>(bindings.Count()));
        for (const SLC2MaterialParameterBinding& binding : bindings)
        {
            stream.Write(binding.ParameterID);
            WriteText(stream, binding.NamePath);
            stream.WriteByte(static_cast<byte>(binding.BindingKind));
        }
        stream.WriteUint32(static_cast<uint32>(pipelines.Count()));
        for (const SLC2MaterialPipelineState& pipeline : pipelines)
        {
            WriteText(stream, pipeline.ProgramId);
            WriteState(stream, pipeline.State);
        }
        output.Resize(static_cast<int32>(stream.GetPosition()));
        Platform::MemoryCopy(output.Get(), stream.GetHandle(), stream.GetPosition());
    }

    void SLC2MaterialStorage::EncodeInstance(const uint32    referenceKind,
                                             const UID&      referenceId,
                                             MaterialParams& parameters,
                                             List<byte>&     output)
    {
        MemoryWriteStream stream(256);
        WriteHeader(stream, InstanceType, referenceKind, referenceId);
        WriteParameters(stream, parameters);
        output.Resize(static_cast<int32>(stream.GetPosition()));
        Platform::MemoryCopy(output.Get(), stream.GetHandle(), stream.GetPosition());
    }

    bool SLC2MaterialStorage::DecodeMaterial(const byte*                         pBytes,
                                             const int32                         length,
                                             UID&                                shaderId,
                                             MaterialParams&                     parameters,
                                             List<SLC2MaterialParameterBinding>& bindings,
                                             List<SLC2MaterialPipelineState>&    pipelines)
    {
        if (pBytes == nullptr || length <= 0)
            return false;
        MemoryReadStream                   stream(pBytes, static_cast<uint32>(length));
        uint32                             referenceKind;
        UID                                decodedShaderId;
        MaterialParams                     decodedParameters;
        List<SLC2MaterialParameterBinding> decodedBindings;
        List<SLC2MaterialPipelineState>    decodedPipelines;
        if (!ReadHeader(stream, MaterialType, referenceKind, decodedShaderId) || decodedShaderId == UID::Empty ||
            !ReadParameters(stream, decodedParameters) || stream.GetLength() - stream.GetPosition() < sizeof(uint32))
            return false;

        uint32 count;
        stream.ReadUint32(&count);
        if (count > 65536)
            return false;
        for (uint32 index = 0; index < count; ++index)
        {
            if (stream.GetLength() - stream.GetPosition() < sizeof(UID))
                return false;
            SLC2MaterialParameterBinding binding;
            stream.Read(binding.ParameterID);
            if (binding.ParameterID == UID::Empty || !ReadText(stream, binding.NamePath, 4096) ||
                stream.GetLength() - stream.GetPosition() < sizeof(byte))
                return false;
            binding.BindingKind = static_cast<ShaderProgramBindingKind>(stream.ReadByte());
            if (binding.BindingKind < ShaderProgramBindingKind::Uniform ||
                binding.BindingKind > ShaderProgramBindingKind::Sampler)
                return false;
            decodedBindings.Add(MoveTemp(binding));
        }

        if (stream.GetLength() - stream.GetPosition() < sizeof(uint32))
            return false;
        stream.ReadUint32(&count);
        if (count > 1024)
            return false;
        for (uint32 index = 0; index < count; ++index)
        {
            SLC2MaterialPipelineState pipeline;
            if (!ReadText(stream, pipeline.ProgramId, 4096) || !ReadState(stream, pipeline.State))
                return false;
            decodedPipelines.Add(MoveTemp(pipeline));
        }
        if (stream.GetPosition() != stream.GetLength())
            return false;

        shaderId   = decodedShaderId;
        parameters = MoveTemp(decodedParameters);
        bindings   = MoveTemp(decodedBindings);
        pipelines  = MoveTemp(decodedPipelines);
        return true;
    }

    bool SLC2MaterialStorage::DecodeInstance(const byte*     pBytes,
                                             const int32     length,
                                             uint32&         referenceKind,
                                             UID&            referenceId,
                                             MaterialParams& parameters)
    {
        if (pBytes == nullptr || length <= 0)
            return false;
        MemoryReadStream stream(pBytes, static_cast<uint32>(length));
        uint32           decodedReferenceKind;
        UID              decodedReferenceId;
        MaterialParams   decodedParameters;
        if (!ReadHeader(stream, InstanceType, decodedReferenceKind, decodedReferenceId) ||
            decodedReferenceId == UID::Empty || !ReadParameters(stream, decodedParameters) ||
            stream.GetPosition() != stream.GetLength())
            return false;
        referenceKind = decodedReferenceKind;
        referenceId   = decodedReferenceId;
        parameters    = MoveTemp(decodedParameters);
        return true;
    }
} // namespace SE
