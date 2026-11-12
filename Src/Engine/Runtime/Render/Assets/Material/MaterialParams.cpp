
#include <Runtime/Render/Assets/Material/MaterialParams.h>

#include <Runtime/Core/Math/Matrix.h>
#include <Runtime/Core/Serialization/MemoryWriteStream.h>
#include <Runtime/Core/Serialization/ReadStream.h>
#include <Runtime/Graphics/GPUDevice.h>
#include <Runtime/Graphics/Shaders/ShaderProgramInstance.h>
#include <Runtime/Graphics/Textures/GPUTexture.h>
#include <Runtime/Render/Assets/Material/MaterialInfo.h>
#include <Runtime/Render/Assets/Texture/TextureBase.h>
#include <Runtime/Render/RenderBuffers.h>
#include <Runtime/Resource/AssetContent.h>
#include <Runtime/Resource/Streaming/Streaming.h>
#include <Runtime/Utilities/Variant.h>

namespace SE
{
    MaterialParameter::MaterialParameter() :
        m_Id(UID::New()), m_ParamId(UID::New()), m_IsPublic(false), m_Override(false), AsData {}
    {
    }

    MaterialParameter::~MaterialParameter()
    {
    }

    Variant MaterialParameter::GetValue() const
    {
        switch (m_Type)
        {
        case MaterialParameterType::Bool:
            return _asBool;
        case MaterialParameterType::Integer:
        case MaterialParameterType::SceneTexture:
        case MaterialParameterType::ChannelMask:
        case MaterialParameterType::TextureGroupSampler:
            return _asInteger;
        case MaterialParameterType::Float:
            return _asFloat;
        case MaterialParameterType::Vector2:
            return _asVector2;
        case MaterialParameterType::Vector3:
            return _asVector3;
        case MaterialParameterType::Vector4:
            return *(Float4*)&AsData;
        case MaterialParameterType::Color:
            return _asColor;
        case MaterialParameterType::Matrix:
            return Variant(*(Matrix*)&AsData);
        case MaterialParameterType::NormalMap:
        case MaterialParameterType::Texture:
        case MaterialParameterType::CubeTexture:
        case MaterialParameterType::GameplayGlobal:
            return m_AsAsset.Get();
        case MaterialParameterType::GPUTextureVolume:
        case MaterialParameterType::GPUTextureArray:
        case MaterialParameterType::GPUTextureCube:
        case MaterialParameterType::GPUTexture:
            return static_cast<ScriptingObject*>(m_AsGPUTexture);
        default:
            return Variant::Zero;
        }
    }

