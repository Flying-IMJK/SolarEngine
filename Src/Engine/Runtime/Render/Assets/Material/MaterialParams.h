#pragma once
#include <Runtime/Core/Math/Color.h>
#include <Runtime/Core/Math/Vector2.h>
#include <Runtime/Core/Math/Vector3.h>
#include <Runtime/Core/Types/Collections/DataContainer.h>
#include <Runtime/Core/Types/Object.h>
#include <Runtime/Core/Types/Strings/String.h>
#include <Runtime/Core/Types/UID.h>
#include <Runtime/Core/Types/Variable.h>

#include <Runtime/API.h>
#include <Runtime/Resource/AssetRef.h>
#include <Runtime/Resource/Storage/StorageReference.h>

namespace SE
{

    class GPUTexture;
    class RenderBuffers;
    class GPUTextureView;
    class ShaderProgramInstance;
    class WriteStream;
    class ReadStream;
    class MaterialParams;
    class MaterialInstance;

    struct MaterialParamsLink
    {
        MaterialParams* This;
        MaterialParamsLink* Up;
        MaterialParamsLink* Down;
    };

    /// <summary>
    /// The material parameter types.
    /// </summary>
    enum class MaterialParameterType : byte
    {
        /// <summary>
        /// The invalid type.
        /// </summary>
        Invalid = 0,

        /// <summary>
        /// The bool.
        /// </summary>
        Bool = 1,

        /// <summary>
        /// The integer.
        /// </summary>
        Integer = 2,

        /// <summary>
        /// The float.
        /// </summary>
        Float = 3,

        /// <summary>
        /// The vector2
        /// </summary>
        Vector2 = 4,

        /// <summary>
        /// The vector3.
        /// </summary>
        Vector3 = 5,

        /// <summary>
        /// The vector4.
        /// </summary>
        Vector4 = 6,

        /// <summary>
        /// The color.
        /// </summary>
        Color = 7,

        /// <summary>
        /// The texture.
        /// </summary>
        Texture = 8,

        /// <summary>
        /// The cube texture.
        /// </summary>
        CubeTexture = 9,

        /// <summary>
        /// The normal map texture.
        /// </summary>
        NormalMap = 10,

        /// <summary>
        /// The scene texture.
        /// </summary>
        SceneTexture = 11,

        /// <summary>
        /// The GPU texture (created from code).
        /// </summary>
        GPUTexture = 12,

        /// <summary>
        /// The matrix.
        /// </summary>
        Matrix = 13,

        /// <summary>
        /// The GPU texture array (created from code).
        /// </summary>
        GPUTextureArray = 14,

        /// <summary>
        /// The GPU volume texture (created from code).
        /// </summary>
        GPUTextureVolume = 15,

        /// <summary>
        /// The GPU cube texture (created from code).
        /// </summary>
        GPUTextureCube = 16,

        /// <summary>
        /// The RGBA channel selection mask.
        /// </summary>
        ChannelMask = 17,

        /// <summary>
        /// The gameplay global.
        /// </summary>
        GameplayGlobal = 18,

        /// <summary>
        /// The texture sampler derived from texture group settings.
        /// </summary>
        TextureGroupSampler = 19,

        /// <summary>
        /// The Global SDF (textures and constants).
        /// </summary>
        GlobalSDF = 20,
    };

    enum class MaterialParameterSetResult : byte
    {
        Success,
        InvalidValueType,
        UnsupportedParameterType,
    };

    /// <summary>
    /// Structure of serialized material parameter data.
    /// </summary>
    struct SerializedMaterialParam
    {
        MaterialParameterType Type;
        UID ID;
        bool IsPublic;
        bool Override;
        String Name;
        String ShaderName;

        union
        {
            bool AsBool;
            int32 AsInteger;
            float AsFloat;
            Float2 AsFloat2;
            Float3 AsFloat3;
            Color AsColor;
            UID AsGuid;
            byte AsData[16 * 4];
        };

        SerializedMaterialParam() : Type(), IsPublic(false), Override(false), AsData {} {}
    };

    /// <summary>
    /// Material variable object. Allows to modify material parameter value at runtime.
    /// </summary>
    class SE_API_RUNTIME MaterialParameter : public Object
    {
        friend MaterialParams;
        friend MaterialInstance;
    private:
        UID m_Id;

        UID m_ParamId;
        MaterialParameterType m_Type = MaterialParameterType::Invalid;
        bool m_IsPublic;
        bool m_Override;
        String                m_ShaderName;

        union
        {
            bool _asBool;
            int32 _asInteger;
            float _asFloat;
            Float2 _asVector2;
            Float3 _asVector3;
            Color _asColor;
            byte AsData[16 * 4];
        };

        AssetRef<Asset> m_AsAsset;
        // 代码创建的 GPUTexture 只在运行时使用，MaterialParameter 不参与其生命周期管理。
        GPUTexture* m_AsGPUTexture = nullptr;
        String m_Name;

    public:
        MaterialParameter();
        ~MaterialParameter() override;

        MaterialParameter(const MaterialParameter& other)
            : MaterialParameter()
        {
#if !BUILD_RELEASE
            ENGINE_UNREACHABLE_CODE(); // Not used
#endif
        }


        MaterialParameter& operator=(const MaterialParameter& other)
        {
#if !BUILD_RELEASE
            ENGINE_UNREACHABLE_CODE(); // Not used
#endif
            return *this;
        }

