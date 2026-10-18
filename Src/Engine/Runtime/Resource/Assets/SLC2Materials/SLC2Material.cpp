#include "SLC2Material.h"

#include "Runtime/Core/Memory/Memory.h"
#include "Runtime/Core/Serialization/MemoryReadStream.h"
#include "Runtime/Core/Serialization/MemoryWriteStream.h"
#include "Runtime/Graphics/Shaders/GPUConstantBuffer.h"
#include "Runtime/Graphics/Shaders/Reflection/ShaderNameResolver.h"
#include "Runtime/Graphics/Shaders/Reflection/ShaderProgramReflection.h"
#include "Runtime/Graphics/Shaders/SLC2GPUShaderProgram.h"
#include "Runtime/Graphics/Textures/GPUTexture.h"
#include "Runtime/Render/Assets/Texture/TextureBase.h"
#include "Runtime/Resource/Factories/BinaryAssetFactory.h"
#include "Runtime/Resource/Storage/StorageChunk.h"

namespace SE
{
    BINARY_ASSET_FACTORY(SLC2Material, true);
    BINARY_ASSET_FACTORY(SLC2MaterialInstance, true);

    class SLC2MaterialStorage final
    {
    public:
        static constexpr uint32 Version = 1;
        static constexpr uint32 Magic = 0x324D4C53; // SLM2
        static constexpr uint32 MaterialType = 1;
        static constexpr uint32 InstanceType = 2;
        static constexpr int32 Chunk = 0;

        static void Encode(uint32 type, uint32 referenceKind, const UID& reference,
                           const SLC2MaterialParameters& parameters,
                           List<byte>& output)
        {
            MemoryWriteStream stream(256);
            stream.WriteUint32(Magic);
            stream.WriteUint32(Version);
            stream.WriteUint32(type);
            stream.WriteUint32(referenceKind);
            stream.Write(reference);
            const auto& values = parameters.GetValues();
            stream.WriteUint32(static_cast<uint32>(values.Count()));
            for (const SLC2MaterialStoredValue& value : values)
            {
                stream.WriteUint32(static_cast<uint32>(value.Path.Length()));
                stream.WriteBytes(value.Path.Get(), static_cast<uint32>(value.Path.Length() * sizeof(Char)));
                stream.WriteByte(static_cast<byte>(value.Kind));
                if (value.Kind == SLC2MaterialStoredKind::TextureAsset)
                    stream.Write(value.TextureId);
                else
                {
                    stream.WriteUint32(static_cast<uint32>(value.Bytes.Count()));
                    stream.WriteBytes(value.Bytes.Get(), static_cast<uint32>(value.Bytes.Count()));
                }
            }
            output.Resize(static_cast<int32>(stream.GetPosition()));
            Platform::MemoryCopy(output.Get(), stream.GetHandle(), stream.GetPosition());
        }

        static bool Decode(const byte* bytes, int32 length, uint32 expectedType,
                           uint32& referenceKind, UID& reference,
                           SLC2MaterialParameters& parameters)
        {
            if (bytes == nullptr || length < 5 * sizeof(uint32) + sizeof(UID))
                return false;
            MemoryReadStream stream(bytes, static_cast<uint32>(length));
            uint32 magic, version, type, count;
            stream.ReadUint32(&magic);
            stream.ReadUint32(&version);
            stream.ReadUint32(&type);
            if (magic != Magic || version != Version || type != expectedType)
                return false;
            stream.ReadUint32(&referenceKind);
            if (referenceKind > 1 || (type == MaterialType && referenceKind != 0))
                return false;
            stream.Read(reference);
            stream.ReadUint32(&count);
            if (count > 65536)
                return false;

            SLC2MaterialParameters decoded;
            for (uint32 index = 0; index < count; index++)
            {
                if (stream.GetLength() - stream.GetPosition() < sizeof(uint32))
                    return false;
                uint32 chars;
                stream.ReadUint32(&chars);
                uint32 remaining = stream.GetLength() - stream.GetPosition();
                if (chars == 0 || chars > 4096 || chars > remaining / sizeof(Char) ||
                    remaining - chars * sizeof(Char) < sizeof(byte))
                    return false;
                const String path(reinterpret_cast<const Char*>(stream.Move(chars * sizeof(Char))),
                                  static_cast<int32>(chars));
                const byte kind = stream.ReadByte();
                if (decoded.Find(path) != nullptr)
                    return false;
                remaining = stream.GetLength() - stream.GetPosition();
                if (kind == static_cast<byte>(SLC2MaterialStoredKind::TextureAsset))
                {
                    if (remaining < sizeof(UID))
                        return false;
                    UID textureId;
                    stream.Read(textureId);
                    if (textureId == UID::Empty)
                        return false;
                    if (!decoded.SetTextureReference(path, textureId))
                        return false;
                }
                else if (kind == static_cast<byte>(SLC2MaterialStoredKind::Uniform) ||
                         kind == static_cast<byte>(SLC2MaterialStoredKind::ConstantBuffer))
                {
                    if (remaining < sizeof(uint32))
                        return false;
                    uint32 size;
                    stream.ReadUint32(&size);
                    if (size == 0 || size > stream.GetLength() - stream.GetPosition())
                        return false;
                    const void* data = stream.Move(size);
                    const bool valid = kind == static_cast<byte>(SLC2MaterialStoredKind::Uniform)
                        ? decoded.SetUniform(path, data, size) : decoded.SetConstantBuffer(path, data, size);
                    if (!valid)
                        return false;
                }
                else
                    return false;
            }
            if (stream.GetPosition() != stream.GetLength())
                return false;
            parameters = MoveTemp(decoded);
            return true;
        }
    };