    MaterialParameterSetResult MaterialParameter::TrySetValue(const Variant& value)
    {
        switch (m_Type)
        {
        case MaterialParameterType::Bool:
            if (value.Type.Type != VariantTypes::Bool)
            {
                return MaterialParameterSetResult::InvalidValueType;
            }
            _asBool = (bool)value;
            break;
        case MaterialParameterType::Integer:
        case MaterialParameterType::SceneTexture:
        case MaterialParameterType::ChannelMask:
        case MaterialParameterType::TextureGroupSampler:
            if (value.Type.Type != VariantTypes::Int && value.Type.Type != VariantTypes::Enum)
            {
                return MaterialParameterSetResult::InvalidValueType;
            }
            _asInteger = (int32)value;
            break;
        case MaterialParameterType::Float:
            if (value.Type.Type != VariantTypes::Float)
            {
                return MaterialParameterSetResult::InvalidValueType;
            }
            _asFloat = (float)value;
            break;
        case MaterialParameterType::Vector2:
            if (value.Type.Type != VariantTypes::Float2)
            {
                return MaterialParameterSetResult::InvalidValueType;
            }
            _asVector2 = (Float2)value;
            break;
        case MaterialParameterType::Vector3:
            if (value.Type.Type != VariantTypes::Float3)
            {
                return MaterialParameterSetResult::InvalidValueType;
            }
            _asVector3 = (Float3)value;
            break;
        case MaterialParameterType::Vector4:
            if (value.Type.Type != VariantTypes::Float4 && value.Type.Type != VariantTypes::Color)
            {
                return MaterialParameterSetResult::InvalidValueType;
            }
            *(Float4*)&AsData = (Float4)value;
            break;
        case MaterialParameterType::Color:
            if (value.Type.Type != VariantTypes::Color && value.Type.Type != VariantTypes::Float4)
            {
                return MaterialParameterSetResult::InvalidValueType;
            }
            _asColor = (Color)value;
            break;
        case MaterialParameterType::Matrix:
            if (value.Type.Type != VariantTypes::Matrix)
            {
                return MaterialParameterSetResult::InvalidValueType;
            }
            *(Matrix*)&AsData = (Matrix)value;
            break;
        case MaterialParameterType::NormalMap:
        case MaterialParameterType::Texture:
        case MaterialParameterType::CubeTexture:
            switch (value.Type.Type)
            {
            case VariantTypes::Null:
                m_AsAsset = nullptr;
                break;
            /*case MaterialVariantType::Types::Guid:
                m_AsAsset = AssetContent::LoadAsync<TextureBase>(*(SGUID*)value.AsData);
                    break;*/
            case VariantTypes::Pointer:
                m_AsAsset = (TextureBase*)value.AsPointer;
                    break;
            case VariantTypes::Object:
            {
                TextureBase* texture = TypeTryCast<TextureBase>(value.AsObject);
                if (value.AsObject != nullptr && texture == nullptr)
                {
                    return MaterialParameterSetResult::InvalidValueType;
                }
                m_AsAsset = texture;
                break;
            }
            case VariantTypes::Asset:
            {
                TextureBase* texture = TypeTryCast<TextureBase>(value.AsAsset);
                if (value.AsAsset != nullptr && texture == nullptr)
                {
                    return MaterialParameterSetResult::InvalidValueType;
                }
                m_AsAsset = texture;
                break;
            }
            case VariantTypes::UID:
                m_AsAsset = AssetContent::LoadAsync<TextureBase>(*(UID*)value.AsData);
                break;
            default:
                return MaterialParameterSetResult::InvalidValueType;
            }
            break;
        case MaterialParameterType::GPUTextureVolume:
        case MaterialParameterType::GPUTextureCube:
        case MaterialParameterType::GPUTextureArray:
        case MaterialParameterType::GPUTexture:
            switch (value.Type.Type)
            {
            case VariantTypes::Null:
                m_AsGPUTexture = nullptr;
                break;
            case VariantTypes::Pointer:
                m_AsGPUTexture = (GPUTexture*)value.AsPointer;
                break;
            case VariantTypes::Object:
            {
                GPUTexture* texture = TypeTryCast<GPUTexture>(value.AsObject);
                if (value.AsObject != nullptr && texture == nullptr)
                {
                    return MaterialParameterSetResult::InvalidValueType;
                }
                m_AsGPUTexture = texture;
                break;
            }
            default:
                return MaterialParameterSetResult::InvalidValueType;
            }
            break;
        case MaterialParameterType::GameplayGlobal:
            switch (value.Type.Type)
            {
            case VariantTypes::Null:
                m_AsAsset = nullptr;
                break;
            case VariantTypes::Pointer:
                m_AsAsset = (Asset*)value.AsPointer;
                break;
            case VariantTypes::Object:
            {
                Asset* asset = TypeTryCast<Asset>(value.AsObject);
                if (value.AsObject != nullptr && asset == nullptr)
                {
                    return MaterialParameterSetResult::InvalidValueType;
                }
                m_AsAsset = asset;
                break;
            }
            case VariantTypes::Asset:
                m_AsAsset = value.AsAsset;
                break;
            case VariantTypes::UID:
                m_AsAsset = AssetContent::LoadAsync<Asset>(*(UID*)value.AsData);
                break;
            default:
                return MaterialParameterSetResult::InvalidValueType;
            }
            break;
        case MaterialParameterType::GlobalSDF:
            return MaterialParameterSetResult::UnsupportedParameterType;
        default:
            return MaterialParameterSetResult::UnsupportedParameterType;
        }

        return MaterialParameterSetResult::Success;
    }

