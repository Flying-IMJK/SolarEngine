
#include "MaterialParams.h"

#include "../../../Utilities/Variant.h"
#include "Runtime/Core/Math/Matrix.h"
#include "Runtime/Core/Serialization/MemoryWriteStream.h"
#include "Runtime/Core/Serialization/ReadStream.h"
#include "Runtime/Graphics/GPUContext.h"
#include "Runtime/Graphics/GPUDevice.h"
#include "Runtime/Graphics/Textures/GPUTexture.h"
#include "Runtime/Render/Assets/Texture/TextureBase.h"
#include "Runtime/Resource/AssetContent.h"
#include "Runtime/Resource/Streaming/Streaming.h"

namespace SE
{
    MaterialParameter::MaterialParameter():
        m_Id(UID::New()),
        m_IsPublic(false), m_Override(false), m_RegisterIndex(0),
        m_Offset(0), _asBool(false), _asInteger(0), _asFloat(0), _asVector2(),
        _asVector3(), _asColor(), AsData{}
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
            // return m_AsGPUTexture.Get();
        default:
            return Variant::Zero;
        }
    }

    void MaterialParameter::SetValue(const Variant& value)
    {
        bool invalidType = false;
        switch (m_Type)
        {
        case MaterialParameterType::Bool:
            _asBool = (bool)value;
            break;
        case MaterialParameterType::Integer:
        case MaterialParameterType::SceneTexture:
        case MaterialParameterType::ChannelMask:
        case MaterialParameterType::TextureGroupSampler:
            _asInteger = (int32)value;
            break;
        case MaterialParameterType::Float:
            _asFloat = (float)value;
            break;
        case MaterialParameterType::Vector2:
            _asVector2 = (Float2)value;
            break;
        case MaterialParameterType::Vector3:
            _asVector3 = (Float3)value;
            break;
        case MaterialParameterType::Vector4:
            *(Float4*)&AsData = (Float4)value;
            break;
        case MaterialParameterType::Color:
            _asColor = (Color)value;
            break;
        case MaterialParameterType::Matrix:
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
                m_AsAsset = TypeCast<TextureBase>(value.AsObject);
                    break;
            case VariantTypes::Asset:
                m_AsAsset = TypeCast<TextureBase>(value.AsAsset);
                    break;
            default:
                invalidType = true;
            }
            break;
        case MaterialParameterType::GPUTextureVolume:
        case MaterialParameterType::GPUTextureCube:
        case MaterialParameterType::GPUTextureArray:
        case MaterialParameterType::GPUTexture:
            /*switch (value.Type.Type)
            {
            case MaterialVariantType::Types::Null:
                m_AsGPUTexture = nullptr;
                    break;
            case MaterialVariantType::Types::Pointer:
                m_AsGPUTexture = (GPUTexture*)value.AsPointer;
                    break;
            case MaterialVariantType::Types::Object:
                m_AsGPUTexture = Cast<GPUTexture>(value.AsObject);
                    break;
            default:
                invalidType = true;
            }*/
            break;
        /*case MaterialParameterType::GameplayGlobal:
            switch (value.Type.Type)
            {
            case MaterialVariantType::Types::Null:
                m_AsAsset = nullptr;
                    break;
            case MaterialVariantType::Types::Guid:
                m_AsAsset = AssetContent::LoadAsync<GameplayGlobals>(*(SGUID*)value.AsData);
                    break;
            case MaterialVariantType::Types::Pointer:
                m_AsAsset = (GameplayGlobals*)value.AsPointer;
                    break;
            case MaterialVariantType::Types::Object:
                m_AsAsset = Cast<GameplayGlobals>(value.AsObject);
                    break;
            case MaterialVariantType::Types::Asset:
                m_AsAsset = Cast<GameplayGlobals>(value.AsAsset);
                    break;
        default:
            invalidType = true;
            }
            break;*/
        case MaterialParameterType::GlobalSDF:
            break;
        default:
            invalidType = true;
        }

        if (invalidType)
        {
            LOG_ERROR("Render", "Invalid material parameter value type {0} to set (param type: {1})", value.Type.ToString(), Types::GetEnumString(m_Type));
        }
    }

    void MaterialParameter::Bind(BindMeta& meta) const
    {
        switch (m_Type)
        {
        case MaterialParameterType::Bool:
            ENGINE_ASSERT(meta.Constants.Get() && meta.Constants.Length() >= static_cast<int32>(m_Offset) + sizeof(bool));
            *((int32*)(meta.Constants.Get() + m_Offset)) = _asBool;
            break;
        case MaterialParameterType::Integer:
            ENGINE_ASSERT(meta.Constants.Get() && meta.Constants.Length() >= static_cast<int32>(m_Offset) + sizeof(int32));
            *((int32*)(meta.Constants.Get() + m_Offset)) = _asInteger;
            break;
        case MaterialParameterType::Float:
            ENGINE_ASSERT(meta.Constants.Get() && meta.Constants.Length() >= static_cast<int32>(m_Offset) + sizeof(float));
            *((float*)(meta.Constants.Get() + m_Offset)) = _asFloat;
            break;
        case MaterialParameterType::Vector2:
            ENGINE_ASSERT(meta.Constants.Get() && meta.Constants.Length() >= static_cast<int32>(m_Offset) + sizeof(Float2));
            *((Float2*)(meta.Constants.Get() + m_Offset)) = _asVector2;
            break;
        case MaterialParameterType::Vector3:
            ENGINE_ASSERT(meta.Constants.Get() && meta.Constants.Length() >= static_cast<int32>(m_Offset) + sizeof(Float3));
            *((Float3*)(meta.Constants.Get() + m_Offset)) = _asVector3;
            break;
        case MaterialParameterType::Vector4:
            ENGINE_ASSERT(meta.Constants.Get() && meta.Constants.Length() >= static_cast<int32>(m_Offset) + sizeof(Float4));
            *((Float4*)(meta.Constants.Get() + m_Offset)) = *(Float4*)&AsData;
            break;
        case MaterialParameterType::Color:
            ENGINE_ASSERT(meta.Constants.Get() && meta.Constants.Length() >= static_cast<int32>(m_Offset) + sizeof(Float4));
            *((Color*)(meta.Constants.Get() + m_Offset)) = _asColor;
            break;
        case MaterialParameterType::Matrix:
            ENGINE_ASSERT(meta.Constants.Get() && meta.Constants.Length() >= (int32)m_Offset + sizeof(Matrix));
            Matrix::Transpose(*(Matrix*)&AsData, *(Matrix*)(meta.Constants.Get() + m_Offset));
            break;
        case MaterialParameterType::NormalMap:
        {
            // If normal map texture is set but not loaded yet, use default engine normal map (reduces loading artifacts)
            GPUTexture* texture = m_AsAsset ? DynamicCast<TextureBase> (DynamicCast<TextureBase>(m_AsAsset.Get()))->GetTexture() : nullptr;
            if (texture && texture->ResidentMipLevels() <= 0)
            {
                // texture = GPUDevice::instance->GetDefaultNormalMap();
            }
            GPUTextureView* textureView = GET_TEXTURE_VIEW_SAFE(texture);
            meta.Context->BindSR(m_RegisterIndex, textureView);
            break;
        }
        case MaterialParameterType::Texture:
        case MaterialParameterType::CubeTexture:
        {
            const auto texture = m_AsAsset ? DynamicCast<TextureBase>(m_AsAsset.Get())->GetTexture() : nullptr;
            const auto view = GET_TEXTURE_VIEW_SAFE(texture);
            meta.Context->BindSR(m_RegisterIndex, view);
            break;
        }
        case MaterialParameterType::GPUTexture:
        {
            /*const auto texture = m_AsGPUTexture.Get();
            const auto view = GET_TEXTURE_VIEW_SAFE(texture);
            meta.Context->BindSR(m_RegisterIndex, view);*/
            break;
        }
        case MaterialParameterType::GPUTextureArray:
        case MaterialParameterType::GPUTextureCube:
        {
            /*const auto view = m_AsGPUTexture ? m_AsGPUTexture->ViewArray() : nullptr;
            meta.Context->BindSR(m_RegisterIndex, view);*/
            break;
        }
        case MaterialParameterType::GPUTextureVolume:
        {
            /*const auto view = m_AsGPUTexture ? m_AsGPUTexture->ViewVolume() : nullptr;
            meta.Context->BindSR(m_RegisterIndex, view);*/
            break;
        }
        /*case MaterialParameterType::SceneTexture:
        {
            GPUTextureView* view = nullptr;
            const auto type = (MaterialSceneTextures)_asInteger;
            if (type == MaterialSceneTextures::SceneColor)
            {
                view = meta.Input;
            }
            else if (meta.Buffers)
            {
                switch (type)
                {
                case MaterialSceneTextures::SceneDepth:
                case MaterialSceneTextures::WorldPosition:
                    view = meta.CanSampleDepth
                               ? EnumHasAnyFlags(meta.Buffers->DepthBuffer->Flags(), GPUTextureFlags::ReadOnlyDepthView)
                                     ? meta.Buffers->DepthBuffer->ViewReadOnlyDepth()
                                     : meta.Buffers->DepthBuffer->View()
                               : GPUDevice::Instance->GetDefaultWhiteTexture()->View();
                    break;
                case MaterialSceneTextures::AmbientOcclusion:
                case MaterialSceneTextures::BaseColor:
                case MaterialSceneTextures::DiffuseColor:
                case MaterialSceneTextures::SpecularColor:
                    view = meta.CanSampleGBuffer ? meta.Buffers->GBuffer0->View() : nullptr;
                    break;
                case MaterialSceneTextures::WorldNormal:
                case MaterialSceneTextures::ShadingModel:
                    view = meta.CanSampleGBuffer ? meta.Buffers->GBuffer1->View() : nullptr;
                    break;
                case MaterialSceneTextures::Roughness:
                case MaterialSceneTextures::Metalness:
                case MaterialSceneTextures::Specular:
                    view = meta.CanSampleGBuffer ? meta.Buffers->GBuffer2->View() : nullptr;
                    break;
                default: ;
                }
            }
            else if (type == MaterialSceneTextures::SceneDepth)
            {
                view = GPUDevice::Instance->GetDefaultWhiteTexture()->View();
            }
            meta.Context->BindSR(m_RegisterIndex, view);
            break;
        }*/
        case MaterialParameterType::ChannelMask:
            ENGINE_ASSERT(meta.Constants.Get() && meta.Constants.Length() >= (int32)m_Offset + sizeof(int32));
            *((Float4*)(meta.Constants.Get() + m_Offset)) = Float4(_asInteger == 0, _asInteger == 1, _asInteger == 2, _asInteger == 3);
            break;
        /*case MaterialParameterType::GameplayGlobal:
            if (m_AsAsset)
            {
                const auto e = m_AsAsset.As<GameplayGlobals>()->Variables.TryGet(m_Name);
                if (e)
                {
                    switch (e->Value.Type.Type)
                    {
                    case MaterialVariantType::Types::Bool:
                        ENGINE_ASSERT(meta.Constants.Get() && meta.Constants.Length() >= static_cast<int32>(m_Offset) + sizeof(bool));
                        *((bool*)(meta.Constants.Get() + m_Offset)) = e->Value.AsBool;
                        break;
                    case MaterialVariantType::Types::Int:
                        ENGINE_ASSERT(meta.Constants.Get() && meta.Constants.Length() >= static_cast<int32>(m_Offset) + sizeof(int32));
                        *((int32*)(meta.Constants.Get() + m_Offset)) = e->Value.AsInt;
                        break;
                    case MaterialVariantType::Types::Uint:
                        ENGINE_ASSERT(meta.Constants.Get() && meta.Constants.Length() >= static_cast<int32>(m_Offset) + sizeof(uint32));
                        *((uint32*)(meta.Constants.Get() + m_Offset)) = e->Value.AsUint;
                        break;
                    case MaterialVariantType::Types::Float:
                        ENGINE_ASSERT(meta.Constants.Get() && meta.Constants.Length() >= static_cast<int32>(m_Offset) + sizeof(float));
                        *((float*)(meta.Constants.Get() + m_Offset)) = e->Value.AsFloat;
                        break;
                    case MaterialVariantType::Types::Float2:
                        ENGINE_ASSERT(meta.Constants.Get() && meta.Constants.Length() >= static_cast<int32>(m_Offset) + sizeof(Float2));
                        *((Float2*)(meta.Constants.Get() + m_Offset)) = e->Value.AsFloat2();
                        break;
                    case MaterialVariantType::Types::Float3:
                        ENGINE_ASSERT(meta.Constants.Get() && meta.Constants.Length() >= static_cast<int32>(m_Offset) + sizeof(Float3));
                        *((Float3*)(meta.Constants.Get() + m_Offset)) = e->Value.AsFloat3();
                        break;
                    case MaterialVariantType::Types::Float4:
                    case MaterialVariantType::Types::Color:
                        ENGINE_ASSERT(meta.Constants.Get() && meta.Constants.Length() >= static_cast<int32>(m_Offset) + sizeof(Float4));
                        *((Float4*)(meta.Constants.Get() + m_Offset)) = e->Value.AsFloat4();
                        break;
                    case MaterialVariantType::Types::Double2:
                        ENGINE_ASSERT(meta.Constants.Get() && meta.Constants.Length() >= static_cast<int32>(m_Offset) + sizeof(Float2));
                        *((Float2*)(meta.Constants.Get() + m_Offset)) = (Float2)e->Value.AsDouble2();
                        break;
                    case MaterialVariantType::Types::Double3:
                        ENGINE_ASSERT(meta.Constants.Get() && meta.Constants.Length() >= static_cast<int32>(m_Offset) + sizeof(Float3));
                        *((Float3*)(meta.Constants.Get() + m_Offset)) = (Float3)e->Value.AsDouble3();
                        break;
                    case MaterialVariantType::Types::Double4:
                        ENGINE_ASSERT(meta.Constants.Get() && meta.Constants.Length() >= static_cast<int32>(m_Offset) + sizeof(Float4));
                        *((Float4*)(meta.Constants.Get() + m_Offset)) = (Float4)e->Value.AsDouble4();
                        break;
                    default: ;
                    }
                }
            }
            break;*/
        case MaterialParameterType::TextureGroupSampler:
            meta.Context->BindSampler(m_RegisterIndex, Streaming::GetTextureGroupSampler(_asInteger));
            break;
        /*case MaterialParameterType::GlobalSDF:
        {
            GlobalSignDistanceFieldPass::BindingData bindingData;
            if (GlobalSignDistanceFieldPass::Instance()->Get(meta.Buffers, bindingData))
                Platform::MemoryClear(&bindingData, sizeof(bindingData));
            meta.Context->BindSR(m_RegisterIndex, bindingData.Texture ? bindingData.Texture->ViewVolume() : nullptr);
            *((GlobalSignDistanceFieldPass::ConstantsData*)(meta.Constants.Get() + m_Offset)) = bindingData.Constants;
            break;
        }*/
        default:
            break;
        }
    }

    bool MaterialParameter::HasContentLoaded() const
    {
        return m_AsAsset == nullptr || m_AsAsset->IsLoaded();
    }

    void MaterialParameter::clone(const MaterialParameter* param)
    {
        // Clone data
        m_Type = param->m_Type;
        m_IsPublic = param->m_IsPublic;
        m_Override = param->m_Override;
        m_RegisterIndex = param->m_RegisterIndex;
        m_Offset = param->m_Offset;
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
        // m_AsGPUTexture = param->m_AsGPUTexture;
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

    void MaterialParams::Bind(MaterialParamsLink* link, MaterialParameter::BindMeta& meta)
    {
        ENGINE_ASSERT(link && link->This);
        for (int32 i = 0; i < link->This->Count(); i++)
        {
            const MaterialParamsLink* params = link;
            while (params->Down && !params->This->At(i).IsOverride())
            {
                params = params->Down;
            }

            params->This->At(i).Bind(meta);
        }
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
            case 1: // [Deprecated on 15.11.2019, expires on 15.11.2021]
            {
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
                    param->m_ParamId = UID::New();
                    param->m_Type = static_cast<MaterialParameterType>(stream->ReadByte());
                    param->m_IsPublic = stream->ReadBool();
                    param->m_Override = param->m_IsPublic;
                    stream->ReadString(&param->m_Name, 10421);
                    param->m_RegisterIndex = stream->ReadByte();
                    stream->ReadUint16(&param->m_Offset);

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
                    /*case MaterialParameterType::GameplayGlobal:
                        stream->Read(id);
                        param->m_AsAsset = AssetContent::LoadAsync<GameplayGlobals>(id);
                        break;*/
                    default:
                        break;
                    }
                }
            }
                break;
            case 2: // [Deprecated on 15.11.2019, expires on 15.11.2021]
            {
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
                    param->m_Override = param->m_IsPublic;
                    stream->ReadString(&param->m_Name, 10421);
                    param->m_RegisterIndex = stream->ReadByte();
                    stream->ReadUint16(&param->m_Offset);

                    // Read value
                    switch (param->m_Type)
                    {
                    case MaterialParameterType::Bool:
                        param->_asBool = stream->ReadBool();
                        break;
                    case MaterialParameterType::Integer:
                    case MaterialParameterType::SceneTexture:
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
                    /*case MaterialParameterType::GameplayGlobal:
                        stream->Read(id);
                        param->m_AsAsset = AssetContent::LoadAsync<GameplayGlobals>(id);
                        break;*/
                    default:
                        break;
                    }
                }
            }
                break;
            case 3:
            {
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
                    param->m_RegisterIndex = stream->ReadByte();
                    stream->ReadUint16(&param->m_Offset);

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
                    /*case MaterialParameterType::GameplayGlobal:
                        stream->Read(id);
                        param->m_AsAsset = AssetContent::LoadAsync<GameplayGlobals>(id);
                        break;*/
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
        // Skip serialization if no parameters to save
        if (IsEmpty())
            return;

        // Version
        stream->WriteUint16(3);

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
            stream->WriteByte(param->m_RegisterIndex);
            stream->WriteUint16(param->m_Offset);

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
        stream->WriteUint16(3);

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
                stream->WriteByte(param.RegisterIndex);
                stream->WriteUint16(param.Offset);

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
                case MaterialParameterType::GPUTextureVolume:
                case MaterialParameterType::GPUTextureCube:
                case MaterialParameterType::GPUTextureArray:
                case MaterialParameterType::GPUTexture:
                    stream->Write(param.AsGuid);
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
            data.Copy(stream.GetHandle(), stream.GetPosition());
        else
            data.Release();
    }

#if SE_EDITOR

    void MaterialParams::GetReferences(List<UID>& output) const
    {
        for (int32 i = 0; i < Count(); i++)
        {
            if (At(i).m_AsAsset)
                output.Add(At(i).m_AsAsset->GetID());
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