    bool SLC2MaterialParameters::SetBytes(const String& path, SLC2MaterialStoredKind kind,
                                          const void* bytes, uint32 size)
    {
        if (path.IsEmpty() || bytes == nullptr || size == 0)
            return false;
        SLC2MaterialStoredValue* value = nullptr;
        for (auto& candidate : m_Values)
        {
            if (candidate.Path == path)
            {
                value = &candidate;
                break;
            }
        }
        if (value == nullptr)
            value = &m_Values.AddOne();
        value->Path = path;
        value->Kind = kind;
        value->TextureId = UID::Empty;
        value->Texture = nullptr;
        value->Bytes.Resize(static_cast<int32>(size));
        Platform::MemoryCopy(value->Bytes.Get(), bytes, size);
        return true;
    }

    bool SLC2MaterialParameters::SetUniform(const String& path, const void* bytes, uint32 size)
    {
        return SetBytes(path, SLC2MaterialStoredKind::Uniform, bytes, size);
    }

    bool SLC2MaterialParameters::SetConstantBuffer(const String& path, const void* bytes, uint32 size)
    {
        return SetBytes(path, SLC2MaterialStoredKind::ConstantBuffer, bytes, size);
    }

    bool SLC2MaterialParameters::SetTexture(const String& path, TextureBase* texture)
    {
        if (texture == nullptr || !SetTextureReference(path, texture->GetID()))
            return false;
        for (auto& value : m_Values)
            if (value.Path == path)
                value.Texture = texture;
        return true;
    }

    bool SLC2MaterialParameters::SetTextureReference(const String& path, const UID& textureId)
    {
        if (path.IsEmpty() || textureId == UID::Empty)
            return false;
        SLC2MaterialStoredValue* value = nullptr;
        for (auto& candidate : m_Values)
        {
            if (candidate.Path == path)
            {
                value = &candidate;
                break;
            }
        }
        if (value == nullptr)
            value = &m_Values.AddOne();
        value->Path = path;
        value->Kind = SLC2MaterialStoredKind::TextureAsset;
        value->Bytes.Clear();
        value->TextureId = textureId;
        value->Texture = nullptr;
        return true;
    }

    bool SLC2MaterialParameters::Remove(const String& path)
    {
        for (int32 index = 0; index < m_Values.Count(); index++)
        {
            if (m_Values[index].Path == path)
            {
                m_Values.RemoveAt(index);
                return true;
            }
        }
        return false;
    }

    const SLC2MaterialStoredValue* SLC2MaterialParameters::Find(const String& path) const
    {
        for (const auto& value : m_Values)
            if (value.Path == path)
                return &value;
        return nullptr;
    }

    void SLC2MaterialParameters::Clear() { m_Values.Clear(); }

    void SLC2MaterialRuntimeValues::Set(const SLC2MaterialRuntimeValue& value)
    {
        for (auto& candidate : m_Values)
        {
            if (candidate.Path == value.Path)
            {
                candidate = value;
                return;
            }
        }
        m_Values.Add(value);
    }

    void SLC2MaterialRuntimeValues::SetUniform(const String& path, const void* bytes, uint32 size)
    {
        SLC2MaterialRuntimeValue value;
        value.Path = path;
        value.Kind = SLC2MaterialRuntimeKind::Uniform;
        value.Bytes = bytes;
        value.ByteSize = size;
        Set(value);
    }