    void MaterialParameter::SetValue(const Variant& value)
    {
        const MaterialParameterSetResult result = TrySetValue(value);
        if (result != MaterialParameterSetResult::Success)
        {
            LOG_ERROR("Render", "Cannot set material parameter value type {0} (param type: {1}, result: {2})", value.Type.ToString(), Types::GetEnumString(m_Type), static_cast<int32>(result));
        }
    }

    bool MaterialParameter::Bind(BindMeta& meta) const
    {
        if (!meta.Program || m_ShaderName.IsEmpty())
            return false;
        ShaderVar variable;
        if (!meta.Program->FindVar(m_ShaderName, variable))
            return true;
        const auto write = [&variable](const void* pData, uint32 size) {
            return variable.GetLocation().UniformByteSize == size && variable.SetUniform(pData, size);
        };
        switch (m_Type)
        {
            case MaterialParameterType::Bool: {
                const uint32 value = _asBool ? 1u : 0u;
                return write(&value, sizeof(value));
            }
            case MaterialParameterType::Integer:
                return write(&_asInteger, sizeof(_asInteger));
            case MaterialParameterType::Float:
                return write(&_asFloat, sizeof(_asFloat));
            case MaterialParameterType::Vector2:
                return write(&_asVector2, sizeof(_asVector2));
            case MaterialParameterType::Vector3:
                return write(&_asVector3, sizeof(_asVector3));
            case MaterialParameterType::Vector4:
                return write(AsData, sizeof(Float4));
            case MaterialParameterType::Color:
                return write(&_asColor, sizeof(_asColor));
            case MaterialParameterType::Matrix:
                return write(AsData, sizeof(Matrix));
            case MaterialParameterType::ChannelMask: {
                const Float4 value(_asInteger == 0, _asInteger == 1, _asInteger == 2, _asInteger == 3);
                return write(&value, sizeof(value));
            }
            case MaterialParameterType::TextureGroupSampler:
                return variable.SetSampler(Streaming::GetTextureGroupSampler(_asInteger));
            case MaterialParameterType::NormalMap:
            case MaterialParameterType::Texture:
            case MaterialParameterType::CubeTexture: {
                auto            asset   = TypeTryCast<TextureBase>(m_AsAsset.Get());
                GPUTexture*     texture = asset ? asset->GetTexture() : nullptr;
                GPUTextureView* view = texture ? texture->View() : nullptr;
                return variable.SetTexture(view);
            }
            case MaterialParameterType::GPUTexture:
            case MaterialParameterType::GPUTextureArray:
            case MaterialParameterType::GPUTextureCube:
            case MaterialParameterType::GPUTextureVolume: {
                GPUTextureView* view = nullptr;
                if (m_AsGPUTexture)
                {
                    if (m_Type == MaterialParameterType::GPUTextureVolume)
                        view = m_AsGPUTexture->ViewVolume();
                    else if (m_Type == MaterialParameterType::GPUTextureArray ||
                             m_Type == MaterialParameterType::GPUTextureCube)
                        view = m_AsGPUTexture->ViewArray();
                    else
                        view = m_AsGPUTexture->View();
                }
                return variable.SetTexture(view);
            }
            case MaterialParameterType::SceneTexture: {
                GPUTextureView* view = nullptr;
                const auto      type = static_cast<MaterialSceneTextures>(_asInteger);
                if (type == MaterialSceneTextures::SceneColor)
                    view = meta.Input;
                else if (meta.Buffers)
                {
                    GPUTexture* texture = nullptr;
                    switch (type)
                    {
                        case MaterialSceneTextures::SceneDepth:
                        case MaterialSceneTextures::WorldPosition:
                            if (meta.CanSampleDepth && meta.Buffers->DepthBuffer)
                            {
                                texture = meta.Buffers->DepthBuffer;
                                view    = EnumHasAnyFlags(texture->Flags(), GPUTextureFlags::ReadOnlyDepthView) ?
                                              texture->ViewReadOnlyDepth() :
                                              texture->View();
                            }
                            break;
                        case MaterialSceneTextures::AmbientOcclusion:
                        case MaterialSceneTextures::BaseColor:
                        case MaterialSceneTextures::DiffuseColor:
                        case MaterialSceneTextures::SpecularColor:
                            texture = meta.CanSampleGBuffer ? meta.Buffers->GBuffer0 : nullptr;
                            break;
                        case MaterialSceneTextures::WorldNormal:
                        case MaterialSceneTextures::ShadingModel:
                            texture = meta.CanSampleGBuffer ? meta.Buffers->GBuffer1 : nullptr;
                            break;
                        case MaterialSceneTextures::Roughness:
                        case MaterialSceneTextures::Metalness:
                        case MaterialSceneTextures::Specular:
                            texture = meta.CanSampleGBuffer ? meta.Buffers->GBuffer2 : nullptr;
                            break;
                        default:
                            break;
                    }
                    if (!view && texture)
                        view = texture->View();
                }
                return variable.SetTexture(view);
            }
            default:
                LOG_ERROR("Render", "Unsupported ShaderArtifact material parameter {0}.", m_Name);
                return false;
        }
    }