    public:
        FORCE_INLINE UID GetID() const
        {
            return m_Id;
        }

        /// <summary>
        /// Gets the parameter ID (not the parameter instance Id but the original parameter ID).
        /// </summary>
        FORCE_INLINE UID GetParameterID() const
        {
            return m_ParamId;
        }

        /// <summary>
        /// Gets the parameter type.
        /// </summary>
        FORCE_INLINE MaterialParameterType GetParameterType() const
        {
            return m_Type;
        }

        /// <summary>
        /// Gets the parameter name.
        /// </summary>
        FORCE_INLINE const String& GetName() const
        {
            return m_Name;
        }

        /// <summary>
        /// Returns true is parameter is public visible.
        /// </summary>
        FORCE_INLINE bool IsPublic() const
        {
            return m_IsPublic;
        }

        /// <summary>
        /// Returns true is parameter is overriding the value.
        /// </summary>
        FORCE_INLINE bool IsOverride() const
        {
            return m_Override;
        }

        /// <summary>
        /// Sets the value override mode.
        /// </summary>
        void SetIsOverride(bool value)
        {
            m_Override = value;
        }

        /// The stable reflected path generated for this parameter, independent of its display name.
        const String& GetShaderName() const { return m_ShaderName; }

    public:
        /// <summary>
        /// Gets the value of the parameter.
        /// </summary>
        /// <returns>The value.</returns>
        Variant GetValue() const;

        FORCE_INLINE Asset* GetAssetValue() const
        {
            return m_AsAsset.Get();
        }

        FORCE_INLINE GPUTexture* GetGPUTextureValue() const
        {
            return m_AsGPUTexture;
        }

        /// <summary>
        /// Tries to set the parameter value and reports a deterministic failure reason.
        /// </summary>
        MaterialParameterSetResult TrySetValue(const Variant& value);

        /// <summary>
        /// Sets the value of the parameter. Kept as the compatibility logging wrapper.
        /// </summary>
        /// <param name="value">The value.</param>
        void SetValue(const Variant& value);

    public:
        /// <summary>
        /// The material parameter binding metadata.
        /// </summary>
        struct BindMeta
        {
            ShaderProgramInstance* Program          = nullptr;
            GPUTextureView*        Input            = nullptr;
            const RenderBuffers*   Buffers          = nullptr;
            bool                   CanSampleDepth   = false;
            bool                   CanSampleGBuffer = false;
        };

        // Missing variables are valid for optimized Program variants. Present variables must match the value ABI.
        bool Bind(BindMeta& meta) const;

        bool HasContentLoaded() const;

        bool operator==(const MaterialParameter& other) const;

        // [Object]
        String ToString() const override;

    private:
        void clone(const MaterialParameter* param);

    };

    /// <summary>
    /// The collection of material parameters.
    /// </summary>
    class SE_API_RUNTIME MaterialParams : public List<MaterialParameter>
    {
        friend MaterialInstance;
    private:
        int32 m_VersionHash = 0;

    public:
        MaterialParameter* Get(const UID& id);
        const MaterialParameter* Get(const UID& id) const;
        MaterialParameter* Get(const StringView& name);
        const MaterialParameter* Get(const StringView& name) const;
        int32 Find(const UID& id);
        int32 Find(const StringView& name);

    public:
        /// <summary>
        /// Gets the parameters version hash. Every time the parameters are modified (loaded, edited, etc.) the hash changes. Can be used to sync instanced parameters collection.
        /// </summary>
        int32 GetVersionHash() const;

    public:
        /// <summary>
        /// Binds the parameters to the pipeline.
        /// </summary>
        /// <param name="link">The parameters binding link. Used to support per-parameter override.</param>
        /// <param name="meta">The bind meta.</param>
        static bool Bind(MaterialParamsLink* pLink, MaterialParameter::BindMeta& meta);

        /// <summary>
        /// Clones the parameters list.
        /// </summary>
        /// <param name="result">The result container.</param>
        void Clone(MaterialParams& result);

        /// <summary>
        /// Releases the whole data.
        /// </summary>
        void Dispose();

        /// <summary>
        /// Loads material parameters from the stream.
        /// </summary>
        /// <param name="stream">The stream with data.</param>
        /// <returns>True if cannot load parameters for the file.</returns>
        bool Load(ReadStream* stream);

        /// <summary>
        /// Saves material parameters to the stream.
        /// </summary>
        /// <param name="stream">The stream with data.</param>
        void Save(WriteStream* stream);

        /// <summary>
        /// Saves the material parameters to the stream.
        /// </summary>
        /// <param name="stream">The stream with data.</param>
        /// <param name="params">The array of parameters.</param>
        static void Save(WriteStream* stream, const List<SerializedMaterialParam>* params);

        /// <summary>
        /// Saves the material parameters to the bytes container.
        /// </summary>
        /// <param name="data">The output data.</param>
        /// <param name="params">The array of parameters.</param>
        static void Save(BytesContainer& data, const List<SerializedMaterialParam>* params);

#if SE_EDITOR

        /// <summary>
        /// Gets the asset references (see Asset.GetReferences for more info).
        /// </summary>
        /// <param name="output">The output.</param>
        void GetReferences(List<UID>& output) const;
#endif

        bool HasContentLoaded() const;

    private:
        void UpdateHash();
    };

} // SE