    void SLC2MaterialRuntimeValues::SetTexture(const String& path, GPUTextureView* texture)
    {
        SLC2MaterialRuntimeValue value;
        value.Path = path;
        value.Kind = SLC2MaterialRuntimeKind::Texture;
        value.Texture = texture;
        Set(value);
    }

    void SLC2MaterialRuntimeValues::SetBuffer(const String& path, GPUBuffer* buffer)
    {
        SLC2MaterialRuntimeValue value;
        value.Path = path;
        value.Kind = SLC2MaterialRuntimeKind::Buffer;
        value.Buffer = buffer;
        Set(value);
    }

    void SLC2MaterialRuntimeValues::SetSampler(const String& path, GPUSampler* sampler)
    {
        SLC2MaterialRuntimeValue value;
        value.Path = path;
        value.Kind = SLC2MaterialRuntimeKind::Sampler;
        value.Sampler = sampler;
        Set(value);
    }

    void SLC2MaterialRuntimeValues::SetConstantBuffer(const String& path, GPUConstantBuffer* buffer)
    {
        SLC2MaterialRuntimeValue value;
        value.Path = path;
        value.Kind = SLC2MaterialRuntimeKind::ConstantBuffer;
        value.ConstantBuffer = buffer;
        Set(value);
    }

    void SLC2MaterialRuntimeValues::SetConstantBuffer(const String& path, const void* bytes, uint32 size)
    {
        SLC2MaterialRuntimeValue value;
        value.Path = path;
        value.Kind = SLC2MaterialRuntimeKind::ConstantBufferBytes;
        value.Bytes = bytes;
        value.ByteSize = size;
        Set(value);
    }

    const SLC2MaterialRuntimeValue* SLC2MaterialRuntimeValues::Find(const String& path) const
    {
        for (const auto& value : m_Values)
            if (value.Path == path)
                return &value;
        return nullptr;
    }

    // The binding implementation only searches current-variant leaves; inactive authored paths are untouched.
    class SLC2MaterialBinding final
    {
        struct AppliedBlock
        {
            String Path;
            int32 UniformBlockIndex = -1;
        };

        static const SLC2MaterialStoredValue* FindStored(
            const String& path, const SLC2MaterialParameters& defaults,
            const List<const SLC2MaterialParameters*>& overrides)
        {
            for (const SLC2MaterialParameters* values : overrides)
                if (const auto* value = values->Find(path))
                    return value;
            return defaults.Find(path);
        }

        static bool FindSource(const String& path, const SLC2MaterialDrawContext& context,
                               const SLC2MaterialRuntimeValues*& source, bool& isMaterial)
        {
            // Match the complete first segment; a similarly prefixed name is not a system source.
            isMaterial = path.StartsWith(SE_TEXT("Material."), StringSearchCase::CaseSensitive);
            if (isMaterial) source = context.Material;
            else if (path.StartsWith(SE_TEXT("PerMaterial."), StringSearchCase::CaseSensitive)) source = context.PerMaterial;
            else if (path.StartsWith(SE_TEXT("PerView."), StringSearchCase::CaseSensitive)) source = context.PerView;
            else if (path.StartsWith(SE_TEXT("PerDraw."), StringSearchCase::CaseSensitive)) source = context.PerDraw;
            else if (path.StartsWith(SE_TEXT("SceneTexture."), StringSearchCase::CaseSensitive)) source = context.SceneTexture;
            else return false;
            return true;
        }

        static bool SetBlock(ShaderProgramInstance& instance, const String& path,
                             const SLC2MaterialRuntimeValue* runtime,
                             const SLC2MaterialStoredValue* stored)
        {
            if (runtime != nullptr)
            {
                if (runtime->Kind == SLC2MaterialRuntimeKind::ConstantBuffer)
                    return instance.SetConstantBuffer(path, runtime->ConstantBuffer);
                if (runtime->Kind == SLC2MaterialRuntimeKind::ConstantBufferBytes)
                    return instance.SetConstantBuffer(path, runtime->Bytes, runtime->ByteSize);
                return false;
            }
            return stored != nullptr && stored->Kind == SLC2MaterialStoredKind::ConstantBuffer &&
                instance.SetConstantBuffer(path, stored->Bytes.Get(), static_cast<uint32>(stored->Bytes.Count()));
        }