    bool MaterialParameter::HasContentLoaded() const
    {
        switch (m_Type)
        {
        case MaterialParameterType::NormalMap:
        case MaterialParameterType::Texture:
        case MaterialParameterType::CubeTexture:
        case MaterialParameterType::GameplayGlobal:
            return m_AsAsset == nullptr || m_AsAsset->IsLoaded();
        default:
            return true;
        }
    }

    void MaterialParameter::clone(const MaterialParameter* param)
    {
        // Clone data
        m_Type = param->m_Type;
        m_IsPublic = param->m_IsPublic;
        m_Override = param->m_Override;
        m_ShaderName = param->m_ShaderName;
        m_Name = param->m_Name;
        m_ParamId = param->m_ParamId;

        // Clone value
        switch (m_Type)
        {
        case MaterialParameterType::Bool:
            _asBool = param->_asBool;
            break;
        case MaterialParameterType::Integer:
        case MaterialParameterType::SceneTexture:
        case MaterialParameterType::ChannelMask:
        case MaterialParameterType::TextureGroupSampler:
            _asInteger = param->_asInteger;
            break;
        case MaterialParameterType::Float:
            _asFloat = param->_asFloat;
            break;
        case MaterialParameterType::Vector2:
            _asVector2 = param->_asVector2;
            break;
        case MaterialParameterType::Vector3:
            _asVector3 = param->_asVector3;
            break;
        case MaterialParameterType::Vector4:
            *(Float4*)&AsData = *(Float4*)&param->AsData;
            break;
        case MaterialParameterType::Color:
            _asColor = param->_asColor;
            break;
        case MaterialParameterType::Matrix:
            *(Matrix*)&AsData = *(Matrix*)&param->AsData;
            break;
        default:
            break;
        }
        m_AsAsset = param->m_AsAsset;
        m_AsGPUTexture = param->m_AsGPUTexture;
    }

    bool MaterialParameter::operator==(const MaterialParameter& other) const
    {
        return m_ParamId == other.m_ParamId;
    }

    String MaterialParameter::ToString() const
    {
        return String::Format(SE_TEXT("\'{0}\' ({1}:{2}:{3})"), m_Name, Types::GetEnumString(m_Type), m_ParamId, m_IsPublic);
    }

    MaterialParameter* MaterialParams::Get(const UID& id)
    {
        MaterialParameter* result = nullptr;
        for (int32 i = 0; i < Count(); i++)
        {
            if (At(i).GetParameterID() == id)
            {
                result = &At(i);
                break;
            }
        }
        return result;
    }

