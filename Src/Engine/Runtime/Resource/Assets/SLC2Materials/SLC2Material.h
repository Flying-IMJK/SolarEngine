#pragma once

#include "GraphicShader.h"
#include "Runtime/Resource/AssetRef.h"

namespace SE
{
    class GPUBuffer;
    class GPUConstantBuffer;
    class GPUSampler;
    class GPUTextureView;
    class TextureBase;
    class SLC2MaterialInstance;

    enum class SLC2MaterialStoredKind : byte { Uniform, ConstantBuffer, TextureAsset };

    // Editor-authored values are separate from compiled code and never contain GPU pointers.
    struct SE_API_RUNTIME SLC2MaterialStoredValue
    {
        String Path;
        SLC2MaterialStoredKind Kind = SLC2MaterialStoredKind::Uniform;
        List<byte> Bytes;
        UID TextureId = UID::Empty;
        mutable AssetRef<TextureBase> Texture;
    };

    class SE_API_RUNTIME SLC2MaterialParameters
    {
    public:
        bool SetUniform(const String& path, const void* bytes, uint32 size);
        bool SetConstantBuffer(const String& path, const void* bytes, uint32 size);
        bool SetTexture(const String& path, TextureBase* texture);
        bool SetTextureReference(const String& path, const UID& textureId);
        bool Remove(const String& path);
        const SLC2MaterialStoredValue* Find(const String& path) const;
        const List<SLC2MaterialStoredValue>& GetValues() const { return m_Values; }
        void Clear();
    private:
        bool SetBytes(const String& path, SLC2MaterialStoredKind kind, const void* bytes, uint32 size);
        List<SLC2MaterialStoredValue> m_Values;
    };

    enum class SLC2MaterialRuntimeKind : byte { Uniform, Texture, Buffer, Sampler, ConstantBuffer, ConstantBufferBytes };

    struct SE_API_RUNTIME SLC2MaterialRuntimeValue
    {
        String Path;
        SLC2MaterialRuntimeKind Kind = SLC2MaterialRuntimeKind::Texture;
        GPUTextureView* Texture = nullptr;
        GPUBuffer* Buffer = nullptr;
        GPUSampler* Sampler = nullptr;
        GPUConstantBuffer* ConstantBuffer = nullptr;
        const void* Bytes = nullptr;
        uint32 ByteSize = 0;
    };

    // Ephemeral one-Draw input; referenced resources and bytes survive through PrepareDraw only.
    class SE_API_RUNTIME SLC2MaterialRuntimeValues
    {
    public:
        void SetUniform(const String& path, const void* bytes, uint32 size);
        void SetTexture(const String& path, GPUTextureView* value);
        void SetBuffer(const String& path, GPUBuffer* value);
        void SetSampler(const String& path, GPUSampler* value);
        void SetConstantBuffer(const String& path, GPUConstantBuffer* value);
        void SetConstantBuffer(const String& path, const void* bytes, uint32 size);
        const SLC2MaterialRuntimeValue* Find(const String& path) const;
        const List<SLC2MaterialRuntimeValue>& GetValues() const { return m_Values; }
    private:
        void Set(const SLC2MaterialRuntimeValue& value);
        List<SLC2MaterialRuntimeValue> m_Values;
    };

    // Sources belong to the caller's current Draw; this view owns no values or GPU resources.
    struct SE_API_RUNTIME SLC2MaterialDrawContext
    {
        const SLC2MaterialRuntimeValues* Material = nullptr;
        const SLC2MaterialRuntimeValues* PerMaterial = nullptr;
        const SLC2MaterialRuntimeValues* PerView = nullptr;
        const SLC2MaterialRuntimeValues* PerDraw = nullptr;
        const SLC2MaterialRuntimeValues* SceneTexture = nullptr;
    };

    SE_CLASS(Reflect, API(NoSpawn))
    class SE_API_RUNTIME SLC2Material final : public BinaryAsset
    {
        SE_DEFINE_CLASS(SLC2Material, BinaryAsset)
        ASSET_HEADER(SLC2Material);
    public:
        SLC2MaterialParameters& GetParameters() { return m_Parameters; }
        const SLC2MaterialParameters& GetParameters() const { return m_Parameters; }
        void SetGraphicShader(GraphicShader* shader) { m_Shader = shader; }
        GraphicShader* GetGraphicShader() const { return m_Shader.Get(); }
        bool PrepareDraw(const String& passName, GraphicShaderGeometryMode geometryMode,
                         const ShaderCompileTarget& target, const List<String>& defines,
                         const SLC2MaterialDrawContext& context,
                         std::unique_ptr<GraphicShaderDrawProgram>& output) const;
#if SE_EDITOR
        static bool SaveAssetData(const StringView& path, GraphicShader* shader,
                                  const SLC2MaterialParameters& parameters);
        void GetReferences(List<UID>& output) const override;
#endif
        uint32 GetSerializedVersion() const override;
    protected:
        bool OnInit(AssetInitData& initData) override;
        AssetChunksFlag GetChunksToPreload() const override;
        LoadResult load() override;
        void Unload(bool isReloading) override;
    private:
        friend class SLC2MaterialInstance;
        bool PrepareDrawWithChain(const String& passName, GraphicShaderGeometryMode geometryMode,
                                  const ShaderCompileTarget& target, const List<String>& defines,
                                  const List<const SLC2MaterialParameters*>& overrides,
                                  const SLC2MaterialDrawContext& context,
                                  std::unique_ptr<GraphicShaderDrawProgram>& output) const;
        AssetRef<GraphicShader> m_Shader;
        SLC2MaterialParameters m_Parameters;
    };

    SE_CLASS(Reflect, API(NoSpawn))
    class SE_API_RUNTIME SLC2MaterialInstance final : public BinaryAsset
    {
        SE_DEFINE_CLASS(SLC2MaterialInstance, BinaryAsset)
        ASSET_HEADER(SLC2MaterialInstance);
    public:
        SLC2MaterialParameters& GetOverrides() { return m_Overrides; }
        const SLC2MaterialParameters& GetOverrides() const { return m_Overrides; }
        void SetBaseMaterial(SLC2Material* material);
        bool SetParentInstance(SLC2MaterialInstance* instance);
        bool PrepareDraw(const String& passName, GraphicShaderGeometryMode geometryMode,
                         const ShaderCompileTarget& target, const List<String>& defines,
                         const SLC2MaterialDrawContext& context,
                         std::unique_ptr<GraphicShaderDrawProgram>& output) const;
#if SE_EDITOR
        static bool SaveAssetData(const StringView& path, SLC2Material* material,
                                  SLC2MaterialInstance* parent,
                                  const SLC2MaterialParameters& overrides);
        void GetReferences(List<UID>& output) const override;
#endif
        uint32 GetSerializedVersion() const override;
    protected:
        bool OnInit(AssetInitData& initData) override;
        AssetChunksFlag GetChunksToPreload() const override;
        LoadResult load() override;
        void Unload(bool isReloading) override;
    private:
        AssetRef<SLC2Material> m_Base;
        AssetRef<SLC2MaterialInstance> m_Parent;
        SLC2MaterialParameters m_Overrides;
    };
} // namespace SE