    public:
        static bool Prepare(GraphicShaderDrawProgram& draw, const SLC2MaterialParameters& defaults,
                            const List<const SLC2MaterialParameters*>& overrides,
                            const SLC2MaterialDrawContext& context)
        {
            ShaderProgramInstance& instance = draw.GetInstance();
            const ShaderProgramReflection& reflection = instance.GetProgram()->GetReflection();
            List<ShaderProgramBinding> bindings;
            if (!reflection.EnumerateBindings(bindings))
                return false;

            List<AppliedBlock> appliedBlocks;
            for (const ShaderProgramBinding& binding : bindings)
            {
                const SLC2MaterialRuntimeValues* source = nullptr;
                bool isMaterial = false;
                if (!FindSource(binding.Path, context, source, isMaterial))
                {
                    LOG_ERROR("Resource", "SLC2 Material has no source for active path {0}.", binding.Path);
                    return false;
                }

                // An authored or transient whole-block value fills its active fields before individual overrides.
                for (int32 cursor = 0; cursor < binding.Path.Length(); cursor++)
                {
                    if (binding.Path[cursor] != '.')
                        continue;
                    const String blockPath(*binding.Path, cursor);
                    const auto* runtimeBlock = source != nullptr ? source->Find(blockPath) : nullptr;
                    const auto* storedBlock = isMaterial && runtimeBlock == nullptr
                        ? FindStored(blockPath, defaults, overrides) : nullptr;
                    if ((runtimeBlock == nullptr && (storedBlock == nullptr ||
                         storedBlock->Kind != SLC2MaterialStoredKind::ConstantBuffer)) ||
                        (runtimeBlock != nullptr && runtimeBlock->Kind != SLC2MaterialRuntimeKind::ConstantBuffer &&
                         runtimeBlock->Kind != SLC2MaterialRuntimeKind::ConstantBufferBytes))
                        continue;
                    bool alreadyApplied = false;
                    for (const AppliedBlock& block : appliedBlocks)
                        alreadyApplied |= block.Path == blockPath;
                    if (alreadyApplied)
                        continue;
                    ShaderVarLocation location;
                    if (!ShaderNameResolver::ResolveUniformBlock(reflection, blockPath, location) ||
                        !SetBlock(instance, blockPath, runtimeBlock, storedBlock))
                        return false;
                    AppliedBlock applied;
                    applied.Path = blockPath;
                    applied.UniformBlockIndex = location.UniformBlockIndex;
                    appliedBlocks.Add(MoveTemp(applied));
                }

                const auto* runtime = source != nullptr ? source->Find(binding.Path) : nullptr;
                const auto* stored = isMaterial && runtime == nullptr
                    ? FindStored(binding.Path, defaults, overrides) : nullptr;
                if (runtime == nullptr && stored == nullptr)
                {
                    bool covered = false;
                    if (binding.Kind == ShaderProgramBindingKind::Uniform)
                        for (const AppliedBlock& block : appliedBlocks)
                            covered |= block.UniformBlockIndex == binding.Location.UniformBlockIndex &&
                                binding.Path.StartsWith(block.Path + SE_TEXT("."), StringSearchCase::CaseSensitive);
                    if (covered)
                        continue;
                    LOG_ERROR("Resource", "SLC2 Material has no active value for {0}.", binding.Path);
                    return false;
                }

                ShaderVar var(&instance, &reflection, binding.Location);
                bool written = false;
                if (binding.Kind == ShaderProgramBindingKind::Uniform)
                    written = runtime != nullptr
                        ? (runtime->Kind == SLC2MaterialRuntimeKind::Uniform &&
                           var.SetUniform(runtime->Bytes, runtime->ByteSize))
                        : (stored->Kind == SLC2MaterialStoredKind::Uniform &&
                           var.SetUniform(stored->Bytes.Get(), static_cast<uint32>(stored->Bytes.Count())));
                else if (binding.Kind == ShaderProgramBindingKind::Texture)
                {
                    GPUTextureView* view = runtime != nullptr && runtime->Kind == SLC2MaterialRuntimeKind::Texture
                        ? runtime->Texture : nullptr;
                    if (runtime == nullptr && stored->Kind == SLC2MaterialStoredKind::TextureAsset)
                    {
                        if (!stored->Texture && stored->TextureId != UID::Empty)
                            stored->Texture = stored->TextureId;
                        GPUTexture* texture = stored->Texture ? stored->Texture->GetTexture() : nullptr;
                        view = texture != nullptr ? texture->View() : nullptr;
                    }
                    written = view != nullptr && var.SetTexture(view);
                }
                else if (binding.Kind == ShaderProgramBindingKind::Buffer)
                    written = runtime != nullptr && runtime->Kind == SLC2MaterialRuntimeKind::Buffer &&
                        var.SetBuffer(runtime->Buffer);
                else if (binding.Kind == ShaderProgramBindingKind::Sampler)
                    written = runtime != nullptr && runtime->Kind == SLC2MaterialRuntimeKind::Sampler &&
                        var.SetSampler(runtime->Sampler);
                if (!written)
                {
                    LOG_ERROR("Resource", "SLC2 Material could not set active value {0}.", binding.Path);
                    return false;
                }
            }
            return true;
        }
    };