    const MaterialParameter* MaterialParams::Get(const UID& id) const
    {
        for (int32 i = 0; i < Count(); i++)
        {
            if (At(i).GetParameterID() == id)
            {
                return &At(i);
            }
        }
        return nullptr;
    }

    MaterialParameter* MaterialParams::Get(const StringView& name)
    {
        MaterialParameter* result = nullptr;
        for (int32 i = 0; i < Count(); i++)
        {
            if (At(i).GetName() == name)
            {
                result = &At(i);
                break;
            }
        }
        return result;
    }

    const MaterialParameter* MaterialParams::Get(const StringView& name) const
    {
        for (int32 i = 0; i < Count(); i++)
        {
            if (At(i).GetName() == name)
            {
                return &At(i);
            }
        }
        return nullptr;
    }

    int32 MaterialParams::Find(const UID& id)
    {
        int32 result = -1;
        for (int32 i = 0; i < Count(); i++)
        {
            if (At(i).GetParameterID() == id)
            {
                result = i;
                break;
            }
        }
        return result;
    }

    int32 MaterialParams::Find(const StringView& name)
    {
        int32 result = -1;
        for (int32 i = 0; i < Count(); i++)
        {
            if (At(i).GetName() == name)
            {
                result = i;
                break;
            }
        }
        return result;
    }

    int32 MaterialParams::GetVersionHash() const
    {
        return m_VersionHash;
    }

    bool MaterialParams::Bind(MaterialParamsLink* pLink, MaterialParameter::BindMeta& meta)
    {
        if (!pLink)
            return true;
        const MaterialParamsLink* base = pLink;
        while (base->Down)
            base = base->Down;
        for (const MaterialParameter& original : *base->This)
        {
            const MaterialParameter* value = &original;
            // Resolve by UID, so ordering and public display names cannot change override precedence.
            for (const MaterialParamsLink* current = pLink; current != base; current = current->Down)
            {
                const MaterialParameter* candidate = current->This->Get(original.GetParameterID());
                if (candidate && candidate->IsOverride())
                {
                    value = candidate;
                    break;
                }
            }
            if (value->GetParameterType() != original.GetParameterType() || !value->Bind(meta))
            {
                LOG_ERROR("Render", "Cannot bind ShaderArtifact material parameter {0}.", original.GetName());
                return false;
            }
        }
        return true;
    }

    void MaterialParams::Clone(MaterialParams& result)
    {
        ENGINE_ASSERT(this != &result);

        // Clone all parameters
        result.Resize(Count(), false);
        for (int32 i = 0; i < Count(); i++)
        {
            result.At(i).clone(&At(i));
        }

        result.m_VersionHash = m_VersionHash;
    }

    void MaterialParams::Dispose()
    {
        Resize(0);
        m_VersionHash = 0;
    }