    SLC2Material::SLC2Material(const SpawnParams& params, const AssetInfo* info) : BinaryAsset(params, info) {}
    SLC2MaterialInstance::SLC2MaterialInstance(const SpawnParams& params, const AssetInfo* info) : BinaryAsset(params, info) {}

    bool SLC2Material::PrepareDraw(const String& passName, GraphicShaderGeometryMode geometryMode,
                                   const ShaderCompileTarget& target, const List<String>& defines,
                                   const SLC2MaterialDrawContext& context,
                                   std::unique_ptr<GraphicShaderDrawProgram>& output) const
    {
        List<const SLC2MaterialParameters*> overrides;
        return PrepareDrawWithChain(passName, geometryMode, target, defines, overrides, context, output);
    }

    bool SLC2Material::PrepareDrawWithChain(const String& passName, GraphicShaderGeometryMode geometryMode,
                                            const ShaderCompileTarget& target, const List<String>& defines,
                                            const List<const SLC2MaterialParameters*>& overrides,
                                            const SLC2MaterialDrawContext& context,
                                            std::unique_ptr<GraphicShaderDrawProgram>& output) const
    {
        output.reset();
        if (!m_Shader || !m_Shader->WaitForLoaded() ||
            !m_Shader->CreateDrawProgram(passName, geometryMode, target, defines, output))
            return false;
        if (!SLC2MaterialBinding::Prepare(*output, m_Parameters, overrides, context))
        {
            output.reset();
            return false;
        }
        return true;
    }

    void SLC2MaterialInstance::SetBaseMaterial(SLC2Material* material)
    {
        m_Parent = nullptr;
        m_Base = material;
    }

    bool SLC2MaterialInstance::SetParentInstance(SLC2MaterialInstance* instance)
    {
        for (auto* current = instance; current != nullptr; current = current->m_Parent.Get())
            if (current == this)
                return false;
        m_Base = nullptr;
        m_Parent = instance;
        return true;
    }

    bool SLC2MaterialInstance::PrepareDraw(const String& passName, GraphicShaderGeometryMode geometryMode,
                                           const ShaderCompileTarget& target, const List<String>& defines,
                                           const SLC2MaterialDrawContext& context,
                                           std::unique_ptr<GraphicShaderDrawProgram>& output) const
    {
        output.reset();
        List<const SLC2MaterialParameters*> overrides;
        const SLC2MaterialInstance* current = this;
        while (current != nullptr)
        {
            overrides.Add(&current->m_Overrides);
            if (current->m_Base)
            {
                if (!current->m_Base->WaitForLoaded())
                    return false;
                return current->m_Base->PrepareDrawWithChain(passName, geometryMode, target, defines,
                                                             overrides, context, output);
            }
            current = current->m_Parent.Get();
            if (current != nullptr && !current->WaitForLoaded())
                return false;
            if (overrides.Count() > 64)
                break;
        }
        LOG_ERROR("Resource", "SLC2 MaterialInstance has no valid base Material.");
        return false;
    }

    uint32 SLC2Material::GetSerializedVersion() const { return SLC2MaterialStorage::Version; }
    uint32 SLC2MaterialInstance::GetSerializedVersion() const { return SLC2MaterialStorage::Version; }
    bool SLC2Material::OnInit(AssetInitData& data) { return data.SerializedVersion == SLC2MaterialStorage::Version; }
    bool SLC2MaterialInstance::OnInit(AssetInitData& data) { return data.SerializedVersion == SLC2MaterialStorage::Version; }
    AssetChunksFlag SLC2Material::GetChunksToPreload() const { return GET_CHUNK_FLAG(SLC2MaterialStorage::Chunk); }
    AssetChunksFlag SLC2MaterialInstance::GetChunksToPreload() const { return GET_CHUNK_FLAG(SLC2MaterialStorage::Chunk); }