    bool MaterialParams::Load(ReadStream* stream)
    {
        bool result = false;

        // Release
        Resize(0);

        // Check for not empty params
        if (stream != nullptr && stream->CanRead())
        {
            // Version
            uint16 version;
            stream->ReadUint16(&version);
            switch (version)
            {
                case 5: {
                    // Size of the collection
                    uint16 paramsCount;
                    stream->ReadUint16(&paramsCount);
                    Resize(paramsCount, false);

                    // Read all parameters
                    UID id;
                    for (int32 i = 0; i < paramsCount; i++)
                    {
                        auto param = &At(i);

                        // Read properties
                        param->m_Type = static_cast<MaterialParameterType>(stream->ReadByte());
                        stream->Read(param->m_ParamId);
                        param->m_IsPublic = stream->ReadBool();
                        param->m_Override = stream->ReadBool();
                        stream->ReadString(&param->m_Name, 10421);
                        stream->ReadString(&param->m_ShaderName, 10421);

                        // Read value
                        switch (param->m_Type)
                        {
                            case MaterialParameterType::Bool:
                                param->_asBool = stream->ReadBool();
                                break;
                            case MaterialParameterType::Integer:
                            case MaterialParameterType::SceneTexture:
                            case MaterialParameterType::ChannelMask:
                            case MaterialParameterType::TextureGroupSampler:
                                stream->ReadInt32(&param->_asInteger);
                                break;
                            case MaterialParameterType::Float:
                                stream->ReadFloat(&param->_asFloat);
                                break;
                            case MaterialParameterType::Vector2:
                                stream->Read(param->_asVector2);
                                break;
                            case MaterialParameterType::Vector3:
                                stream->Read(param->_asVector3);
                                break;
                            case MaterialParameterType::Vector4:
                                stream->Read((Float4&)param->AsData);
                                break;
                            case MaterialParameterType::Color:
                                stream->Read(param->_asColor);
                                break;
                            case MaterialParameterType::Matrix:
                                stream->Read((Matrix&)param->AsData);
                                break;
                            case MaterialParameterType::NormalMap:
                            case MaterialParameterType::Texture:
                            case MaterialParameterType::CubeTexture:
                                stream->Read(id);
                                param->m_AsAsset = AssetContent::LoadAsync<TextureBase>(id);
                                break;
                            case MaterialParameterType::GPUTextureVolume:
                            case MaterialParameterType::GPUTextureCube:
                            case MaterialParameterType::GPUTextureArray:
                            case MaterialParameterType::GPUTexture:
                                /*stream->Read(id);
                                param->m_AsGPUTexture = id;*/
                                break;
                            case MaterialParameterType::GameplayGlobal:
                                stream->Read(id);
                                param->m_AsAsset = AssetContent::LoadAsync<Asset>(id);
                                break;
                            default:
                                break;
                        }
                    }
                }
                break;

                default:
                    result = true;
                    break;
            }
        }

        UpdateHash();

        return result;
    }

    void MaterialParams::Save(WriteStream* stream)
    {
        // Version
        stream->WriteUint16(5);

        // Size of the collection
        stream->WriteUint16(Count());

        // Write all parameters
        for (int32 i = 0; i < Count(); i++)
        {
            // Cache
            auto param = &At(i);

            // Write properties
            stream->WriteByte(static_cast<byte>(param->m_Type));
            stream->Write(param->m_ParamId);
            stream->WriteBool(param->m_IsPublic);
            stream->WriteBool(param->m_Override);
            stream->WriteString(param->m_Name, 10421);
            stream->WriteString(param->m_ShaderName, 10421);

            // Write value
            UID id;
            switch (param->m_Type)
            {
            case MaterialParameterType::Bool:
                stream->WriteBool(param->_asBool);
                break;
            case MaterialParameterType::Integer:
            case MaterialParameterType::SceneTexture:
            case MaterialParameterType::ChannelMask:
            case MaterialParameterType::TextureGroupSampler:
                stream->WriteInt32(param->_asInteger);
                break;
            case MaterialParameterType::Float:
                stream->WriteFloat(param->_asFloat);
                break;
            case MaterialParameterType::Vector2:
                stream->Write(param->_asVector2);
                break;
            case MaterialParameterType::Vector3:
                stream->Write(param->_asVector3);
                break;
            case MaterialParameterType::Vector4:
                stream->Write((Float4&)param->AsData);
                break;
            case MaterialParameterType::Color:
                stream->Write(param->_asColor);
                break;
            case MaterialParameterType::Matrix:
                stream->Write((Matrix&)param->AsData);
                break;
            case MaterialParameterType::NormalMap:
            case MaterialParameterType::Texture:
            case MaterialParameterType::CubeTexture:
            case MaterialParameterType::GameplayGlobal:
                id = param->m_AsAsset.GetID();
                stream->Write(id);
                break;
            case MaterialParameterType::GPUTextureVolume:
            case MaterialParameterType::GPUTextureArray:
            case MaterialParameterType::GPUTextureCube:
            case MaterialParameterType::GPUTexture:
                /*id = param->m_AsGPUTexture.GetID();
                stream->Write(id);*/
                break;
            default:
                break;
            }
        }
    }

    void MaterialParams::Save(WriteStream* stream, const List<SerializedMaterialParam>* params)
    {
        // Version
        stream->WriteUint16(5);

        // Size of the collection
        stream->WriteUint16(params ? params->Count() : 0);

        // Write all parameters
        if (params)
        {
            for (int32 i = 0; i < params->Count(); i++)
            {
                // Cache
                const SerializedMaterialParam& param = params->At(i);

                // Write properties
                stream->WriteByte(static_cast<byte>(param.Type));
                stream->Write(param.ID);
                stream->WriteBool(param.IsPublic);
                stream->WriteBool(param.Override);
                stream->WriteString(param.Name, 10421);
                const bool uniform =
                    param.Type == MaterialParameterType::Bool || param.Type == MaterialParameterType::Integer ||
                    param.Type == MaterialParameterType::Float || param.Type == MaterialParameterType::Vector2 ||
                    param.Type == MaterialParameterType::Vector3 || param.Type == MaterialParameterType::Vector4 ||
                    param.Type == MaterialParameterType::Color || param.Type == MaterialParameterType::Matrix ||
                    param.Type == MaterialParameterType::ChannelMask;
                stream->WriteString(uniform ? SE_TEXT("Data.") + param.ShaderName : param.ShaderName, 10421);

                // Write value
                switch (param.Type)
                {
                case MaterialParameterType::Bool:
                    stream->WriteBool(param.AsBool);
                    break;
                case MaterialParameterType::SceneTexture:
                case MaterialParameterType::ChannelMask:
                case MaterialParameterType::Integer:
                case MaterialParameterType::TextureGroupSampler:
                    stream->WriteInt32(param.AsInteger);
                    break;
                case MaterialParameterType::Float:
                    stream->WriteFloat(param.AsFloat);
                    break;
                case MaterialParameterType::Vector2:
                    stream->Write(param.AsFloat2);
                    break;
                case MaterialParameterType::Vector3:
                    stream->Write(param.AsFloat3);
                    break;
                case MaterialParameterType::Vector4:
                    stream->Write((Float4&)param.AsData);
                    break;
                case MaterialParameterType::Color:
                    stream->Write(param.AsColor);
                    break;
                case MaterialParameterType::Matrix:
                    stream->Write((Matrix&)param.AsData);
                    break;
                case MaterialParameterType::NormalMap:
                case MaterialParameterType::Texture:
                case MaterialParameterType::CubeTexture:
                case MaterialParameterType::GameplayGlobal:
                    stream->Write(param.AsGuid);
                    break;
                case MaterialParameterType::GPUTextureVolume:
                case MaterialParameterType::GPUTextureCube:
                case MaterialParameterType::GPUTextureArray:
                case MaterialParameterType::GPUTexture:
                    // 代码创建的 GPUTexture 仅为运行时引用，不进入 Material 资产数据。
                    break;
                default:
                    break;
                }
            }
        }
    }

    void MaterialParams::Save(BytesContainer& data, const List<SerializedMaterialParam>* params)
    {
        MemoryWriteStream stream(1024);
        Save(&stream, params);
        if (stream.GetPosition() > 0)
        {
            data.Copy(stream.GetHandle(), stream.GetPosition());
        }
        else
        {
            data.Release();
        }
    }

#if SE_EDITOR

    void MaterialParams::GetReferences(List<UID>& output) const
    {
        for (int32 i = 0; i < Count(); i++)
        {
            if (At(i).m_AsAsset)
            {
                output.Add(At(i).m_AsAsset->GetID());
            }
        }
    }

#endif

    bool MaterialParams::HasContentLoaded() const
    {
        bool result = true;

        for (int32 i = 0; i < Count(); i++)
        {
            if (!At(i).HasContentLoaded())
            {
                result = false;
                break;
            }
        }

        return result;
    }

    void MaterialParams::UpdateHash()
    {
        m_VersionHash = rand();
    }
} // SE