    Asset::LoadResult SLC2Material::load()
    {
        auto* chunk = GetChunk(SLC2MaterialStorage::Chunk);
        UID shaderId;
        uint32 referenceKind;
        SLC2MaterialParameters values;
        if (chunk == nullptr || !chunk->IsValid() ||
            !SLC2MaterialStorage::Decode(chunk->Get(), chunk->Size(), SLC2MaterialStorage::MaterialType,
                                         referenceKind, shaderId, values) || shaderId == UID::Empty)
            return LoadResult::InvalidData;
        m_Shader = shaderId;
        m_Parameters = MoveTemp(values);
        return LoadResult::Ok;
    }

    Asset::LoadResult SLC2MaterialInstance::load()
    {
        auto* chunk = GetChunk(SLC2MaterialStorage::Chunk);
        UID parentId;
        uint32 referenceKind;
        SLC2MaterialParameters values;
        if (chunk == nullptr || !chunk->IsValid() ||
            !SLC2MaterialStorage::Decode(chunk->Get(), chunk->Size(), SLC2MaterialStorage::InstanceType,
                                         referenceKind, parentId, values) || parentId == UID::Empty)
            return LoadResult::InvalidData;
        // The serialized parent kind prevents speculative loads of the wrong asset type.
        if (referenceKind == 1)
            m_Parent = AssetContent::LoadAsync<SLC2MaterialInstance>(parentId);
        else
            m_Base = AssetContent::LoadAsync<SLC2Material>(parentId);
        if (!m_Parent && !m_Base)
            return LoadResult::InvalidData;
        m_Overrides = MoveTemp(values);
        return LoadResult::Ok;
    }

    void SLC2Material::Unload(bool isReloading)
    {
        m_Shader = nullptr;
        m_Parameters.Clear();
    }

    void SLC2MaterialInstance::Unload(bool isReloading)
    {
        m_Base = nullptr;
        m_Parent = nullptr;
        m_Overrides.Clear();
    }

#if SE_EDITOR
    bool SLC2Material::SaveAssetData(const StringView& path, GraphicShader* shader,
                                     const SLC2MaterialParameters& parameters)
    {
        if (shader == nullptr)
            return false;
        List<byte> bytes;
        SLC2MaterialStorage::Encode(SLC2MaterialStorage::MaterialType, 0, shader->GetID(), parameters, bytes);
        StorageChunk chunk;
        chunk.Data.Copy(bytes.Get(), bytes.Count());
        AssetInitData data;
        data.Header.TypeID = Typeof<SLC2Material>();
        data.Header.Chunks[SLC2MaterialStorage::Chunk] = &chunk;
        data.SerializedVersion = SLC2MaterialStorage::Version;
        return SaveToAsset(path, data);
    }

    bool SLC2MaterialInstance::SaveAssetData(const StringView& path, SLC2Material* material,
                                             SLC2MaterialInstance* parent,
                                             const SLC2MaterialParameters& overrides)
    {
        if ((material == nullptr) == (parent == nullptr))
            return false;
        List<byte> bytes;
        const UID id = parent != nullptr ? parent->GetID() : material->GetID();
        SLC2MaterialStorage::Encode(SLC2MaterialStorage::InstanceType, parent != nullptr ? 1u : 0u,
                                    id, overrides, bytes);
        StorageChunk chunk;
        chunk.Data.Copy(bytes.Get(), bytes.Count());
        AssetInitData data;
        data.Header.TypeID = Typeof<SLC2MaterialInstance>();
        data.Header.Chunks[SLC2MaterialStorage::Chunk] = &chunk;
        data.SerializedVersion = SLC2MaterialStorage::Version;
        return SaveToAsset(path, data);
    }

    void SLC2Material::GetReferences(List<UID>& output) const
    {
        BinaryAsset::GetReferences(output);
        if (m_Shader)
            output.Add(m_Shader.GetID());
        for (const auto& value : m_Parameters.GetValues())
            if (value.TextureId != UID::Empty)
                output.Add(value.TextureId);
    }

    void SLC2MaterialInstance::GetReferences(List<UID>& output) const
    {
        BinaryAsset::GetReferences(output);
        if (m_Base)
            output.Add(m_Base.GetID());
        if (m_Parent)
            output.Add(m_Parent.GetID());
        for (const auto& value : m_Overrides.GetValues())
            if (value.TextureId != UID::Empty)
                output.Add(value.TextureId);
    }
#endif
} // namespace SE
